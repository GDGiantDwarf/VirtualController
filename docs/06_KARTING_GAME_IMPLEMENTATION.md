# Karting Game Implementation

Detailed walkthrough of how the Karting game is implemented using the VirtualController framework with server-authoritative physics and lap-based racing.

## Overview

Karting is a racing game where up to 4 players compete on a track to complete the most laps first. Unlike Snake's turn-based grid movement, Karting uses continuous analog physics with real-time collision detection via pixel-based track analysis.

| Property | Value |
|----------|-------|
| **Track Type** | Pixel-based collision detection (image-based) |
| **Max Players** | 4 simultaneous |
| **Tick Rate** | 30ms (33 updates/sec) - *Higher than Snake for smooth physics* |
| **Lap Goal** | 3 laps to finish |
| **Input Model** | Analog (throttle -1..1, steer -1..1) - *Not discrete directions* |
| **Win Condition** | First to complete 3 laps |
| **Colors** | Red (P1), Blue (P2), Orange (P3), Yellow (P4) |
| **Collision Detection** | Server-side, SFML image pixel analysis |

## Game Rules

### Movement & Physics
- Players control cars with analog input:
  - **Throttle**: Forward (1.0) / Reverse (-1.0) / Idle (0.0)
  - **Steer**: Right (1.0) / Left (-1.0) / Straight (0.0)
- Each tick updates:
  - Car velocity (friction-applied)
  - Car rotation (steering increments rotation)
  - Car position (velocity moves car forward/backward in rotation direction)
  - Maximum speed: 300 units/sec
  - Acceleration: 200 units/sec²
  - Steering speed: 3°/frame

### Track Collision Detection

Server-side collision detection uses **SFML image pixel analysis**:
- **Track Image**: `sprites/track.png` (loaded headlessly on server)
- **Grass Pixels** (RGB: 0, 128, 0): Off-track → friction penalty
- **Road Pixels** (any other color): On-track → normal physics
- **Yellow Pixels**: Finish line marker → lap counter increment

When a car drives on grass:
- Speed reduced by friction coefficient (0.8x)
- Car loses control responsiveness
- Yellow pixel detection triggers lap completion on **finish line crossing**

### Lap Completion

Lap detection is **pixel-precise**:
1. Server loads `assets/track.png` at startup
2. Each frame, checks if car position overlaps yellow finish line pixels
3. Detects **edge crossing**: `wasOnFinishLine=false` → `nowOnFinishLine=true`
4. Increments `lapsCompleted` counter
5. At 3 laps: Sets `finishedRace=true`, moves to finish order queue

### Win Condition
- First player to 3 laps wins
- Game transitions to ENDED state
- Final finish order displayed on screen
- Players can click "BACK TO START" to reset and replay

## Startup Positions

Configured in `GameLogic::spawnCars()` - cars spawn at predefined track positions:

```cpp
std::array<Protocol::Vec2, 4> spawnPoints = {
    Protocol::Vec2{577.0f, 533.0f},  // Player 0
    Protocol::Vec2{503.0f, 574.0f},  // Player 1
    Protocol::Vec2{431.0f, 533.0f},  // Player 2
    Protocol::Vec2{357.0f, 574.0f}   // Player 3
};
```

These positions are **baked into the game logic** (not derived from image analysis). Spawned in `init()` so cars exist in lobby.

## Architecture

### File Structure

```
server/karting/
├── CMakeLists.txt              # Build with SFML::Graphics
├── src/
│   ├── Protocol.h              # Car state structs, input command
│   ├── GameServer.h/cpp        # TCP server, connection management
│   ├── GameLogic.h/cpp         # Physics, collision detection
│   └── main.cpp                # Entry point

games/karting/
├── CMakeLists.txt              # Client build
├── karting.cpp                 # Single-file game (900 lines)
├── sprites/
│   ├── track.png               # Track image for rendering
│   └── car.png                 # Car sprite
└── karting.ico                 # Game icon
```

### Server-Side Code Organization

