#!/usr/bin/env python3
"""
Protocol Test Suite

Tests for VirtualController TCP protocol message format, parsing, and validation.
Covers unit-level protocol validation without requiring server.
"""

import json
import unittest
from typing import Dict, Any


class ProtocolValidator:
    """Validates VirtualController protocol messages"""

    @staticmethod
    def parse_message(data: str) -> Dict[str, Any]:
        """Parse raw message into JSON"""
        return json.loads(data)

    @staticmethod
    def validate_connect(msg: Dict[str, Any]) -> bool:
        """Validate CONNECT message format"""
        required = {"type", "controller_count"}
        return required.issubset(msg.keys()) and isinstance(msg["controller_count"], int)

    @staticmethod
    def validate_input(msg: Dict[str, Any]) -> bool:
        """Validate INPUT message format"""
        required = {"type", "player_id", "direction"}
        if not required.issubset(msg.keys()):
            return False
        # Valid directions: UP, DOWN, LEFT, RIGHT
        valid_dirs = {"UP", "DOWN", "LEFT", "RIGHT"}
        return msg["direction"] in valid_dirs

    @staticmethod
    def validate_state_update(msg: Dict[str, Any]) -> bool:
        """Validate STATE_UPDATE message format"""
        required = {"type", "state", "players"}
        if not required.issubset(msg.keys()):
            return False
        if not isinstance(msg["players"], list):
            return False
        # Each player should have id, x, y, direction
        for player in msg["players"]:
            if not all(
                k in player for k in ["id", "x", "y", "direction"]
            ):
                return False
        return True

    @staticmethod
    def validate_start_game(msg: Dict[str, Any]) -> bool:
        """Validate START_GAME message format"""
        return msg.get("type") == "START_GAME"

    @staticmethod
    def validate_reset_game(msg: Dict[str, Any]) -> bool:
        """Validate RESET_GAME message format"""
        return msg.get("type") == "RESET_GAME"


