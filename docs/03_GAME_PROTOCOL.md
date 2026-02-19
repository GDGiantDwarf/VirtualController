# Game Protocol Specification

Complete specification of the JSON-based TCP protocol for all client-server communication.

## Protocol Overview

- **Transport**: TCP/IP (default port 8765)
- **Format**: Text-based JSON, one message per line (newline-terminated)
- **Direction**: Bidirectional (client→server and server→client)
- **Encoding**: UTF-8 ASCII
- **Connection**: Persistent TCP connection, multiple messages per session

## Message Types

### 1. CONNECT (Client → Server)

Sent once when client first connects to report local input capabilities.

```json
{"type": "connect", "controllers": 2}
```

**Fields:**
- `type` (string): Always `"connect"`
- `controllers` (int): Number of input devices on this PC (keyboard + joysticks)

**Server Response:**
- Assigns unique global player IDs to this connection
- Example: connection 1 gets [0,1], connection 2 gets [2,3]
- Initializes game state if in LOBBY

**Example Scenarios:**
```json
// Single PC with keyboard only
{"type": "connect", "controllers": 1}

// PC with keyboard + 1 joystick
{"type": "connect", "controllers": 2}

// PC with keyboard + 2 joysticks
{"type": "connect", "controllers": 3}
```

### 2. INPUT (Client → Server)

Sent whenever player presses movement keys (up/down/left/right).

```json
{"type": "input", "playerId": 0, "direction": 3}
```

**Fields:**
- `type` (string): Always `"input"`
- `playerId` (int): Local player index (0-3)
- `direction` (int): Movement direction (see Direction Encoding)

**Server Processing:**
1. Convert `playerId` to global ID using connection's mapping
2. Queue input command
3. Apply on next tick (120ms later)
4. Invalid `playerId` is ignored

**Frequency:** Sent on key press or joystick change, throttled to ~100ms minimum to avoid flooding.

**Direction Encoding:**
```
0 = Up (↑)
1 = Down (↓)
2 = Left (←)
3 = Right (→)
```

**Example Sequence:**
```json
{"type": "input", "playerId": 0, "direction": 3}  // Player 0 moves right
{"type": "input", "playerId": 1, "direction": 0}  // Player 1 moves up
{"type": "input", "playerId": 0, "direction": 1}  // Player 0 changes to down
```

### 3. STATE_UPDATE (Server → Client)

Sent every 120ms (one per game tick) containing complete game state.

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
      "score": 245,
      "body": [
        {"x": 30, "y": 20},
        {"x": 29, "y": 20},
        {"x": 28, "y": 20}
      ]
    }
  ],
  "food": [
    {"x": 40, "y": 15},
    {"x": 25, "y": 30}
  ]
}
```

**Fields:**
- `type` (string): Always `"state"`
- `active` (boolean): Game running (legacy field, use `state` field)
- `connected` (int): Total number of players in game
- `state` (int): Game state (0=LOBBY, 1=ACTIVE, 2=ENDED)
- `players` (array): Array of player objects
  - `id` (int): Player ID (0-3)
  - `alive` (boolean): Player still in game
  - `dir` (int): Current direction (not necessarily movement direction)
  - `score` (int): Current score
  - `body` (array): Array of body segments (head first)
    - `x` (int): X coordinate (0-59 for snake)
    - `y` (int): Y coordinate (0-39 for snake)
- `food` (array): Array of food positions
  - `x` (int): X coordinate
  - `y` (int): Y coordinate

**Game State Values:**
```
0 = LOBBY (waiting for players, show START button)
1 = ACTIVE (game running, hide START button)
2 = ENDED (all players dead, show BACK TO START button)
```

**Delivery:** Sent to ALL connected clients simultaneously via broadcasting.

**Example State Progression:**

**Lobby State:**
```json
{"type": "state", "active": false, "connected": 2, "state": 0, "players": [...empty...], "food": []}
```

**Active State (game running):**
```json
{"type": "state", "active": true, "connected": 2, "state": 1, "players": [{id: 0, alive: true, ...}], "food": [{x: 25, y: 15}]}
```

**Ended State:**
```json
{"type": "state", "active": false, "connected": 2, "state": 2, "players": [{id: 0, alive: false, score: 250, ...}], "food": []}
```

### 4. START_GAME (Client → Server)

Signal to transition from LOBBY to ACTIVE state.

```json
{"type": "start"}
```

**Fields:**
- `type` (string): Always `"start"`

**Server Response:**
- If state is LOBBY:
  - Set state → ACTIVE
  - Initialize players at starting positions
  - Clear all pending inputs
  - Begin 120ms tick cycle
- If state is ACTIVE or ENDED:
  - Ignore (no effect)

**When Sent:**
- When user clicks "START GAME" button on lobby screen (from any client)
- First client to click "START" triggers for all

**Example:**
```json
{"type": "start"}
```

### 5. RESET_GAME (Client → Server)

Signal to transition from ENDED back to LOBBY state.

```json
{"type": "reset"}
```

**Fields:**
- `type` (string): Always `"reset"`

**Server Response:**
- If state is ENDED:
  - Set state → LOBBY
  - Call `gameLogic.init()` with current player count
  - Reset all scores to 0
  - Clear all pending inputs
  - Broadcast new LOBBY state
- If state is LOBBY or ACTIVE:
  - Ignore

**When Sent:**
- When user clicks "BACK TO START" button on end screen (from any client)
- Resets game back to beginning without disconnecting

**Example:**
```json
{"type": "reset"}
```

## Protocol Sequences

### Connection Initialization

```
Client                                                 Server
  │
  ├─ TCP connect ───────────────────────────────────────>│
  │                                                      │
  ├─ {"type":"connect","controllers":2} ────────────────>│
  │                                                      │ Assign player IDs [0,1]
  │                                                      │ Initialize GameLogic(2)
  │                                                      │
  │<─ {"type":"state",...} ──────────────────────────────┤ LOBBY state
  │                                                      │