```cpp
// Protocol.h
struct CarState {
    int playerId;
    Vec2 position;
    Vec2 velocity;
    float rotation;
    float speed;
    int lapsCompleted;
    bool finishedRace;
};

struct InputCommand {
    int playerId;
    float throttle;              // -1.0 to 1.0
    float steer;                 // -1.0 to 1.0
};

// GameLogic.cpp (270 lines)
class GameLogic {
    static constexpr float ACCELERATION = 200.0f;
    static constexpr float MAX_SPEED = 300.0f;
    static constexpr float ROTATION_SPEED = 3.0f;
    static constexpr float FRICTION = 0.98f;
    
    // Core methods
    init(int playerCount)        // Create cars at spawn points
    startGame()                  // Reset state, mark active
    applyInputs()                // Update velocity/rotation from input
    tick()                       // Update position, collision, lap detection
    
    // Collision detection
    isOnTrack(Vec2 pos)          // Pixel-based grass check
    checkLapCompletion()         // Finish line crossing detection
};

// GameServer.cpp
void gameLoop() {
    const auto tickDuration = 30ms;  // TICK_RATE = 0.03f
    
    while (running) {
        if (now - lastTick >= tickDuration) {
            gameLogic.applyInputs(pendingInputs);
            gameLogic.tick();
            broadcastGameState();  // Send all car positions
            lastTick = now;
        }
    }
}
```

### Client-Side Code Organization

```cpp
karting.cpp (900 lines)
├── InputAdapter class       // Keyboard + joystick → throttle/steer
│   ├── getInput(player)     // Returns {throttle, steer}
│   └── countConnectedControllers()
│
├── NetworkClient class      // TCP connection
│   ├── connect()            // TCP connect
│   ├── sendInput()          // Send {throttle, steer}
│   ├── sendConnect()        // Report controller count
│   ├── sendStartGame()      // Start racing
│   ├── receiveState()       // Receive all car states
│   └── parseGameState()     // JSON → GameState
│
├── Renderer class           // Drawing
│   ├── init()               // Load sprites
│   ├── drawState()          // Track + cars + HUD
│   ├── drawLobby()          // Player count + START button
│   ├── drawEndScreen()      // Finish order + BACK button
│   └── getTrackSize()       // Used for window sizing
│
└── main()
    ├── Connect to server
    ├── Load track sprite (for window size)
    ├── Game loop @ 60 FPS
    │   ├── Handle events
    │   ├── Send input every 100ms
    │   ├── Receive state every frame
    │   └── Render based on server state
    └── Cleanup
```

## Game Loop

Client runs at **60 FPS**, server updates at **33 Hz**. Important difference from Snake:
- **Snake**: Turn-based, can work with slower server (120ms)
- **Karting**: Continuous physics, needs faster server (30ms) to feel responsive

### Server Game Loop

```cpp
void GameServer::gameLoop() {
    auto lastTick = steady_clock::now();
    const auto tickDuration = chrono::milliseconds(30);
    
    while (m_running) {
        auto now = steady_clock::now();
        
        // Process all client messages
        handleClientMessages();
        
        // Physics update every 30ms
        if (now - lastTick >= tickDuration) {
            {
                lock_guard lock(m_inputMutex);
                m_gameLogic.applyInputs(m_pendingInputs);
            }
            
            m_gameLogic.tick();        // Update positions, collisions, laps
            broadcastGameState();      // Send to all clients
            
            lastTick = now;
        }
        
        sleep_for(10ms);  // Prevent busy-waiting
    }
}
```

**Key Difference**: 30ms (not 120ms) because analog controls need frequent position updates to feel smooth.

### Client Game Loop

```cpp
while (window.isOpen()) {
    // 1. Handle window events
    while (auto e = window.pollEvent()) {
        if (e->is<sf::Event::Closed>()) window.close();
        if (e->is<sf::Event::MouseButtonPressed>()) {
            // Handle START / BACK button clicks
        }
    }
    
    // 2. Send input every 100ms (throttle/steer)
    if (inputClock.getElapsedTime().asMilliseconds() > 100) {
        for (int player = 0; player < MAX_PLAYERS; ++player) {
            auto input = InputAdapter::getInput(player);
            client.sendInput(player, input.throttle, input.steer);
        }
        inputClock.restart();
    }
    
    // 3. Receive server state @ ~33 Hz
    GameState newState = client.receiveState();
    if (newState.valid) {
        currentState = newState;
        // Update car positions, lap counts
    }
    
    // 4. Render @ 60 FPS
    if (currentState.state == 2) {      // ENDED
        Renderer::drawEndScreen(window, currentState, font, mousePos);
    } else if (currentState.state == 0) { // LOBBY
        Renderer::drawLobby(window, currentState, font, mousePos, startButtonClicked);
    } else {                              // ACTIVE (state == 1)
        Renderer::drawState(window, currentState, font);
    }
}
```

## Input Handling

### Input Adapter

Karting uses **analog input**, not discrete directions like Snake.

