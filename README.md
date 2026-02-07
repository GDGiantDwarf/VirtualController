# VirtualController

Multi-player virtual controller system using ViGEmBus for Windows with client-server network architecture.

## Components

- **Launcher** (`launcher/`) - Qt6 game launcher with local controller management (ViGEm)
- **Server** (`server/`) - TCP game server for multiplayer logic
- **Games** (`games/snake/`) - Networked game clients (SFML)

## Prerequisites

- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++
- CMake 3.16+
- Qt 6.10+ - for launcher
- SFML 3.0+ - for games
- [ViGEmBus Driver](https://github.com/nefarius/ViGEmBus/releases) - for launcher

## Quick Start

### Build Everything

```powershell
# Launcher (requires Qt6 and ViGEm paths - see BUILD.md)
cd launcher/build
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DVIGEM_SDK="<path-to-vigem>" `
  -DQt6_DIR="<path-to-qt6>/lib/cmake/Qt6"
cmake --build . --config Release

# Server (no dependencies)
cd ../../server/build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Snake (requires SFML path - see BUILD.md)
cd ../../games/snake/build
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DSFML_DIR="<path-to-sfml>/lib/cmake/SFML"
cmake --build . --config Release
```

**Note**: Replace `<path-to-*>` with your actual installation paths

## Running the Application

```powershell
# Start the server (in terminal 1)
.\server\build\bin\Release\GameServer.exe

# Start the launcher (in terminal 2)
.\launcher\build\bin\Release\GameLibraryLauncher.exe 127.0.0.1 8765

# Launch Snake from the launcher UI
# Or run directly:
.\games\snake\build\bin\Release\snake.exe 127.0.0.1 8765
```

## Debug Proxy (Optional)

Monitor all network traffic in real-time:

```powershell
python debug_proxy.py

# Then connect launcher to proxy instead:
.\launcher\build\bin\Release\GameLibraryLauncher.exe 127.0.0.1 8766
```

## Architecture

### Project Structure

```
VirtualController/
├── launcher/          # Qt6 launcher + ViGEm controller manager
│   ├── src/
│   └── build/
├── server/            # TCP game server
│   ├── src/
│   └── build/
└── games/snake/       # SFML networked game client
    ├── snake_client.cpp
    └── build/
```

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
