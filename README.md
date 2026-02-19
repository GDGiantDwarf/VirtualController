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
build.bat
```

**⚠️ Important:** Always build from the root directory. The project uses a unified build system - do NOT build from individual component folders (launcher/, server/, games/).

**Note**: This assumes Qt, ViGEmClient, and vcpkg are in their default installation paths. Edit paths in build.bat if needed.

All executables are output to: `build/bin/Release/`

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

## Development

### Adding Games

Games must follow this structure:
```
games/
└── your_game/
    ├── your_game.exe          # Must match folder name
    └── build/                 # Build directory
        └── bin/Release/
            └── your_game.exe
```

The launcher's GameScanner automatically discovers games in the `games/` folder.
It will only detect games whose .exe file matches the folder name.
Any matching .ico file will also be used in the launcher to represent your game