```cpp
class InputAdapter {
public:
    struct Input {
        float throttle{0.0f};   // -1.0 (reverse) to 1.0 (forward)
        float steer{0.0f};      // -1.0 (left) to 1.0 (right)
    };
    
    static Input getInput(int player) {
        Input input;
        
        // Keyboard (player 0)
        if (player == 0) {
            if (pressed(Up) || pressed(W))   input.throttle += 1.0f;
            if (pressed(Down) || pressed(S)) input.throttle -= 1.0f;
            if (pressed(Right) || pressed(D)) input.steer += 1.0f;
            if (pressed(Left) || pressed(A))  input.steer -= 1.0f;
        }
        
        bool keyboardActive = (abs(input.throttle) > 0.01 || abs(input.steer) > 0.01);
        
        // Joystick (analog sticks)
        if (Joystick::isConnected(player) && (player > 0 || !keyboardActive)) {
            float joyX = Joystick::getAxisPosition(player, Axis::X);
            float joyY = Joystick::getAxisPosition(player, Axis::Y);
            
            const float deadzone = 15.0f;
            if (abs(joyX) > deadzone) {
                input.steer = joyX / 100.0f;     // Normalize to -1..1
            }
            if (abs(joyY) > deadzone) {
                input.throttle = -joyY / 100.0f; // Invert Y axis
            }
        }
        
        return input;
    }
};
```

**Differences from Snake:**
- Returns `Input` struct (with throttle, steer) not `std::optional<Direction>`
- Analog values mapped from joystick axes (-100..100 → -1..1)
- Keyboard maps to analog (press = full value, release = 0)
- Joystick takes priority over keyboard per player

### Input Sending

```cpp
// Send every 100ms regardless of value change
if (inputClock.getElapsedTime().asMilliseconds() > 100) {
    for (int player = 0; player < MAX_PLAYERS; ++player) {
        auto input = InputAdapter::getInput(player);
        client.sendInput(player, input.throttle, input.steer);
    }
    inputClock.restart();
}
```

**Note**: Unlike Snake which only sends on change, Karting sends **continuously** because values are analog and smooth physics requires current state.

## Rendering

### Window Sizing

**Dynamic based on track image:**

```cpp
// Load track to get window size
Renderer::init();
sf::Vector2u trackSize = Renderer::getTrackSize();

sf::RenderWindow window(sf::VideoMode(trackSize), "Multiplayer Karting Client");
```

This automatically matches window size to track image dimensions (typically ~1000x600).

### drawState() - Active Race

```cpp
static void drawState(sf::RenderWindow& window, const GameState& state, const sf::Font& font) {
    window.clear(sf::Color(50, 120, 50));  // Green grass background
    
    // 1. Draw track
    sf::Sprite trackSprite(s_trackTexture);
    window.draw(trackSprite);
    
    // 2. Draw cars
    static std::array<sf::Color, 4> colors{
        sf::Color::Red,                    // Player 0
        sf::Color::Blue,                   // Player 1
        sf::Color(255, 165, 0),            // Player 2 (Orange)
        sf::Color::Yellow                  // Player 3
    };
    
    sf::Sprite carSprite(s_carTexture);
    sf::Vector2u carSize = s_carTexture.getSize();
    carSprite.setOrigin(carSize.x / 2.f, carSize.y / 2.f);  // Center for rotation
    
    for (const auto& c : state.cars) {
        carSprite.setPosition(c.position.x, c.position.y);
        carSprite.setRotation(sf::degrees(c.rotation));
        carSprite.setColor(colors[c.playerId % 4]);
        window.draw(carSprite);
    }
    
    // 3. Draw HUD (top-left lap counter)
    sf::Text hudText(font);
    hudText.setCharacterSize(20);
    hudText.setFillColor(sf::Color::White);
    hudText.setPosition(10.f, 10.f);
    
    std::ostringstream hud;
    for (const auto& c : state.cars) {
        hud << "P" << (c.playerId + 1) << ": " << c.lapsCompleted << " laps\n";
    }
    
    hudText.setString(hud.str());
    window.draw(hudText);
    
    window.display();
}
```

**Key Differences from Snake:**
- Sprites instead of grid cells
- Sprite rotation matches server rotation
- HUD shows lap count in real-time
- Track image drawn as background (not tiled cells)

### drawLobby() - Start Screen

```
╔══════════════════════════════════╗
║  Multiplayer Karting             ║
║                                  ║
║  Players Connected: 2 / 4        ║
║                                  ║
║     ┌──────────────────────┐     ║
║     │  START GAME          │     ║
║     └──────────────────────┘     ║
║                                  ║
║  Waiting for players...          ║
╚══════════════════════════════════╝
```

- Title: "Multiplayer Karting"
- Player count display
- START button (enabled if > 0 players)
- Hover effect (brighter green when mouse over)

