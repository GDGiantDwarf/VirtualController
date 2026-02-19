# VirtualController Documentation Overview

This documentation covers the entire VirtualController project, a multi-player virtual controller system with a networked game server and launcher.

## Documentation Structure

### 1. **[Core Architecture](01_CORE_ARCHITECTURE.md)**
   - System overview and component interactions
   - Multiplayer game flow
   - Build system and directory structure
   - Contribution areas

### 2. **[Server & Networking](02_SERVER_AND_NETWORKING.md)**
   - Game server design and implementation
   - TCP protocol and message handling
   - Player ID management and connection mapping
   - Game state synchronization

### 3. **[Game Protocol Specification](03_GAME_PROTOCOL.md)**
   - JSON message format
   - Message types (CONNECT, INPUT, STATE_UPDATE, etc.)
   - Direction encoding
   - Game state structures

### 4. **[Launcher & Game Discovery](04_LAUNCHER_AND_DISCOVERY.md)**
   - Game launcher UI and features
   - Game scanner implementation
   - Game discovery from build directory
   - Icon and metadata handling

### 5. **[Snake Game Implementation](05_SNAKE_GAME_IMPLEMENTATION.md)**
   - Snake-specific game mechanics
   - Client architecture (rendering, input, networking)
   - Game state machine (LOBBY → ACTIVE → ENDED)
   - Starting positions and player colors

### 6. **[Adding New Games](06_ADDING_NEW_GAMES.md)**
   - Game template and structure
   - Networking client integration
   - Building and compilation
   - Icon requirements
   - Testing with multiple players

### 7. **[Build System](07_BUILD_SYSTEM.md)**
   - CMake configuration
   - Unified build directory structure
   - Build commands for each component
   - Dependencies and compilation flags

### 8. **[Debugging & Testing](08_DEBUGGING_AND_TESTING.md)**
   - Debug proxy for network monitoring
   - Testing multi-platform connections
   - Common issues and solutions
   - Performance profiling tips

## Quick Navigation

- **Just want to add a game?** → Start with [Adding New Games](06_ADDING_NEW_GAMES.md)
- **Working on the server?** → See [Server & Networking](02_SERVER_AND_NETWORKING.md)
- **Improving the launcher?** → Check [Launcher & Game Discovery](04_LAUNCHER_AND_DISCOVERY.md)
- **Understanding the system?** → Read [Core Architecture](01_CORE_ARCHITECTURE.md) first

## Project Statistics

- **Languages**: C++17, CMake, Python (debug tools)
- **Main Dependencies**: Qt 6.10+, SFML 3.0+, vcpkg
- **Components**: Launcher, Server, Game Client(s)
- **Max Players**: 4 simultaneous
- **Server Tick Rate**: 120ms per game update
- **Platform**: Windows 10/11 (64-bit)

## Key Concepts

- **Multiplayer**: TCP-based client-server architecture
- **Portability**: Unified build directory with no external dependencies
- **Extensibility**: Easy to add new games following the protocol
- **Network Protocol**: JSON over TCP for simplicity and debugging
- **Game State Machine**: LOBBY → ACTIVE → ENDED states
- **Player Attribution**: Per-connection player ID mapping for multi-device scenarios

## Contact & Contributing

For questions or contributions, refer to the specific documentation sections for your area of interest.
