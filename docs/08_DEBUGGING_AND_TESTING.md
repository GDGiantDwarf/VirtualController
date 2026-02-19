# Debugging and Testing

Complete guide to monitoring, debugging, and testing the VirtualController system.

## Network Monitoring

### Debug Proxy Script (debug_proxy.py)

Real-time HTTP/TCP traffic monitor for analyzing game protocol messages.

**Location**: Root directory
**Usage**: Must be run before starting GameServer

```bash
python debug_proxy.py
```

#### Setup

First time only:
```bash
# Install dependencies
pip install pyquery  # or: python -m pip install pyquery

# Run proxy
python debug_proxy.py
```

#### How It Works

1. Proxy starts listening on localhost:9001
2. Game clients connect to proxy instead of server
3. Proxy forwards all messages to real server (localhost:9000)
4. Proxy logs all messages to console with timestamps

#### Console Output Format

```
[HH:MM:SS] FROM: <client_address> TO: <server_address>
<JSON message>

[HH:MM:SS] FROM: GameServer
<JSON response>
```

#### Example Session

```bash
$ python debug_proxy.py
Starting debug proxy on localhost:9001, forwarding to localhost:9000
[14:23:45] FROM: 127.0.0.1:54321 TO: 127.0.0.1:9000
{"type": "connect", "controllers": 2}

[14:23:45] FROM: 127.0.0.1:54321 TO: 127.0.0.1:9000
{"type": "input", "playerId": 0, "direction": 1}

[14:23:46] FROM: GameServer
{"type": "state_update", "state": 1, "players": [...], "food": [...]}
```

### Connecting Through Proxy

For local testing, clients automatically connect to localhost:9000. To use proxy:

1. Edit snake.cpp:
   ```cpp
   const int PORT = 9001;  // Changed from 9000
   ```

2. Or modify connection in NetworkClient class

3. Rebuild and test

## Debugging Game Logic

### Common Issues and Solutions

#### Game Freezes on Launch

**Symptoms**: 
- Server starts but GUI unresponsive
- No error messages

**Debugging Steps**:

1. Check server is running:
   ```bash
   # Open another terminal
   netstat -an | findstr 9000
   # Should show: TCP 127.0.0.1:9000 LISTENING
   ```

2. Check network connectivity:
   ```bash
   # From game client folder
   telnet localhost 9001
   # Should connect, or show connection refused
   ```

3. Add console logging in main.cpp:
   ```cpp
   std::cout << "Connecting to server at " << HOST << ":" << PORT << std::endl;
   if (!client.connect(HOST, PORT)) {
       std::cerr << "Connection failed!" << std::endl;
       return -1;
   }
   std::cout << "Connected successfully" << std::endl;
   ```

#### Wrong Number of Snakes Appearing

**Symptoms**:
- Expected 2 snakes, only 1 appears
- Player count mismatch with controllers

**Debugging Steps**:

1. Check controller count reported by client (debug_proxy.py):
   ```
   [14:23:45] FROM: snake.exe
   {"type": "connect", "controllers": 2}
   ```

2. Check assignment in server logs - add to GameServer::onConnect():
   ```cpp
   std::cout << "Assigned player IDs: ";
   for (int id : playerIds) std::cout << id << " ";
   std::cout << std::endl;
   ```

3. Check game state includes all players:
   ```cpp
   std::cout << "GameLogic has " << state.players.size() << " players" << std::endl;
   ```

#### Controllers Not Responding

**Symptoms**:
- Keyboard works, joystick doesn't
- Snake freezes midgame

**Debugging Steps**:

1. Check joystick connection:
   ```cpp
   // In InputAdapter constructor
   for (int i = 0; i < sf::Joystick::Count; ++i) {
       if (sf::Joystick::isConnected(i)) {
           std::cout << "Joystick " << i << " connected" << std::endl;
       }
   }
   ```

2. Monitor input events:
   ```cpp
   std::cout << "Input from player " << playerId 
             << ": direction " << direction << std::endl;
   ```

3. Check input reaches server (debug_proxy.py shows INPUT messages)

4. Verify no duplicate direction commands:
   ```cpp
   // In GameServer::onInput()
   if (globalPlayerId < state.players.size()) {
       std::cout << "Applied direction " << direction 
                 << " to player " << globalPlayerId << std::endl;
   }
   ```

#### Game Won't Start (Stuck on Lobby)

**Symptoms**:
- START button visible but non-responsive
- Player count shows 0

