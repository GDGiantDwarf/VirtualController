# Server & Networking

## Game Server Architecture

The game server is the authoritative center of all multiplayer games, managing game logic, connection handling, and state synchronization.

### Components

```
GameServer
├── Connection Manager
│   ├── Accept Loop (new connections)
│   ├── Message Handler (parse/process messages)
│   ├── Connection Cleanup (dead connection removal)
│   └── Player ID Mapper (local→global ID conversion)
│
├── Game Logic
│   ├── Player State (alive, position, score)
│   ├── Food Management
│   ├── Collision Detection
│   ├── Game State (LOBBY, ACTIVE, ENDED)
│   └── Tick Simulation
│
└── Network I/O
    ├── TCP Listening Socket
    ├── Non-blocking Sockets (per connection)
    ├── Message Serialization (JSON)
    └── State Broadcasting
```

## Server Startup

```cpp
GameServer server(8765);      // Create server on default port
if (server.start()) {         // Initialize TCP socket
    server.run();             // Blocking - runs accept and game loops
}
```

### Initialization Steps

1. **Socket Creation** - Listen on port 8765 (TCP)
2. **Non-blocking Mode** - Enable non-blocking I/O
3. **Thread Spawning**:
   - Accept Thread: Waits for new client connections
   - Game Thread: Runs game loop (120ms ticks)
4. **State**: Ready to accept connections

## Connection Management

### Lifecycle

```
[New Connection] 
      ↓
[Accept from listen socket]
      ↓
[Create Connection object]
      ↓
[Assign connection ID & player ID range]
      ↓
[Receive CONNECT message with controller count]
      ↓
[Assign global player IDs (0,1 | 2,3 | etc.)]
      ↓
[Initialize game if first connection]
      ↓
[Receive input/control messages]
      ↓
[Send state broadcasts]
      ↓
[Close on error or disconnect]
```

### Multi-Device Player Attribution

**Problem**: Multiple PCs with multiple controllers need unique player IDs.

**Solution**: Server-side mapping per connection.

```cpp
// Each connection stores its assigned global player IDs
class Connection {
    std::vector<int> m_playerIds;  // e.g., {0, 1} or {2, 3}
};

// When client sends input with local playerId:
int globalPlayerId = conn->getGlobalPlayerId(localPlayerId);
// Convert PC1's player 1 → 1, PC2's player 1 → 3
```

### Player ID Assignment

```
Connection 1 CONNECT: controllers=2
  └─ Assigned: players 0, 1

Connection 2 CONNECT: controllers=2
  └─ Assigned: players 2, 3

Connection 3 CONNECT: controllers=1
  └─ Assigned: player 4 (would exceed MAX_PLAYERS)
  └─ Rejected or capped
```

### Maximum Players

Controlled by `GameLogic::MAX_PLAYERS` (currently 4). Modify to increase:

```cpp
// In server/src/GameLogic.h
static constexpr int MAX_PLAYERS = 4;  // Change to 8 for more players
```

## Game Loop

Server runs at 120ms per tick (~8 updates/second).

```cpp
while (m_running) {
    // 1. Process all incoming client messages
    handleClientMessages();      // 1-5ms
    
    // 2. Every 120ms: simulate game update
    if (elapsed >= TICK_RATE) {
        m_gameLogic.applyInputs(m_pendingInputs);  // Apply pending input
        m_gameLogic.tick();                         // Physics/collisions
        broadcastGameState();                       // Send to all clients
        lastTick = now;
    }
    
    // 3. Sleep to prevent CPU spinning
    std::this_thread::sleep_for(10ms);
}
```

### Tick Sequence (Snake Game)

1. **Apply Inputs**: Set each player's direction based on recent input
2. **Move Players**: Update position based on current direction
3. **Resolve Food**: Check if snakes ate food, spawn new food
4. **Detect Collisions**: 
   - Off-grid (wall)
   - Self-collision
   - Player-player collision
5. **Update State**: Mark players as alive/dead
6. **Check End Condition**: If all dead, state→ENDED
7. **Broadcast**: Send complete state to all clients

Each tick processes ~20-50 bytes of input, outputs ~500+ bytes of state per client.

## Network Protocol

### Message Flow

```
Client                                Server
  │                                     │
  ├─ CONNECT (controller count) ───────>│
  │                                     │ Assign player IDs
  │                                     │
  │<─ STATE (game state + player count)─┤ Broadcast every 120ms
  │                                     │
  ├─ INPUT (direction) ────────────────>│
  │                                     │ Every ~100-500ms
  │                                     │
  │                                     │ (1) Apply input
  │                                     │ (2) Simulate
  │                                     │ (3) Broadcast
  │                                     │
  │<─ STATE ────────────────────────────┤
  │                                     │
  ├─ START_GAME ───────────────────────>│
  │                                     │ state: LOBBY → ACTIVE
  │                                     │
  ├─ INPUT ... INPUT ... INPUT ────────>│
  │                                     │
  │<─ STATE ... STATE ... STATE ────────┤
  │                                     │
  ├─ RESET_GAME ───────────────────────>│
  │                                     │ state: ENDED → LOBBY
  │                                     │ Reset all players
```

### Connection Class (`Connection.h`)

Represents a single client connection:

```cpp
class Connection {
public:
    // Send/Receive
    bool send(const std::string& data);        // Send JSON message
    std::string receive();                     // Receive one message
    bool isAlive() const;                      // Check if still connected
    
    // Player ID Management (multi-device support)
    void setPlayerIds(const std::vector<int>& ids);  // Assign global IDs
    int getGlobalPlayerId(int localPlayerId);        // Convert local→global
    
    // Controller tracking
    void setControllerCount(int count);
    int getControllerCount() const;
};
```

