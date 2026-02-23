# Core Architecture

## System Overview

VirtualController is a distributed multiplayer gaming system consisting of:

1. **Game Server** - Centralized game logic and state management
2. **Game Launcher** - User interface for discovering and launching games
3. **Game Clients** - Networked game implementations (e.g., Snake, Karting)

```
                          ┌─────────────────┐
                          │   Game Server   │
                          └────────┬────────┘
                                   │ TCP
                    ┌──────────────┼──────────────┐
                    │              │              │
                    ▼              ▼              ▼
              ┌──────────┐   ┌──────────┐   ┌──────────┐
              │ Client 1 │   │ Client 2 │   │ Client 3 │
              │ (Snake/  │   │ (Snake/  │   │ (Snake/  │
              │ Karting) │   │ Karting) │   │ Karting) │
              └──────────┘   └──────────┘   └──────────┘
                    │              │              │
              ┌─────▼─────┐  ┌─────▼─────┐  ┌─────▼─────┐
              │ Keyboard/ │  │ Keyboard/ │  │ Keyboard/ │
              │ Screen/   │  │ Screen/   │  │ Screen/   │
              │ Joystick  │  │ Joystick  │  │ Joystick  │
              └───────────┘  └───────────┘  └───────────┘
                    │              │              │
              ┌─────▼──────────────▼──────────────▼────┐
              │  Game Launcher (Discovers & Launches)  │
              └────────────────────────────────────────┘
```

## Component Responsibilities

### Game Server
- **Centralized Logic**: Runs game rules engine independently
- **State Management**: Maintains authoritative game state
- **Simulation**: 120ms tick rate for game updates
- **Connection Handling**: Manages multiple client connections
- **Player Attribution**: Maps local player IDs to global IDs per connection
- **Broadcasting**: Sends state updates to all connected clients

### Game Launcher
- **Game Discovery**: Scans `build/bin/Release/` for `.exe` files
- **Game Metadata**: Loads `.ico` icons from build directory
- **UI**: Presents available games to user
- **Game Execution**: Launches selected game with server parameters
- **Virtual Controllers**: Manages input mapping for multiple controllers

### Game Clients
- **Input Capture**: Reads keyboard/joystick input locally
- **Controller Count Reporting**: Tells server how many controllers are connected
- **Networked State**: Receives authoritative state from server
- **Local Rendering**: Draws game based on server state
- **Validation**: Verifies player IDs and state consistency

## Project Directory Structure

```
VirtualController/
├── build/                          # Unified build output directory
│   └── bin/Release/
│       ├── GameLibraryLauncher.exe  # Main launcher application
│       ├── GameServer.exe           # Game server
│       ├── snake.exe                # Snake game client
│       ├── snake.ico                # Game icon
│       ├── karting.exe              # Karting game client
│       ├── karting.ico              # Game icon
│       └── [DLLs and libraries]     # Qt, SFML, system DLLs
│
├── launcher/                        # Game launcher application
│   └── src/
│       ├── main.cpp                 # Launcher entry point
│       ├── scanner/GameScanner.cpp  # Game discovery
│       └── [other UI components]
│
├── server/                          # Game server
│   └── src/
│       ├── main.cpp                 # Server entry point
│       ├── GameServer.cpp           # Connection & message handling
│       ├── GameLogic.cpp            # Game rules
│       ├── Connection.cpp           # Per-client TCP management
│       └── Protocol.h               # Shared protocol definitions
│
├── games/                           # Game source directories
│   ├── snake/
│   │   ├── snake.cpp                # Snake game implementation
│   │   ├── snake.ico                # Game icon (copied to build)
│   │   └── CMakeLists.txt
│   └── karting/
│       ├── karting.cpp              # Karting game implementation
│       ├── karting.ico              # Game icon (copied to build)
│       └── CMakeLists.txt           # Build configuration
│
├── CMakeLists.txt                   # Root CMake configuration
├── build.bat                        # Build script
└── docs/                            # Documentation
```

## Multiplayer Game Flow

### Startup Phase

1. **User runs GameLibraryLauncher.exe**
   - Launcher scans `build/bin/Release/` for games
   - Displays available games with icons
   - Awaits game selection

2. **User selects and launches a game**
   - Launcher spawns game client with server address and port
   - Game client connects to GameServer via TCP

3. **Game client connects to server**
   - Client sends CONNECT message with controller count
   - Server assigns unique player IDs to this connection
   - Server initializes game state if in LOBBY

### Lobby Phase