**Debugging Steps**:

1. Check server received connect messages:
   - Run debug_proxy.py, should show:
   ```
   {"type": "connect", "controllers": N}
   ```

2. Check START button click handled:
   ```cpp
   std::cout << "Button clicked at (" << x << ", " << y << ")" << std::endl;
   ```

3. Verify START_GAME message sent:
   ```
   [14:23:50] FROM: GameLibraryLauncher
   {"type": "start_game"}
   ```

#### Server Shows Wrong Scores

**Symptoms**:
- End screen displays incorrect scores
- Players switched

**Debugging Steps**:

1. Check player ID assignments match:
   ```cpp
   // In GameLogic::update()
   for (int i = 0; i < players.size(); ++i) {
       std::cout << "Player " << i << " score: " 
                 << players[i].score << std::endl;
   }
   ```

2. Verify no re-indexing during reset:
   ```cpp
   // In RESET_GAME handler
   std::cout << "Resetting " << players.size() << " players" << std::endl;
   ```

## Testing Scenarios

### Single Player Local

**Setup**:
1. Start GameServer
2. Start GameLibraryLauncher
3. Connect 1 controller to PC
4. Click on Snake game

**Expected**:
- One snake appears
- Controlled by connected controller or keyboard
- Game runs smoothly at ~60 FPS

**Validation Checklist**:
- [ ] Snake responds to input immediately
- [ ] Game loop stable (no stuttering)
- [ ] End screen appears when snake dies
- [ ] BACK TO START button works

### Local Multiplayer (2 Controllers)

**Setup**:
1. Start GameServer
2. Start GameLibraryLauncher
3. Connect 2 joysticks to same PC
4. Click on Snake game

**Expected**:
- Lobby shows "2 players connected"
- Two snakes appear in opposite corners
- Can control both snakes simultaneously
- Colors are different per player

**Validation Checklist**:
- [ ] Correct player count displayed
- [ ] Both snakes visible and responsive
- [ ] No overlap of starting positions
- [ ] Different colors per snake
- [ ] End game shows both scores

### Network Multiplayer (2 PCs)

**Setup**:
1. Start GameServer on PC-A (local network IP: 192.168.1.5)
2. Start GameLibraryLauncher on PC-A
3. Start GameLibraryLauncher on PC-B
4. On PC-B, manually connect to 192.168.1.5:9000
5. Both click Snake game

**Expected**:
- Four snakes appear (2 per PC)
- All 4 controlled independently
- Collisions work across network
- Synchronization is smooth (< 1s latency)

**Validation Checklist**:
- [ ] Network connection established
- [ ] All 4 snakes visible
- [ ] Correct player count on both launchers
- [ ] Different colors per player
- [ ] No duplicate player indices
- [ ] End game synced across network

### Edge Cases

#### Rapid Start/Stop

**Test**: Click START, then immediately BACK TO START

**Expected**:
- Game ends cleanly
- No crash or hang

**Validation**:
- [ ] Returns to lobby
- [ ] State is clean for next game

#### Disconnect During Game

**Test**: Unplug network cable or quit client mid-game

