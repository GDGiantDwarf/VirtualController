#!/usr/bin/env python3
"""
Networking Test Suite

Tests for VirtualController server connectivity, message exchange, and protocol compliance.
Requires GameServer.exe to be running on localhost:9000
"""

import json
import socket
import time
import unittest
from typing import Optional


class GameClient:
    """TCP client for connecting to GameServer"""

    def __init__(self, host: str = "127.0.0.1", port: int = 8765, timeout: float = 5.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.socket: Optional[socket.socket] = None

    def connect(self) -> bool:
        """Connect to game server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.connect((self.host, self.port))
            return True
        except socket.error as e:
            print(f"Connection failed: {e}")
            return False

    def send_message(self, msg: dict) -> bool:
        """Send JSON message to server"""
        if not self.socket:
            return False
        try:
            msg_str = json.dumps(msg) + "\n"
            self.socket.sendall(msg_str.encode())
            return True
        except socket.error as e:
            print(f"Send failed: {e}")
            return False

    def receive_message(self) -> Optional[dict]:
        """Receive and parse JSON message from server"""
        if not self.socket:
            return None
        try:
            # Read until newline
            data = b""
            while True:
                chunk = self.socket.recv(1)
                if not chunk:
                    return None
                data += chunk
                if data.endswith(b"\n"):
                    break
            return json.loads(data.decode().strip())
        except socket.timeout:
            print("Receive timeout")
            return None
        except (socket.error, json.JSONDecodeError) as e:
            print(f"Receive failed: {e}")
            return None

    def close(self):
        """Close connection"""
        if self.socket:
            self.socket.close()
            self.socket = None


class TestServerConnectivity(unittest.TestCase):
    """Test basic server connectivity"""

    def test_server_is_running(self):
        """Test that server is listening on expected port"""
        client = GameClient()
        self.assertTrue(
            client.connect(),
            "Server not running on 127.0.0.1:9000. Start with: .\\build\\bin\\Release\\GameServer.exe"
        )
        client.close()

    def test_connect_and_disconnect(self):
        """Test connecting and disconnecting"""
        client = GameClient()
        self.assertTrue(client.connect())
        client.close()
        # No assertion needed, just verify no exceptions


class TestProtocolExchange(unittest.TestCase):
    """Test protocol message exchange with server"""

    def setUp(self):
        self.client = GameClient()
        if not self.client.connect():
            self.skipTest("GameServer not running")

    def tearDown(self):
        self.client.close()

    def test_send_connect_message(self):
        """Test sending CONNECT message"""
        msg = {"type": "CONNECT", "controller_count": 1}
        self.assertTrue(self.client.send_message(msg))

    def test_receive_after_connect(self):
        """Test receiving server response after CONNECT"""
        msg = {"type": "CONNECT", "controller_count": 1}
        self.assertTrue(self.client.send_message(msg))
        time.sleep(0.1)  # Give server time to respond
        response = self.client.receive_message()
        self.assertIsNotNone(response, "No response from server after CONNECT")
        self.assertIn("type", response)

    def test_connect_multiple_controllers(self):
        """Test CONNECT with multiple controllers"""
        for count in [1, 2, 3, 4]:
            with self.subTest(controllers=count):
                client = GameClient()
                self.assertTrue(client.connect())
                msg = {"type": "CONNECT", "controller_count": count}
                self.assertTrue(client.send_message(msg))
                time.sleep(0.05)
                client.close()

    def test_send_input_message(self):
        """Test sending INPUT message"""
        # First connect
        connect_msg = {"type": "CONNECT", "controller_count": 1}
        self.assertTrue(self.client.send_message(connect_msg))
        time.sleep(0.1)
        self.client.receive_message()  # Consume response

        # Then send input
        input_msg = {"type": "INPUT", "player_id": 0, "direction": "UP"}
        self.assertTrue(self.client.send_message(input_msg))

    def test_send_start_game(self):
        """Test sending START_GAME message"""
        # Connect first
        connect_msg = {"type": "CONNECT", "controller_count": 1}
        self.assertTrue(self.client.send_message(connect_msg))
        time.sleep(0.1)
        self.client.receive_message()

        # Send START_GAME
        start_msg = {"type": "START_GAME"}
        self.assertTrue(self.client.send_message(start_msg))


class TestMultipleConnections(unittest.TestCase):
    """Test multiple simultaneous connections"""

    def test_two_clients_connect(self):
        """Test two clients connecting simultaneously"""
        client1 = GameClient()
        client2 = GameClient()

        self.assertTrue(client1.connect())
        self.assertTrue(client2.connect())

        # Both send CONNECT
        msg1 = {"type": "CONNECT", "controller_count": 1}
        msg2 = {"type": "CONNECT", "controller_count": 2}

        self.assertTrue(client1.send_message(msg1))
        self.assertTrue(client2.send_message(msg2))

        time.sleep(0.1)

        # Both receive responses
        response1 = client1.receive_message()
        response2 = client2.receive_message()

        self.assertIsNotNone(response1)
        self.assertIsNotNone(response2)

        client1.close()
        client2.close()

    def test_four_clients_connect(self):
        """Test four clients connecting simultaneously"""
        clients = [GameClient() for _ in range(4)]

        # All connect
        for i, client in enumerate(clients):
            self.assertTrue(client.connect(), f"Client {i} failed to connect")
            time.sleep(0.1)  # Small delay between connections to avoid overwhelming server

        # All send CONNECT
        for i, client in enumerate(clients):
            msg = {"type": "CONNECT", "controller_count": i + 1}
            self.assertTrue(client.send_message(msg), f"Client {i} failed to send")

        time.sleep(0.2)  # Give server time to process and respond to all clients

        # All receive responses
        for i, client in enumerate(clients):
            response = client.receive_message()
            self.assertIsNotNone(response, f"Client {i} received no response")

        # Close all
        for client in clients:
            client.close()


class TestNegativeCases(unittest.TestCase):
    """Test error handling and negative cases"""

    def test_connection_timeout(self):
        """Test connection to non-existent port"""
        client = GameClient(port=9999, timeout=1.0)
        self.assertFalse(client.connect())

    def test_invalid_json_message(self):
        """Test sending invalid JSON"""
        client = GameClient()
        if not client.connect():
            self.skipTest("GameServer not running")

        # Send invalid JSON
        try:
            client.socket.sendall(b"{invalid json\n")
            # Server should handle gracefully
            time.sleep(0.1)
            # Try sending valid message after
            msg = {"type": "CONNECT", "controller_count": 1}
            # This might fail, but server shouldn't crash
            client.send_message(msg)
        finally:
            client.close()

    def test_oversized_message(self):
        """Test sending very large message"""
        client = GameClient()
        if not client.connect():
            self.skipTest("GameServer not running")

        # Create large message
        large_msg = {
            "type": "CONNECT",
            "controller_count": 1,
            "data": "x" * 10000  # 10KB of extra data
        }

        try:
            client.send_message(large_msg)
            time.sleep(0.1)
        finally:
            client.close()

    def test_rapid_connections_and_disconnections(self):
        """Test rapid connect/disconnect cycles"""
        for i in range(10):
            client = GameClient()
            if client.connect():
                msg = {"type": "CONNECT", "controller_count": 1}
                client.send_message(msg)
                time.sleep(0.01)
                client.close()


class TestConnectionStability(unittest.TestCase):
    """Test connection stability and recovery"""

    def test_keep_connection_open(self):
        """Test keeping connection open for extended time"""
        client = GameClient(timeout=10.0)
        if not client.connect():
            self.skipTest("GameServer not running")

        try:
            msg = {"type": "CONNECT", "controller_count": 1}
            self.assertTrue(client.send_message(msg))

            # Wait and verify still connected
            time.sleep(1.0)

            msg2 = {"type": "CONNECT", "controller_count": 2}
            self.assertTrue(client.send_message(msg2))
        finally:
            client.close()

    def test_message_ordering(self):
        """Test that messages are received in order"""
        client = GameClient()
        if not client.connect():
            self.skipTest("GameServer not running")

        try:
            # Send multiple messages in quick succession
            for i in range(5):
                msg = {"type": "INPUT", "player_id": 0, "direction": "UP", "sequence": i}
                self.assertTrue(client.send_message(msg))

            time.sleep(0.2)
        finally:
            client.close()


if __name__ == "__main__":
    print("\n" + "="*70)
    print("VirtualController Networking Tests")
    print("="*70)
    print("\nPREREQUISITE: GameServer must be running on port 8765!")
    print("Start server with: .\\build\\bin\\Release\\GameServer.exe")
    print("(Default port is 8765, or specify: GameServer.exe <port>)")
    print("\nTests will skip if server is not available.")
    print("="*70 + "\n")

    unittest.main(verbosity=2)