- **Game State**: LOBBY (state=0)
- **Server**: Waiting for start signal
- **Clients**: 
  - Display number of connected players
  - Show START button
  - No game input processed yet

### Game Phase

- **Game State**: ACTIVE (state=1)
- **Server Loop** (every 120ms):
  1. Collect pending input from all clients
  2. Apply inputs to game logic
  3. Simulate physics/collisions
  4. Update player scores
  5. Check win/lose conditions
  6. Broadcast new state to all clients
- **Clients**:
  - Capture local input (keyboard/joystick)
  - Send input commands to server with global player IDs
  - Render received game state
  - Display current scores and alive players

### End Phase

- **Game State**: ENDED (state=2)
- **Server**: All players dead, waiting for reset
- **Clients**: 
  - Display end screen with final scores
  - Show BACK TO START button
  - Clicking button sends RESET_GAME message

## Player ID Attribution System

**Problem**: Multiple PCs can connect, each with multiple controllers. Server must distinguish between:
- PC1's Player 1 vs PC2's Player 1 (different snakes)

**Solution**: Per-connection player mapping

```
┌─────────────────────────────────────────┐
│ PC 1: 2 Controllers                     │
│ Local Indices: 0, 1                     │
│ Global IDs: 0, 1         ◄──────┐       │
└─────────────────────────────────┼───────┘
                                  │ Mapping
┌─────────────────────────────────┼───────┐
│ PC 2: 2 Controllers             │       │
│ Local Indices: 0, 1             │       │
│ Global IDs: 2, 3         ◄──────┘       │
└─────────────────────────────────────────┘
```

When PC2 sends INPUT with `playerId=1`, server converts it to global `playerId=3` before processing.

## Build System

**Unified Output**: All components build to `build/bin/Release/`

**Key Principle**: No component has its own `build/` directory. This ensures:
- Single deployment folder
- Portability (can move `build/` anywhere)
- Consistency across platforms
- Easy game discovery (scan single folder)

**Build Process**:
```bash
# Root CMakeLists.txt calls add_subdirectory() for:
# - launcher/
# - server/
# - games/snake/
# Each sets RUNTIME_OUTPUT_DIRECTORY to root build/bin/Release/
```

## Contribution Areas

### Adding Games
- Implement game client in C++17 + SFML
- Use provided protocol/networking code
- Follow naming conventions (folder name must match .exe name)
- See [Adding New Games](07_ADDING_NEW_GAMES.md)

### Extending Server
- Modify GameLogic for new game mechanics
- Add protocol messages for new features
- See [Server & Networking](02_SERVER_AND_NETWORKING.md)

### Improving Launcher
- Enhanced UI/UX
- Game metadata (descriptions, screenshots)
- Controller configuration UI
- See [Launcher & Game Discovery](04_LAUNCHER_AND_DISCOVERY.md)

### Debugging & Tools
- Network monitoring
- Performance profiling
- Statistics/reporting
- See [Debugging & Testing](09_DEBUGGING_AND_TESTING.md)

## Performance Characteristics

| Aspect | Value |
|--------|-------|
| **Max Players** | 4 simultaneous |
| **Server Tick Rate** | 120ms (8.3 updates/sec) |
| **Network Protocol** | TCP (JSON over text) |
| **Message Overhead** | ~50 bytes per input, ~500 bytes per state |
| **Typical Latency** | <50ms local, variable over internet |
| **Resolution** | 60x40 game grid (configurable per game) |

## Architecture Decisions

### Why TCP over UDP?
- Simplicity and reliability over latency
- Ordered message delivery guaranteed
- Easier debugging with text-based JSON
- Suitable for turn-based and 120ms-tick games

### Why JSON?
- Human-readable for debugging
- No schema compilation needed
- Text-based for inspection
- Trade-off: slightly larger than binary protocols

### Why Centralized Server?
- Eliminates cheating (client can't modify state)
- Authoritative source of truth
- Easier to implement for turn-based/grid-based games
- Fair for all players

### Why 4-Player Limit?
- Reasonable for local multiplayer games
- Simplifies starting position design
- Performance considerations
- Can be increased by modifying MAX_PLAYERS constant

## Next Steps

- **New to project?** Read [Launcher & Game Discovery](04_LAUNCHER_AND_DISCOVERY.md)
- **Want to add a game?** See [Adding New Games](07_ADDING_NEW_GAMES.md)
- **Diving into code?** Check [Server & Networking](02_SERVER_AND_NETWORKING.md)
- **Need protocol details?** Reference [Game Protocol Specification](03_GAME_PROTOCOL.md)