### Connection Cleanup

Dead connections are removed in `handleClientMessages()`:

```cpp
m_connections.erase(
    std::remove_if(m_connections.begin(), m_connections.end(),
        [](const std::unique_ptr<Connection>& conn) {
            return !conn->isAlive();  // Remove closed/errored connections
        }),
    m_connections.end()
);
```

When a connection dies:
- Its assigned player IDs become invalid
- Affected snakes/players freeze at last known position
- Other players continue playing
- Connection can reconnect and be assigned new IDs

## GameLogic

Implements game-specific rules (see Snake in detail).

### Interface

```cpp
class GameLogic {
public:
    // Initialize game with N players
    void init(int playerCount);
    
    // Apply pending input commands
    void applyInputs(const std::array<InputCommand, MAX_PLAYERS>& inputs);
    
    // Simulate one tick
    void tick();
    
    // Reset to initial state
    void resetGame();
    
    // Get current state (copied for broadcasting)
    Protocol::GameState getState() const;
};
```

### Game States

```
LOBBY (0)
    └─ Waiting for players
    └─ Server broadcasts "connected: N" count
    └─ Transition: START_GAME message → ACTIVE

ACTIVE (1)
    └─ Game running
    └─ 120ms ticks executing
    └─ Input processed
    └─ Transition: All players dead → ENDED

ENDED (2)
    └─ All players dead
    └─ Show final scores
    └─ Transition: RESET_GAME message → LOBBY
```

### State Fields

```cpp
struct GameState {
    std::vector<PlayerState> players;    // Per-player: id, alive, dir, body, score
    std::vector<Vec2> food;              // Food positions
    bool gameActive{false};              // Simple active flag (legacy)
    GameStateType state;                 // LOBBY, ACTIVE, or ENDED
};
```

## Message Parsing

Server parses JSON messages with simple string matching (not a full JSON parser).

```cpp
Protocol::Message parseMessage(const std::string& data) {
    if (data.find("\"type\":\"connect\"") != npos) {
        // Extract "controllers": value
        msg.type = MessageType::CONNECT;
        msg.controllerCount = extractValue("controllers", data);
    }
    else if (data.find("\"type\":\"input\"") != npos) {
        msg.type = MessageType::INPUT;
        msg.playerId = extractValue("playerId", data);
        msg.direction = extractValue("direction", data);
    }
    else if (data.find("\"type\":\"start\"") != npos) {
        msg.type = MessageType::START_GAME;
    }
    // ... etc
}
```

### Limitations
- No error handling for malformed JSON
- Simple search-based parsing (not robust)
- Consider migrating to proper JSON library for robustness

## Broadcasting State

Server sends complete game state to all connected clients every 120ms.

```cpp
void broadcastGameState() {
    Protocol::GameState state = m_gameLogic.getState();
    int playerCount = state.players.size();
    
    std::string json = serializeGameState(state, playerCount);
    
    for (auto& conn : m_connections) {
        conn->send(json);  // Each connection gets full state
    }
}
```

### Serialization Example

```json
{
  "type": "state",
  "active": true,
  "connected": 2,
  "state": 1,
  "players": [
    {
      "id": 0,
      "alive": true,
      "dir": 3,
      "score": 120,
      "body": [
        {"x": 30, "y": 20},
        {"x": 29, "y": 20},
        {"x": 28, "y": 20}
      ]
    },
    {
      "id": 1,
      "alive": false,
      "dir": 1,
      "score": 85,
      "body": [...]
    }
  ],
  "food": [
    {"x": 40, "y": 15},
    {"x": 25, "y": 30}
  ]
}
```

## Extending the Server

### Adding Message Types

1. Add to `Protocol::MessageType` enum:
   ```cpp
   enum class MessageType {
       // ... existing types
       MY_NEW_MESSAGE
   };
   ```

2. Add parsing in `parseMessage()`:
   ```cpp
   if (data.find("\"type\":\"mytype\"") != npos) {
       msg.type = MessageType::MY_NEW_MESSAGE;
   }
   ```

3. Add handler in `handleClientMessages()`:
   ```cpp
   else if (msg.type == Protocol::MessageType::MY_NEW_MESSAGE) {
       // Handle custom message
   }
   ```

### Adding Game-Specific Features

Modify `GameLogic`:
- Override `init()` for custom player placement
- Extend `tick()` for new mechanics
- Add fields to `PlayerState` for custom data

Example: Adding power-ups to Snake:
```cpp
struct PowerUp {
    Vec2 position;
    int type;  // 0=speed, 1=shield, etc.
};

class GameLogic {
    std::vector<PowerUp> m_powerups;
    // Add collision detection and effects in tick()
};
```

## Troubleshooting

### Server Won't Start
- Port already in use: Try a different port
- Firewall blocking: Add exception for port 8765
- Socket creation failed: Insufficient permissions

### Client Connects but Freezes
- Server not ticking: Check TICK_RATE is running
- Bandwidth issues: Network congestion
- Client parsing error: Compare JSON format

### Players Controlling Wrong Snake
- Player ID assignment bug: Check `getGlobalPlayerId()` mapping
- Connection disconnect: Player IDs freed unexpectedly
- Duplicate player IDs: Server state corruption

See [Debugging & Testing](09_DEBUGGING_AND_TESTING.md) for network monitoring.
