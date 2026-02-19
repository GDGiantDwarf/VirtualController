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

## Running Automated Tests

VirtualController includes a comprehensive test suite covering protocol validation, networking, and game logic.

### Quick Test (5 minutes)

Run protocol validation tests only:

```bash
cd d:\GitHub\VirtualController
python tests\test_protocol.py -v
```

**Expected Output**:
```
Ran 36 tests in 0.029s
OK
```

This verifies all JSON protocol messages are properly formatted and validated.

### Network Tests (Requires Active Server)

Test client-server communication:

```bash
# Terminal 1: Start GameServer (listens on port 8765 by default)
.\build\bin\Release\GameServer.exe

# Terminal 2: Run networking tests
python tests\test_networking.py -v
```

**Note**: GameServer uses port **8765** by default. To use a different port:
```bash
.\build\bin\Release\GameServer.exe 9000    # Use port 9000 instead
```

**Tests Performed**:
- [ ] Server accepts connections on port 8765
- [ ] CONNECT message handling
- [ ] INPUT message routing
- [ ] STATE_UPDATE broadcasts
- [ ] Multiple simultaneous clients
- [ ] Connection stability
- [ ] Error handling (invalid JSON, oversized messages)

**Expected Results**:
- Connection tests: PASS
- Protocol exchange: PASS
- Multiple clients: PASS
- Negative cases: PASS (graceful rejection)

### Functional Testing (Manual)

Test end-to-end user scenarios:

#### Single-Player Test

1. Start GameServer
2. Start GameLibraryLauncher
3. Click Snake game
4. Verify lobby screen
5. Click START GAME
6. Play until game over
7. Verify final score displayed

**Validation**:
- [ ] Snake renders correctly
- [ ] Keyboard controls work (Arrow keys)
- [ ] Food spawns randomly
- [ ] Collisions detected
- [ ] Score calculated correctly
- [ ] End screen shows score

#### Multiplayer Test (2 Controllers)

1. Connect keyboard and 1 joystick
2. Start GameServer
3. Start GameLibraryLauncher
4. Click Snake game
5. Verify "2 players connected"
6. Click START GAME
7. Use keyboard for Player 0, joystick for Player 1
8. Play to completion

**Validation**:
- [ ] 2 snakes render with different colors
- [ ] Each snake responds to correct input device
- [ ] Different starting positions
- [ ] Both snakes interact correctly (collisions)
- [ ] Final scores reflect both players
- [ ] Back to Start works

#### Network Multiplayer Test (2 PCs)

1. Configure network (same subnet required)
2. PC-A: Start GameServer (listens on 8765 by default)
   ```bash
   .\build\bin\Release\GameServer.exe
   ```
3. PC-A: Start GameLibraryLauncher (server: 127.0.0.1:8765)
4. PC-B: Start GameLibraryLauncher (server: PC-A IP:8765)
5. Both click Snake game
6. Verify player counts sync (should show 2 on each screen)
7. Both click START GAME
8. Play to completion

**Validation**:
- [ ] Network connection stable
- [ ] Player counts synchronized
- [ ] Game state synced across network
- [ ] Input latency < 200ms local
- [ ] Final scores match on both PCs

### Test Suite Organization

**File**: `tests/`

```
tests/
├── test_protocol.py      # 36 unit tests for JSON validation
├── test_networking.py    # Connectivity and protocol exchange tests
└── test_e2e_scenarios.md # Manual E2E test procedures (this document)
```

### Test Results Documentation

After running tests, document results:

```markdown
# Test Results - [DATE]

## Protocol Tests
- Status: PASS (36/36)
- Duration: 0.029s
- Coverage: CONNECT, INPUT, STATE_UPDATE, START_GAME, RESET_GAME

## Networking Tests (requires server)
- Status: PASS (x tests)
- Server: ✓ Running
- Connection: ✓ Accepted
- Message Exchange: ✓ Working
- Multiple Clients: ✓ Supported

## Functional Tests
- Single-Player: ✓ PASS
- Local Multiplayer: ✓ PASS
- Network Multiplayer: ✓ PASS
- Error Scenarios: ✓ PASS

## Known Issues
- None

## Test Environment
- OS: Windows 10/11
- Python: 3.10+
- Compiler: MSVC 2022
- Build Config: Release
```

### Automated Test Execution

For CI/CD integration:

```bash
# 1. Build project
cmake --build build --config Release

# 2. Run unit tests
python tests/test_protocol.py > protocol_results.txt

# 3. Start server (for integration tests)
Start-Process -FilePath ".\build\bin\Release\GameServer.exe" -WindowStyle Hidden

# 4. Run networking tests
Start-Sleep -Seconds 2
python tests/test_networking.py > networking_results.txt

# 5. Kill server
Stop-Process -Name GameServer -Force

# 6. Check results
echo "Protocol tests:" && findstr "OK\|FAILED" protocol_results.txt
echo "Networking tests:" && findstr "OK\|FAILED" networking_results.txt
```

### Troubleshooting Test Failures

| Issue | Cause | Solution |
|-------|-------|----------|
| `ModuleNotFoundError: No module named 'json'` | Python installation issue | Use `python -m pip install --upgrade setuptools` |
| Server not accepting connections | Wrong IP/port (default: 8765) | Verify GameServer running: `netstat -an \| findstr 8765` or `netstat -ano \| findstr GameServer` |
| Protocol tests fail | JSON validation regression | Check recent changes to protocol messages in GameServer.cpp |
| Network test timeout | Server slow to respond or wrong port | Verify port 8765 is listening. Start server: `.\build\bin\Release\GameServer.exe` |
| Memory warnings in output | Not related to test failures | Safe to ignore, Python cleanup warnings |

## Next Steps

- See [Core Architecture](01_CORE_ARCHITECTURE.md) for system design
- See [Game Protocol](03_GAME_PROTOCOL.md) for message details
- See [Adding New Games](06_ADDING_NEW_GAMES.md) for extending framework