### drawEndScreen() - Finish Screen

```
╔══════════════════════════════════╗
║          GAME OVER               ║
║                                  ║
║  Final Results:                  ║
║  1. Player 1 - 3 laps (Red)      ║
║  2. Player 2 - 2 laps (Blue)     ║
║  3. Player 3 - 1 laps (Orange)   ║
║                                  ║
║     ┌──────────────────────┐     ║
║     │  BACK TO START       │     ║
║     └──────────────────────┘     ║
╚══════════════════════════════════╝
```

- Displays finish order
- Shows lap count for each finisher
- Color-coded by player
- BACK TO START button triggers reset

## Networking Integration

### Protocol Differences from Snake

**Snake uses discrete directions:**
```json
{"type":"input", "direction": 0}  // 0=Up, 1=Down, 2=Left, 3=Right
```

**Karting uses analog values:**
```json
{"type":"input", "throttle": 0.5, "steer": -1.0}
```

### Initial Connection

```cpp
// Count local input devices
int controllerCount = InputAdapter::countConnectedControllers();

// Connect to server
NetworkClient client("127.0.0.1", 8766);
if (!client.connect()) {
    // Show error dialog
}

// Report how many local players this client has
client.sendConnect(controllerCount);
```

### Input Sending

```cpp
bool sendInput(int playerId, float throttle, float steer) {
    std::ostringstream oss;
    oss << "{\"type\":\"input\",\"playerId\":" << playerId
        << ",\"throttle\":" << throttle
        << ",\"steer\":" << steer << "}\n";
    
    std::string msg = oss.str();
    int result = ::send(m_socket, msg.c_str(), msg.size(), 0);
    return result > 0;
}
```

### State Reception

```cpp
GameState receiveState() {
    GameState state;
    if (!m_connected) return state;
    
    char buffer[8192];
    int result = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (result <= 0) {
        return state;  // Connection lost
    }
    
    buffer[result] = '\0';
    std::string data(buffer);
    
    state = parseGameState(data);
    state.valid = true;  // Mark successful parse
    return state;
}
```

**Note**: `valid` field ensures lobby state (with 0 cars) is still accepted as valid, unlike Snake which checked `!players.empty()`.

### Server-Side Input Processing

```cpp
void GameServer::handleClientMessages() {
    for (auto& conn : m_connections) {
        std::string message;
        while (conn->receive(message)) {
            Protocol::Message msg = parseMessage(message);
            
            if (msg.type == CONNECT) {
                // Client reported N local controllers
                // Assign global player IDs
            } else if (msg.type == INPUT) {
                // Store input in m_pendingInputs[msg.playerId]
                m_pendingInputs[msg.playerId].throttle = msg.throttle;
                m_pendingInputs[msg.playerId].steer = msg.steer;
            }
        }
    }
}
```

## Multi-Player Scenarios

### Scenario 1: 2 PCs, 1 Keyboard Each

```
PC 1 (Player 0): Keyboard → sendInput(0, throttle, steer)
PC 2 (Player 1): Keyboard → sendInput(1, throttle, steer)

Server:
- CONNECT PC1: assigned [0]
- CONNECT PC2: assigned [1]
- Receives INPUT messages from both
- Broadcasts state with 2 cars
```

### Scenario 2: 1 PC, 4 Joysticks

```
PC 1 (Local):
  - Joystick 0 → Player 0
  - Joystick 1 → Player 1
  - Joystick 2 → Player 2
  - Joystick 3 → Player 3

Server:
- CONNECT PC1: assigned [0,1,2,3]
- Receives 4 input streams per frame
- Broadcasts state with 4 cars
- All driven from single machine
```

### Scenario 3: 2 PCs, Mixed Controller Types

```
PC 1 (2 Joysticks):
  - Joystick 0 → Player 0
  - Joystick 1 → Player 1

PC 2 (Keyboard + 1 Joystick):
  - Keyboard → Player 0
  - Joystick 2 → Player 1

Server:
- CONNECT PC1: assigned [0,1]
- CONNECT PC2: assigned [2,3]
- Broadcasts 4-player race
```

## Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| **Frame Rate** | 60 FPS | Client window |
| **Server Tick** | 30ms (33 Hz) | *Faster than Snake's 120ms* |
| **Input Send** | 100ms intervals | Continuous values vs. change-only |
| **Network BW** | ~500 bytes/sec | 4 cars × 4 floats × 33 Hz |
| **Memory/Client** | ~8-12 MB | Larger than Snake (sprites) |
| **CPU/Client** | 2-5% | Idle at ~1% |