**Expected**:
- Server continues (doesn't crash)
- Remaining players complete game normally
- Disconnected player's snakes frozen on screen temporarily

**Validation**:
- [ ] Other players can reach end screen
- [ ] Server logs connection loss gracefully

#### Multiple Game Launches

**Test**: 
1. Play game and end it
2. Play again immediately
3. Repeat 3+ times

**Expected**:
- All games work identically
- No memory leaks (check memory usage)
- No resources exhausted

**Validation**:
- [ ] Each game plays to completion
- [ ] Snake always resets to same starting position
- [ ] No slowdown across multiple games

## Profiling and Performance

### Frame Rate Monitoring

Add to renderer (snake.cpp drawState):

```cpp
static int frameCount = 0;
static Clock clock;
frameCount++;

if (clock.getElapsedTime().asSeconds() >= 1.0f) {
    std::cout << "FPS: " << frameCount << std::endl;
    frameCount = 0;
    clock.restart();
}
```

**Expected**:
- Rendering: 60 FPS
- Server ticks: 8-10 updates/second (120ms)
- No frame drops

### Network Latency Measurement

Modify NetworkClient to timestamp messages:

```cpp
// In GameClient.onStateUpdate()
auto receiveTime = std::chrono::system_clock::now();
double latency_ms = std::chrono::duration<double, std::milli>(
    receiveTime - message.sentTime).count();
std::cout << "Latency: " << latency_ms << " ms" << std::endl;
```

**Expected**:
- Local: < 5 ms
- Network: 20-100 ms depending on connection

### Memory Usage

Monitor with Visual Studio or Task Manager:

**Expected**:
- GameServer: < 50 MB
- GameLibraryLauncher: < 100 MB
- Snake game: < 80 MB

**After 10 games**:
- Memory should not increase
- Indicates no memory leaks

## Stress Testing

### Maximum Players

**Test**: Connect many players sequentially

```bash
# Run game client multiple times with different IDs
for i in 1 2 3 4 5; do
    start snake.exe  # Runs in background
done
```

**Expected**:
- Supports at least 4 players per PC
- Server broadcasts to all without latency increase
- No crash with 8+ total players

**Validation**:
- [ ] All 8 snakes visible and responding
- [ ] Game complete time reasonable (< 2 minutes)

### Message Flood

**Test**: Send many input messages rapidly

```cpp
// Modify InputAdapter to send every tick instead of on change
while (running && !window.isDone()) {
    client.sendInput(player, direction);  // Even if unchanged
}
```

**Expected**:
- Server handles gracefully
- Network doesn't saturate
- No crashes

**Validation**:
- [ ] Game remains playable
- [ ] Server message processing keeps up

### Long Game Duration

**Test**: Run game for extended period (avoid deaths)

**Expected**:
- FPS remains constant
- Memory stable
- No network timeouts

**Validation**:
- [ ] Can play uninterrupted for 30+ minutes
- [ ] No gradual slowdown
- [ ] Clean shutdown

## Common Test Failures

### Flaky Tests (Intermittent Failures)

**Network timeouts**:
- Check firewall settings
- Verify network stability
- Test on different network

**Input lag on multiplayer**:
- Check server running on same network segment
- Measure actual latency with debug_proxy.py
- Reduce visual effects if CPU bound

**Random crashes**:
- Run under debugger to get stack trace
- Check for uncaught exceptions
- Validate all pointer dereferences

### Build Verification

Before testing, verify build:

```bash
# Check all executables exist
ls build/bin/Release/
# Expected output:
#   GameLibraryLauncher.exe
#   GameServer.exe
#   snake.exe
#   *.dll files

# Verify icons present
ls build/bin/Release/*.ico
# Expected: snake.ico
```

## Debugging with Visual Studio

### Attach Debugger to Running Game

1. Start game from command line
2. In Visual Studio: Debug → Attach to Process
3. Select `snake.exe`
4. Set breakpoints in code
5. Interact with game to hit breakpoints

### Breakpoint Strategy

Set breakpoints at:
- Network message handlers (examine JSON)
- Game state updates (verify scores)
- Input processing (check direction values)
- Rendering (profile performance)

### Watch Variables

Monitor during execution:
- `state.players[i].x`, `state.players[i].y` (position)
- `state.food.x`, `state.food.y` (food position)
- `inputAdapter.directions[]` (controller inputs)
- `client.isConnected` (network status)

## Logging Best Practices

### Structured Logging

Use consistent format:

```cpp
std::cout << "[GAME] Snake 0 ate food at (25, 30), score now 10" << std::endl;
std::cout << "[NET] Received STATE_UPDATE from server" << std::endl;
std::cout << "[ERROR] Connection lost to server" << std::endl;
```

Categories: `[GAME]`, `[NET]`, `[RENDER]`, `[INPUT]`, `[ERROR]`

### JSON Pretty Print

Make debug_proxy output readable:

```python
# In debug_proxy.py
import json
print(json.dumps(message, indent=2))
```

### Performance Markers

Add timing:

```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... code to measure ...
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "[PERF] Operation took " << duration.count() << " ms" << std::endl;
```

## Continuous Testing Checklist

Before each commit, verify:

- [ ] All components compile without warnings
- [ ] Local single-player game completes
- [ ] Local multiplayer (2 controllers) completes
- [ ] Network multiplayer connects and plays
- [ ] End screen shows correct scores
- [ ] Back to lobby works
- [ ] Repeated games (3+) all work
- [ ] FPS stable at 60
- [ ] Memory doesn't leak
- [ ] No console errors in debug_proxy

## Next Steps

- See [Core Architecture](01_CORE_ARCHITECTURE.md) for system design
- See [Game Protocol](03_GAME_PROTOCOL.md) for message details
- See [Adding New Games](06_ADDING_NEW_GAMES.md) for extending framework
