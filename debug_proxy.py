#!/usr/bin/env python3
"""
TCP Proxy Debugger: Sits between client and server, logs all traffic in real-time.
Usage: python debug_proxy.py [--listen-port 8766] [--server-host 127.0.0.1] [--server-port 8765]
"""

import socket
import threading
import sys
from datetime import datetime

# Default configuration
LISTEN_PORT = 8766
SERVER_HOST = "127.0.0.1"
SERVER_PORT = 8765
SHUTDOWN_EVENT = threading.Event()

def format_data(data, direction):
    """Format data for display."""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    
    # Try to decode as text, fallback to hex
    try:
        text = data.decode('utf-8', errors='replace')
        # Show printable characters and hex for non-printable
        if len(text) > 100:
            return f"[{timestamp}] {direction}: {repr(text[:100])}... ({len(data)} bytes)"
        return f"[{timestamp}] {direction}: {repr(text)} ({len(data)} bytes)"
    except:
        hex_str = ' '.join(f'{b:02x}' for b in data[:64])
        if len(data) > 64:
            hex_str += f" ... ({len(data)} bytes total)"
        return f"[{timestamp}] {direction}: {hex_str}"

def handle_client(client_socket, client_addr):
    """Handle a client connection, forwarding to server and logging traffic."""
    print(f"\n[CLIENT CONNECTED] {client_addr[0]}:{client_addr[1]}")
    
    # Connect to the actual server
    try:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.settimeout(5)  # 5 second timeout
        server_socket.connect((SERVER_HOST, SERVER_PORT))
        server_socket.settimeout(1)  # 1 second timeout for recv
        print(f"[SERVER CONNECTED] {SERVER_HOST}:{SERVER_PORT}\n")
    except Exception as e:
        print(f"[ERROR] Failed to connect to server: {e}")
        client_socket.close()
        return
    
    # Set timeout on client socket too
    client_socket.settimeout(1)
    
    # Start threads to forward data in both directions
    client_to_server = threading.Thread(
        target=forward_data, 
        args=(client_socket, server_socket, f"CLIENT→SERVER", client_addr)
    )
    server_to_client = threading.Thread(
        target=forward_data, 
        args=(server_socket, client_socket, f"SERVER→CLIENT", client_addr)
    )
    
    client_to_server.daemon = True
    server_to_client.daemon = True
    
    client_to_server.start()
    server_to_client.start()
    
    # Wait for both threads to finish
    client_to_server.join()
    server_to_client.join()
    
    print(f"[CLIENT DISCONNECTED] {client_addr[0]}:{client_addr[1]}\n")

def forward_data(source, destination, direction, client_addr):
    """Forward data from source to destination and log it."""
    try:
        while not SHUTDOWN_EVENT.is_set():
            try:
                data = source.recv(4096)
                if not data:
                    break
                
                # Log the data
                log_msg = format_data(data, direction)
                print(log_msg)
                
                # Forward to destination
                destination.sendall(data)
            except socket.timeout:
                # Timeout is normal, just continue
                continue
            except socket.error:
                # Connection closed
                break
    except Exception as e:
        pass  # Connection closed
    finally:
        try:
            source.close()
            destination.close()
        except:
            pass

def main():
    """Main proxy server loop."""
    print("=" * 70)
    print("TCP Debug Proxy - Real-time Message Logger")
    print("=" * 70)
    print(f"Listening on:    127.0.0.1:{LISTEN_PORT}")
    print(f"Forwarding to:   {SERVER_HOST}:{SERVER_PORT}")
    print("\nStart your client with: 127.0.0.1 {}\n".format(LISTEN_PORT))
    print("Press Ctrl-C to shutdown...\n")
    print("=" * 70 + "\n")
    
    proxy_socket = None
    try:
        # Create listening socket
        proxy_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        proxy_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        proxy_socket.settimeout(1)  # 1 second timeout for accept
        proxy_socket.bind(("127.0.0.1", LISTEN_PORT))
        proxy_socket.listen(5)
        
        while not SHUTDOWN_EVENT.is_set():
            try:
                client_socket, client_addr = proxy_socket.accept()
                # Handle each client in a separate thread
                client_thread = threading.Thread(
                    target=handle_client, 
                    args=(client_socket, client_addr)
                )
                client_thread.daemon = True
                client_thread.start()
            except socket.timeout:
                # Timeout is normal for non-blocking accept
                continue
            except KeyboardInterrupt:
                print("\n[SHUTDOWN] Proxy shutting down...")
                SHUTDOWN_EVENT.set()
                break
            except Exception as e:
                if not SHUTDOWN_EVENT.is_set():
                    print(f"[ERROR] {e}")
    
    except KeyboardInterrupt:
        print("\n[SHUTDOWN] Proxy shutting down...")
        SHUTDOWN_EVENT.set()
    except Exception as e:
        print(f"[FATAL ERROR] {e}")
    finally:
        SHUTDOWN_EVENT.set()  # Ensure shutdown event is set
        if proxy_socket:
            try:
                proxy_socket.close()
            except:
                pass
        print("[DONE] Proxy closed")
        sys.exit(0)  # Force exit

if __name__ == "__main__":
    main()