class TestProtocolMessages(unittest.TestCase):
    """Test protocol message validation"""

    def setUp(self):
        self.validator = ProtocolValidator()

    # ============= CONNECT MESSAGE TESTS =============

    def test_valid_connect_message(self):
        """Test valid CONNECT message"""
        msg = self.validator.parse_message('{"type": "CONNECT", "controller_count": 4}')
        self.assertTrue(self.validator.validate_connect(msg))

    def test_connect_with_1_controller(self):
        """Test CONNECT with minimum controllers"""
        msg = self.validator.parse_message('{"type": "CONNECT", "controller_count": 1}')
        self.assertTrue(self.validator.validate_connect(msg))

    def test_connect_with_4_controllers(self):
        """Test CONNECT with maximum controllers"""
        msg = self.validator.parse_message('{"type": "CONNECT", "controller_count": 4}')
        self.assertTrue(self.validator.validate_connect(msg))

    def test_connect_missing_type(self):
        """Test CONNECT missing type field (negative)"""
        msg = self.validator.parse_message('{"controller_count": 2}')
        self.assertFalse(self.validator.validate_connect(msg))

    def test_connect_missing_controller_count(self):
        """Test CONNECT missing controller_count (negative)"""
        msg = self.validator.parse_message('{"type": "CONNECT"}')
        self.assertFalse(self.validator.validate_connect(msg))

    def test_connect_controller_count_wrong_type(self):
        """Test CONNECT with string controller_count (negative)"""
        msg = self.validator.parse_message('{"type": "CONNECT", "controller_count": "2"}')
        self.assertFalse(self.validator.validate_connect(msg))

    def test_connect_extra_fields(self):
        """Test CONNECT with extra fields (should still pass)"""
        msg = self.validator.parse_message(
            '{"type": "CONNECT", "controller_count": 2, "version": "1.0"}'
        )
        self.assertTrue(self.validator.validate_connect(msg))

    # ============= INPUT MESSAGE TESTS =============

    def test_valid_input_up(self):
        """Test valid INPUT message with UP direction"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 0, "direction": "UP"}')
        self.assertTrue(self.validator.validate_input(msg))

    def test_valid_input_down(self):
        """Test valid INPUT message with DOWN direction"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 1, "direction": "DOWN"}')
        self.assertTrue(self.validator.validate_input(msg))

    def test_valid_input_left(self):
        """Test valid INPUT message with LEFT direction"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 2, "direction": "LEFT"}')
        self.assertTrue(self.validator.validate_input(msg))

    def test_valid_input_right(self):
        """Test valid INPUT message with RIGHT direction"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 3, "direction": "RIGHT"}')
        self.assertTrue(self.validator.validate_input(msg))

    def test_input_missing_type(self):
        """Test INPUT missing type (negative)"""
        msg = self.validator.parse_message('{"player_id": 0, "direction": "UP"}')
        self.assertFalse(self.validator.validate_input(msg))

    def test_input_missing_player_id(self):
        """Test INPUT missing player_id (negative)"""
        msg = self.validator.parse_message('{"type": "INPUT", "direction": "UP"}')
        self.assertFalse(self.validator.validate_input(msg))

    def test_input_missing_direction(self):
        """Test INPUT missing direction (negative)"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 0}')
        self.assertFalse(self.validator.validate_input(msg))

    def test_input_invalid_direction(self):
        """Test INPUT with invalid direction (negative)"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 0, "direction": "DIAGONAL"}')
        self.assertFalse(self.validator.validate_input(msg))

    def test_input_direction_lowercase(self):
        """Test INPUT with lowercase direction (negative)"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": 0, "direction": "up"}')
        self.assertFalse(self.validator.validate_input(msg))

    def test_input_negative_player_id(self):
        """Test INPUT with negative player_id (should pass format validation)"""
        msg = self.validator.parse_message('{"type": "INPUT", "player_id": -1, "direction": "UP"}')
        # Format is valid, but semantic validation would reject this
        self.assertTrue(self.validator.validate_input(msg))

    # ============= STATE_UPDATE MESSAGE TESTS =============

    def test_valid_state_update_single_player(self):
        """Test valid STATE_UPDATE with one player"""
        data = '''{
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": [{"id": 0, "x": 10, "y": 10, "direction": "UP"}]
        }'''
        msg = self.validator.parse_message(data)
        self.assertTrue(self.validator.validate_state_update(msg))

    def test_valid_state_update_four_players(self):
        """Test valid STATE_UPDATE with four players"""
        data = '''{
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": [
                {"id": 0, "x": 10, "y": 10, "direction": "UP"},
                {"id": 1, "x": 20, "y": 20, "direction": "DOWN"},
                {"id": 2, "x": 30, "y": 30, "direction": "LEFT"},
                {"id": 3, "x": 40, "y": 40, "direction": "RIGHT"}
            ]
        }'''
        msg = self.validator.parse_message(data)
        self.assertTrue(self.validator.validate_state_update(msg))

    def test_state_update_missing_players_array(self):
        """Test STATE_UPDATE missing players array (negative)"""
        data = '{"type": "STATE_UPDATE", "state": "ACTIVE"}'
        msg = self.validator.parse_message(data)
        self.assertFalse(self.validator.validate_state_update(msg))

    def test_state_update_players_not_array(self):
        """Test STATE_UPDATE with players as object instead of array (negative)"""
        data = '{"type": "STATE_UPDATE", "state": "ACTIVE", "players": {}}'
        msg = self.validator.parse_message(data)
        self.assertFalse(self.validator.validate_state_update(msg))

    def test_state_update_player_missing_id(self):
        """Test STATE_UPDATE with player missing id field (negative)"""
        data = '''{
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": [{"x": 10, "y": 10, "direction": "UP"}]
        }'''
        msg = self.validator.parse_message(data)
        self.assertFalse(self.validator.validate_state_update(msg))

    def test_state_update_player_missing_position(self):
        """Test STATE_UPDATE with player missing position (negative)"""
        data = '''{
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": [{"id": 0, "direction": "UP"}]
        }'''
        msg = self.validator.parse_message(data)
        self.assertFalse(self.validator.validate_state_update(msg))

    def test_state_update_empty_players(self):
        """Test STATE_UPDATE with empty players array"""
        data = '{"type": "STATE_UPDATE", "state": "LOBBY", "players": []}'
        msg = self.validator.parse_message(data)
        self.assertTrue(self.validator.validate_state_update(msg))

    def test_state_update_with_extra_player_fields(self):
        """Test STATE_UPDATE with extra player fields (should still pass)"""
        data = '''{
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": [
                {"id": 0, "x": 10, "y": 10, "direction": "UP", "score": 100, "alive": true}
            ]
        }'''
        msg = self.validator.parse_message(data)
        self.assertTrue(self.validator.validate_state_update(msg))

    # ============= START_GAME MESSAGE TESTS =============

    def test_valid_start_game(self):
        """Test valid START_GAME message"""
        msg = self.validator.parse_message('{"type": "START_GAME"}')
        self.assertTrue(self.validator.validate_start_game(msg))

    def test_start_game_with_extra_fields(self):
        """Test START_GAME with extra fields"""
        msg = self.validator.parse_message('{"type": "START_GAME", "timestamp": 123456}')
        self.assertTrue(self.validator.validate_start_game(msg))

    # ============= RESET_GAME MESSAGE TESTS =============

    def test_valid_reset_game(self):
        """Test valid RESET_GAME message"""
        msg = self.validator.parse_message('{"type": "RESET_GAME"}')
        self.assertTrue(self.validator.validate_reset_game(msg))

    # ============= MALFORMED MESSAGES =============

    def test_invalid_json(self):
        """Test invalid JSON (negative)"""
        with self.assertRaises(json.JSONDecodeError):
            self.validator.parse_message('{"type": "CONNECT" invalid}')

    def test_empty_json(self):
        """Test empty JSON object"""
        msg = self.validator.parse_message('{}')
        self.assertFalse(self.validator.validate_connect(msg))

    def test_json_missing_body(self):
        """Test truncated JSON (negative)"""
        with self.assertRaises(json.JSONDecodeError):
            self.validator.parse_message('{"type": "CONNECT"')

    # ============= OVERSIZED MESSAGES =============

    def test_large_player_count(self):
        """Test CONNECT with extremely large player count"""
        msg = self.validator.parse_message('{"type": "CONNECT", "controller_count": 99999}')
        # Format is valid, but semantic validation would reject this
        self.assertTrue(self.validator.validate_connect(msg))

    def test_large_state_update(self):
        """Test STATE_UPDATE with many players"""
        players = [
            {"id": i, "x": 10 * i, "y": 20 * i, "direction": "UP"}
            for i in range(100)
        ]
        data = {
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": players
        }
        msg_str = json.dumps(data)
        msg = self.validator.parse_message(msg_str)
        self.assertTrue(self.validator.validate_state_update(msg))


class TestProtocolSerialization(unittest.TestCase):
    """Test message serialization"""

    def setUp(self):
        self.validator = ProtocolValidator()

    def test_serialize_connect(self):
        """Test serializing CONNECT message"""
        msg = {"type": "CONNECT", "controller_count": 2}
        serialized = json.dumps(msg)
        deserialized = self.validator.parse_message(serialized)
        self.assertEqual(msg, deserialized)

    def test_serialize_input(self):
        """Test serializing INPUT message"""
        msg = {"type": "INPUT", "player_id": 0, "direction": "UP"}
        serialized = json.dumps(msg)
        deserialized = self.validator.parse_message(serialized)
        self.assertEqual(msg, deserialized)

    def test_serialize_state_update(self):
        """Test serializing STATE_UPDATE message"""
        msg = {
            "type": "STATE_UPDATE",
            "state": "ACTIVE",
            "players": [
                {"id": 0, "x": 10, "y": 10, "direction": "UP"},
                {"id": 1, "x": 20, "y": 20, "direction": "DOWN"}
            ]
        }
        serialized = json.dumps(msg)
        deserialized = self.validator.parse_message(serialized)
        self.assertEqual(msg, deserialized)


if __name__ == "__main__":
    # Run tests with verbose output
    unittest.main(verbosity=2)