**Why faster tick rate?**
- Snake is turn-based → can tolerate 120ms delay
- Karting is physics-continuous → 30ms feels smooth, 120ms feels laggy
- Analog input needs responsive feedback

## Collision Detection: Technical Details

### Server-Side Pixel Analysis

```cpp
class GameLogic {
    sf::Image m_trackImage;  // Loaded at init
    
    bool isOnTrack(Vec2 position) {
        int x = static_cast<int>(position.x);
        int y = static_cast<int>(position.y);
        
        if (x < 0 || x >= m_trackImage.getSize().x ||
            y < 0 || y >= m_trackImage.getSize().y) {
            return false;  // Off image = off track
        }
        
        sf::Color pixel = m_trackImage.getPixel(x, y);
        
        // Grass = RGB(0, 128, 0)
        if (pixel.r == 0 && pixel.g == 128 && pixel.b == 0) {
            return false;  // On grass
        }
        
        return true;  // On road
    }
    
    bool checkLapCompletion(InternalCarState& car) {
        bool nowOnFinishLine = isFinishLine(car.position);
        bool completedLap = !car.lastOnFinishLine && nowOnFinishLine;
        
        if (completedLap) {
            car.lapsCompleted++;
            if (car.lapsCompleted >= LAP_GOAL) {
                car.finishedRace = true;
            }
        }
        
        car.lastOnFinishLine = nowOnFinishLine;
        return completedLap;
    }
};
```

**Key Features:**
- Uses SFML image pixel lookup (no separate physics engine)
- Pixel color determines track state
- Yellow pixels = finish line
- Server-authoritative (clients can't cheat by claiming laps)

## Extending Karting

### Add Speed Boost Zones

1. Add yellow marks to track for boost areas
2. Detect in `isOnTrack()` → apply speed multiplier
3. Update `tick()` to apply boost physics

### Add Obstacles

1. Add red pixels to track image
2. Check in `tick()` for collision → car stops + damage
3. Broadcast damage state to clients

### Add Multiple Laps Goal Selection

1. Add game mode parameter to CONNECT message
2. Server respects LAP_GOAL constant (currently 3)
3. Broadcast current goal in state update

### Adjust Physics Parameters

Edit `GameLogic.h`:
```cpp
static constexpr float ACCELERATION = 200.0f;     // Units/sec²
static constexpr float MAX_SPEED = 300.0f;        // Units/sec
static constexpr float ROTATION_SPEED = 3.0f;     // Degrees/frame
static constexpr float FRICTION = 0.98f;          // Decay per frame (grass 0.8)
```

Lower values = easier driving, higher = more realistic.

## Troubleshooting

### Cars Not Rendering

- Check `sprites/track.png` and `sprites/car.png` exist in build output
- Verify CMakeLists copies sprites folder via `copy_directory` command
- Check console for "Failed to load sprites/..." message

### Game Feels Laggy

- Old tick rate was 120ms, should be 30ms now
- If lag persists, check frame counter (should be 60 FPS client, 33 Hz server)
- Verify network latency (ping localhost should be <1ms)

### Cars Spin Out

- Physics parameters may be too aggressive
- Try increasing `FRICTION` value (0.98 → 0.99)
- Reduce `ROTATION_SPEED` (3.0 → 2.0)

### Lap Counter Stuck

- Check finish line pixels are yellow (0xFF, 0xFF, 0x00) in track image
- Verify car actually crosses finish line (check position output)
- Ensure `LAP_GOAL = 3` matches expected finish count

### Multi-Player Issues

- See [Player ID Attribution](02_SERVER_AND_NETWORKING.md#multi-device-player-attribution)
- Check server logs for CONNECT/INPUT messages
- Verify all clients see correct player count in lobby
- Test single local player first, then network

### Track Collision Not Working

- Track image not loaded: check console for errors
- Wrong pixel colors: verify grass = RGB(0,128,0), not other greens
- Position out of bounds: ensure car spawn points are on road, not off-image

## Comparison: Snake vs Karting

| Aspect | Snake | Karting |
|--------|-------|---------|
| **Input** | Discrete (4 directions) | Analog (throttle, steer) |
| **Physics** | Grid-based, turn-by-turn | Continuous, real-time |
| **Tick Rate** | 120ms | 30ms |
| **Collision** | Cell-based | Pixel-based image analysis |
| **Rendering** | Grid cells | Sprites with rotation |
| **Movement** | Instant | Acceleration-based |
| **Server Load** | Low (cell updates) | High (physics every 30ms) |

Karting demands faster server ticks due to continuous analog control and physics simulation, whereas Snake's turn-based model tolerates longer delays.
