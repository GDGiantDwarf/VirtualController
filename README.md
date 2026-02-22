# VirtualController

Multi-player virtual controller system using ViGEmBus for Windows with client-server network architecture.

## Components

- **Launcher** (`launcher/`) - Qt6 game launcher with local controller management (ViGEm)
- **Server** (`server/`) - TCP game server for multiplayer logic
- **Games** (`games/snake/`) - Networked game clients (SFML)

## Prerequisites

- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++17 or later
- CMake 3.16+
- Qt 6.10+ - for launcher
- SFML 3.0+ - for games
- [ViGEmBus Driver](https://github.com/nefarius/ViGEmBus/releases) - for virtual controllers.

## Quick Start
To build the project, run `build.bat` from the **root directory only**:

```bash
# Build everything (launcher, server, snake, and tests)
build.bat

# Build without tests (faster for development)
build.bat --no-tests
```

**⚠️ Important:** Always build from the root directory. The project uses a unified build system - do NOT build from individual component folders (launcher/, server/, games/).

**Note**: This assumes Qt, ViGEmClient, and vcpkg are in their default installation paths. Edit paths in build.bat if needed.

All executables are output to: `build/bin/Release/`

## User Guides

New to VirtualController? Start here:

- **[General User Guide (PDF)](General%20User%20Guide.pdf)** - How to use the system with any game and any number of players
- **[Snake Game User Guide (PDF)](Snake%20Game%20User%20Guide.pdf)** - Specific guide to playing the Snake game

## Running the Application

```powershell
# Start the server (in terminal 1)
.\build\bin\Release\GameServer.exe

# Start the launcher (in terminal 2)
.\build\bin\Release\GameLibraryLauncher.exe 127.0.0.1 8765

# Launch Snake from the launcher UI (recommended)
# Or run directly:
.\build\bin\Release\snake.exe 127.0.0.1 8765
```

## Debug Proxy (Optional)

Monitor all network traffic in real-time:

```powershell
python debug_proxy.py

# Then connect launcher to proxy instead:
.\build\bin\Release\GameLibraryLauncher.exe 127.0.0.1 8766
```

## Testing

### Automated Tests (GitHub Actions CI)

C++ unit tests and protocol validation run automatically on every push and pull request:

```bash
# Run locally what CI runs:
cd build/tests
ctest -C Release                  # 33 C++ unit tests
cd ../..
python tests\test_protocol.py -v # 36 protocol tests
```

**CI Status**: All 69 automated tests must pass before merging to main/develop.

### Full Test Suite (Local Testing)

For complete testing including C++ unit tests and network functionality:

```bash
# C++ unit tests (33 tests - server, launcher, snake logic)
cd build\tests
ctest -C Release

# Python tests
python tests\test_protocol.py -v      # 36 protocol tests
python tests\test_networking.py -v    # 15 networking tests (requires server)
```

**Test Coverage**: 84 total tests
- C++ unit tests: 33 (GameLogic, PlayerIds, ControllerManagement, GameMechanics)
- Python tests: 51 (36 protocol + 15 networking)

See [tests/README.md](tests/README.md) for detailed test documentation, [Testing Policy](docs/TESTING_POLICY.md) for testing strategy, and [Debugging & Testing](docs/08_DEBUGGING_AND_TESTING.md) for procedures.

## Architecture

### Project Structure

```
VirtualController/
├── build/             # Unified build output (all executables here)
│   └── bin/Release/   # GameLibraryLauncher.exe, GameServer.exe, snake.exe
├── launcher/          # Qt6 launcher + ViGEm controller manager
│   └── src/
├── server/            # TCP game server
│   └── src/
└── games/snake/       # SFML networked game client
    └── snake.cpp
```

**Note:** Individual component folders (launcher/, server/, games/) should NOT contain build/ directories. All builds use the root `build/` directory.

### Multiplayer Flow

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│  Client 1   │         │              │         │  Client 2   │
│   (SFML)    │◄───────►│ Game Server  │◄───────►│   (SFML)    │
│             │  TCP    │ (Game Logic) │  TCP    │             │
└─────────────┘         └──────────────┘         └─────────────┘
      ↑                                                  ↑
   Keyboard/                                        Keyboard/
   Joystick                                         Joystick
```

- **Server**: Game logic, collision detection, state management (120ms tick)
- **Client**: Input capture, rendering, JSON protocol over TCP
- **Launcher**: Discovers games, manages local virtual controllers

## Documentation

Comprehensive technical documentation is available in the `docs/` directory:

- **[Documentation Overview](docs/00_DOCUMENTATION_OVERVIEW.md)** - Start here for navigation
- **[Core Architecture](docs/01_CORE_ARCHITECTURE.md)** - System design and components
- **[Server & Networking](docs/02_SERVER_AND_NETWORKING.md)** - Server implementation details
- **[Game Protocol](docs/03_GAME_PROTOCOL.md)** - JSON message specification
- **[Launcher & Discovery](docs/04_LAUNCHER_AND_DISCOVERY.md)** - Game launcher details
- **[Snake Game Implementation](docs/05_SNAKE_GAME_IMPLEMENTATION.md)** - Reference game walkthrough
- **[Adding New Games](docs/06_ADDING_NEW_GAMES.md)** - Step-by-step development guide
- **[Build System](docs/07_BUILD_SYSTEM.md)** - CMake configuration and compilation
- **[Debugging & Testing](docs/08_DEBUGGING_AND_TESTING.md)** - Testing scenarios and troubleshooting
- **[Testing Policy](docs/Testing%20Policy.pdf)** - Testing strategy, frameworks, and test types

## Development

### Adding Games

Games must follow this structure:
```
games/
└── your_game/
    ├── your_game.cpp          # Game source
    ├── your_game.ico          # 48×48 icon
    └── CMakeLists.txt         # Build configuration
```

See [Adding New Games](docs/06_ADDING_NEW_GAMES.md) for complete instructions.

The launcher's GameScanner automatically discovers games in the build output directory (`build/bin/Release/`).
It will only detect games whose .exe file matches the game name.
Any matching .ico file will also be used in the launcher to represent your game.
