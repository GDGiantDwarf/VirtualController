# VirtualController Unit Tests

This directory contains C++ unit tests for core components using Google Test.

## Test Suites

### Server Tests (`tests/server/`)
- **test_game_logic.cpp** (10 tests): GameLogic state management, player initialization, game transitions
- **test_player_ids.cpp** (7 tests): Player ID assignment and tracking logic

### Launcher Tests (`tests/launcher/`)
- **test_server_sync.cpp** (6 tests): Controller count tracking and duplicate detection

### Snake Game Tests (`tests/snake/`)
- **test_game_mechanics.cpp** (10 tests): Collision detection, movement, and boundary checks

## Python Tests (`test_protocol.py`, `test_networking.py`)
- **test_protocol.py**: 36 unit tests for JSON protocol message validation (no server required)
- **test_networking.py**: 15 integration tests for live server connectivity (requires GameServer running)

## Building & Running

### Build C++ Tests
```bash
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --config Release
```

### Run All Tests
```bash
# Python tests (no build required)
cd tests
python test_protocol.py           # Protocol validation
python test_networking.py         # Requires server running

# C++ tests
cd build/tests
ctest -C Release                  # Run all C++ tests
ctest -C Release --verbose        # Verbose output
```

### Run Individual C++ Tests
```bash
cd build/tests/Release
.\test_game_logic.exe
.\test_player_ids.exe
.\test_controller_management.exe
.\test_game_mechanics.exe
```

## Coverage

**C++ Unit Tests** (33 tests):
- Server: GameLogic state, player initialization, ID assignment (17 tests)
- Launcher: ServerControllerSync updates (6 tests)
- Snake: Movement, collision, boundaries (10 tests)

**Python Tests** (51 tests):
- Protocol: JSON message parsing/validation (36 tests)
- Networking: Server connectivity, message exchange (15 tests)

**Total: 84 automated tests** covering core logic and integration paths