```

### Full Game Sequence

```
[LOBBY PHASE]
Client 1: {...}
Client 2: {...}
          {"type":"start"} ─────────────────────────────>│
Server:                                                  │ state: LOBBY → ACTIVE
          {"type":"state",...} <─────────────────────────┤ Broadcast ACTIVE state

[ACTIVE PHASE - repeats every 120ms]
Client 1: {"type":"input","playerId":0,"direction":3} ──>│
Client 2: {"type":"input","playerId":0,"direction":0} ──>│
Server:                                                  │ Simulate tick
          {"type":"state",...} <─────────────────────────┤ All players/food
          
[ACTIVE PHASE continues...]

[On all players dead]
Server:   {"type":"state","state":2,...} <───────────────┤ Broadcast ENDED

[END PHASE]
Client 1: {"type":"reset"} ─────────────────────────────>│
Server:   {"type":"state","state":0,...} <───────────────┤ Return to LOBBY
```

## Data Structures

### Player Structure

Represents one player in the game.

```
{
  "id": 0,              // Player ID (0-3)
  "alive": true,        // Still in game
  "dir": 3,             // Current direction (0=Up, 1=Down, 2=Left, 3=Right)
  "score": 150,         // Total score
  "body": [             // Snake body (head-first order)
    {"x": 30, "y": 20}, // Head
    {"x": 29, "y": 20},
    {"x": 28, "y": 20}  // Tail
  ]
}
```

### Food Structure

Position of one food item.

```
{
  "x": 25,  // X coordinate
  "y": 15   // Y coordinate
}
```

### Game Coordinates

Grid-based coordinates for Snake game:
- X range: 0-59 (60 columns)
- Y range: 0-39 (40 rows)
- Each cell is one game unit
- Top-left is (0,0)

## Message Size & Bandwidth

Typical message sizes:

| Message | Size | Frequency |
|---------|------|-----------|
| CONNECT | ~40 bytes | Once per connection |
| INPUT | ~50 bytes | ~10-100ms (user input) |
| STATE (2 players) | ~400 bytes | Every 120ms |
| STATE (4 players) | ~800 bytes | Every 120ms |

**Bandwidth Usage (4 players):**
- Input: ~50 inputs/sec × 50 bytes = ~2.5 KB/s per client
- State: 8.3 updates/sec × 800 bytes = ~6.6 KB/s per client
- **Total**: ~9 KB/s per client (bidirectional)

## Error Handling

Current protocol has minimal error handling:
- Invalid JSON: Message ignored
- Unknown message type: Message ignored
- Out-of-range playerId: Input ignored
- Invalid direction: Direction ignored (no validation)

**Future Improvements:**
- Error messages in protocol
- ACK/NAK for critical messages
- Message validation and sanitization
- Timeout handling for stuck clients

## Protocol Evolution

To extend the protocol:

1. **Add new message type:**
   - Add to enum in `Protocol.h`
   - Add parsing in `GameServer::parseMessage()`
   - Add handler in `GameServer::handleClientMessages()`
   - Document here

2. **Add fields to STATE:**
   - Add to `struct GameState` in `Protocol.h`
   - Update serialization in `serializeGameState()`
   - Update client parsing in `parseGameState()`

3. **Backward Compatibility:**
   - Optional new fields should have defaults
   - Old clients ignore unknown fields
   - Server handles both old and new formats

## Debugging Protocol

### Using Debug Proxy

Run `debug_proxy.py` to see all messages:

```python
# Shows all JSON traffic in real-time
python debug_proxy.py
```

### Manual Testing

Send commands with netcat:

```bash
nc localhost 8765
{"type":"connect","controllers":1}
# Watch responses...
```

### Common Issues

**Server not sending state:**
- Check TICK_RATE is running (120ms)
- Verify game loop thread created
- Check for exceptions

**Input not affecting game:**
- Verify INPUT message format
- Check playerId is in valid range
- Verify direction is 0-3

**Wrong player controls wrong snake:**
- Check connection→global ID mapping
- Verify CONNECT assigned correct player IDs
- Check INPUT uses correct playerId
