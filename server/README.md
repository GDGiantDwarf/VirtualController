# Game Server

TCP server that handles multiplayer game logic independently from clients.

## Building

```powershell
cd server/build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Running

```powershell
# Default port (8765)
.\server\build\bin\Release\GameServer.exe

# Custom port
.\server\build\bin\Release\GameServer.exe 9000
```

## Protocol (JSON over TCP)

**Client → Server**:
```json
{"type": "input", "playerId": 0, "direction": 0}
```

**Server → Client**:
```json
{
  "type": "state",
  "active": true,
  "players": [{"id": 0, "alive": true, "dir": 3, "score": 50, "body": [{"x": 10, "y": 10}]}],
  "food": [{"x": 25, "y": 15}]
}
```

## Implementation

- Single-threaded game loop (120ms tick)
- Non-blocking TCP sockets
- Up to 4 simultaneous players
- Automatic initialization on first connection

## Source Structure

```
server/src/
├── main.cpp          # Entry point
├── GameServer.cpp    # TCP server, connection management
├── GameLogic.cpp     # Game rules, state updates
├── Connection.cpp    # Per-client handling
└── Protocol.h        # Shared message definitions
```
