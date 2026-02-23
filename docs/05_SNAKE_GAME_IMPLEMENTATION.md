# Snake Game Implementation

Detailed walkthrough of how the Snake game is implemented using the VirtualController framework.

**See also**: [Karting Game Implementation](06_KARTING_GAME_IMPLEMENTATION.md) for a continuous-physics racing game example.

## Overview

Snake is a grid-based, turn-by-turn multiplayer game where up to 4 players compete to eat food while avoiding obstacles.

| Property | Value |
|----------|-------|
| **Grid Size** | 60×40 cells |
| **Max Players** | 4 simultaneous |
| **Tick Rate** | 120ms (8.3 moves/sec) |
| **Starting Snakes** | 3 body segments |
| **Win Condition** | Last alive player wins |
| **Colors** | Green (P1), Blue (P2), Orange (P3), Yellow (P4) |

## Game Rules

### Movement
- Players move in grid-based directions: UP, DOWN, LEFT, RIGHT
- Each tick, all alive players move in their current direction
- Cannot instantly reverse (e.g., can't go right then left in one move)

### Eating Food
- Food spawns at random positions
- When head lands on food:
  - Snake grows by one segment
  - Food is consumed
  - New food spawns
  - Score increases

### Collisions (player loses)
- **Wall**: Head goes off grid
- **Self**: Head hits own body
- **Other Player**: Head hits another player's body
- Dead player's snake remains on screen but is immobile

### Win Condition
- Last alive player wins
- If all die simultaneously, tie
- Game stays in ENDED state until RESET_GAME

## Startup Positions & Colors

Configured in `GameLogic::init()` based on player count:

### 1 Player
```
Position: (30, 20) - Center
Color: Green
Direction: Right
```

### 2 Players
```
Player 0: (10, 10) - Top-left,    Green,  Right
Player 1: (50, 30) - Bottom-right, Blue,   Left
```

### 3 Players
```
Player 0: (10, 10) - Top-left,     Green,  Right
Player 1: (50, 10) - Top-right,    Blue,   Left
Player 2: (30, 30) - Center-bottom, Orange, Up
```

### 4 Players
```
Player 0: (10, 10) - Top-left,     Green,  Right
Player 1: (50, 10) - Top-right,    Blue,   Left
Player 2: (10, 30) - Bottom-left,  Orange, Right
Player 3: (50, 30) - Bottom-right, Yellow, Left
```

All players start with 3 body segments trailing behind the head.

## Architecture

### File Structure

```
games/snake/
├── CMakeLists.txt          # Build configuration
├── snake.cpp               # Single-file game implementation
│   ├── Structs (100 lines)
│   ├── NetworkClient (200 lines)
│   ├── Renderer (300 lines)
│   ├── InputAdapter (60 lines)
│   └── main() (400 lines)
└── snake.ico               # Game icon (48x48)
```

### Code Organization

```cpp
snake.cpp (800 lines)
├── Direction enum           // UP, DOWN, LEFT, RIGHT
├── GameState struct         // Client-side game state representation
│   └── Players, food, connected count, state
│
├── NetworkClient class      // TCP connection management
│   ├── connect()            // TCP connect to server
│   ├── sendInput()          // Send movement command
│   ├── sendConnect()        // Report controller count
│   ├── sendStartGame()      // Start signal
│   ├── sendResetGame()      // Reset signal
│   ├── receiveState()       // Receive game state
│   └── parseGameState()     // Parse JSON → GameState
│
├── InputAdapter class       // Input reading
│   ├── getInput(player)     // Keyboard or joystick
│   └── countConnectedControllers()  // Enumerate input devices
│
├── Renderer class           // Drawing game
│   ├── drawState()          // Game board + snakes + food
│   ├── drawLobby()          // Start screen with player count
│   ├── drawEndScreen()      // Final scores + reset button
│   └── [helper methods]
│
└── main()                   // Game loop
    ├── Initialize window
    ├── Connect to server
    ├── Load font (Arial)
    ├── Game loop
    │   ├── Handle events
    │   ├── Send input
    │   ├── Receive state
    │   ├── Render screen
    │   └─ Handle UI clicks
    └── Cleanup
```

## Game Loop

Main event loop runs at 60 FPS (SFML's frame rate limit).

```cpp
while (window.isOpen()) {
    // 1. Process window events
    while (auto e = window.pollEvent()) {
        // Handle close, mouse clicks, keyboard
    }
    
    // 2. Send input every 100ms
    if (inputClock.getElapsedTime().asMilliseconds() > 100) {
        if (currentState.gameActive) {
            for (int player = 0; player < MAX_PLAYERS; ++player) {
                auto input = InputAdapter::getInput(player);
                if (input && *input != lastInputs[player]) {
                    client.sendInput(player, *input);
                    lastInputs[player] = *input;
                }
            }
        }
        inputClock.restart();
    }
    
    // 3. Receive server state (non-blocking)
    GameState newState = client.receiveState();
    if (!newState.players.empty()) {
        currentState = newState;
    }
    
    // 4. Render based on game state
    if (currentState.state == 2) {      // ENDED
        Renderer::drawEndScreen(...);
    } else if (currentState.state == 0) { // LOBBY
        Renderer::drawLobby(...);
    } else {                              // ACTIVE
        Renderer::drawState(...);
    }
}
```

## Input Handling

### Input Adapter

```cpp
static std::optional<Direction> getInput(int player) {
    if (player == 0) {
        // Check keyboard
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    return UP;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  return DOWN;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  return LEFT;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) return RIGHT;
    }
    
    if (sf::Joystick::isConnected(player)) {
        // Check joystick
        float x = sf::Joystick::getAxisPosition(player, X_AXIS);
        float y = sf::Joystick::getAxisPosition(player, Y_AXIS);
        
        if (abs(x) > abs(y)) {
            if (x > 50)  return RIGHT;
            if (x < -50) return LEFT;
        } else {
            if (y > 50)  return DOWN;
            if (y < -50) return UP;
        }
    }
    
    return std::nullopt;  // No input
}
```

### Input Throttling

Input is only sent every 100ms (not every frame) to avoid:
- Server message flooding
- Repeated identical commands
- Wasting bandwidth

```cpp
if (inputClock.getElapsedTime().asMilliseconds() > 100) {
    // Send input
    inputClock.restart();
}
```

## Rendering

### Window & Cells

```
Window: 600×400 pixels
Grid:   60×40 cells
Cell:   10×10 pixels (600÷60 = 10, 400÷40 = 10)
```

### drawState() - Game Screen

```cpp
// Clear background
window.clear(sf::Color(30, 30, 30));  // Dark gray

// Draw food (red)
for (const auto& f : food) {
    cell.setFillColor(sf::Color::Red);
    cell.setPosition(f.x * 10, f.y * 10);
    window.draw(cell);
}

// Draw players
static std::array<sf::Color, 4> colors{
    sf::Color::Green,           // Player 0
    sf::Color::Blue,            // Player 1
    sf::Color(255, 165, 0),     // Player 2 (Orange)
    sf::Color::Yellow           // Player 3
};

for (const auto& player : players) {
    if (!player.alive) continue;
    
    for (size_t i = 0; i < player.body.size(); ++i) {
        if (i == 0) {
            // Head: bright color
            cell.setFillColor(colors[player.id]);
        } else {
            // Body: darker gray
            cell.setFillColor(sf::Color(120, 120, 120));
        }
        cell.setPosition(player.body[i].x * 10, player.body[i].y * 10);
        window.draw(cell);
    }
}

window.display();
```

### drawLobby() - Start Screen

```
╔═════════════════════════════════╗
║     Multiplayer Snake           ║
║                                 ║
║  Connected Players: 2           ║
║                                 ║
║     ┌─────────────────────┐     ║
║     │  START GAME         │     ║
║     └─────────────────────┘     ║
║                                 ║
║  Waiting for more players...    ║
╚═════════════════════════════════╝
```

Displays:
- Game title
- Number of connected players
- START button (clickable, enabled if >0 players)
- Button hover effect (lighter color)

### drawEndScreen() - Game Over

```
╔═════════════════════════════════╗
║          GAME OVER              ║
║                                 ║
║  Final Scores:                  ║
║  Player 1: 245 points (Green)   ║
║  Player 2: 180 points (Blue)    ║
║  Player 3:   0 points (Orange)  ║
║                                 ║
║     ┌─────────────────────┐     ║
║     │  BACK TO START      │     ║
║     └─────────────────────┘     ║
╚═════════════════════════════════╝
```

Displays:
- "GAME OVER" title (red)
- Final scores for all players (color-coded)
- BACK TO START button (clickable, resets game)

## Networking Integration

### Initial Connection

```cpp
// Count local controllers
int controllers = InputAdapter::countConnectedControllers();

// Connect to server
NetworkClient client(serverHost, serverPort);
if (!client.connect()) {
    // Show error window
}

// Report controller count
client.sendConnect(controllers);
```

### Input Sending

```cpp
// Player presses key
auto input = InputAdapter::getInput(player);

// Send to server with local player ID
if (input) {
    client.sendInput(player, *input);
    // Server converts: local player → global player
}
```

### State Reception

```cpp
// Non-blocking receive
GameState newState = client.receiveState();

// Valid state returned
if (!newState.players.empty()) {
    currentState = newState;
}

// Parse game state field for UI
if (currentState.state == 0) {  // LOBBY
    showStartButton();
} else if (currentState.state == 1) {  // ACTIVE
    hideStartButton();
} else if (currentState.state == 2) {  // ENDED
    showResetButton();
}
```

## UI Interactions

### Mouse Click Handling

```cpp
if (e->is<sf::Event::MouseButtonPressed>()) {
    sf::Vector2f mousePos = window.mapPixelToCoords(e->as<sf::Event::MouseButtonPressed>().position);
    
    if (currentState.state == 2) {  // ENDED
        // Check if clicked BACK TO START button
        if (mousePos.x >= 350 && mousePos.x <= 650 &&
            mousePos.y >= 480 && mousePos.y <= 560) {
            client.sendResetGame();
        }
    } else if (!currentState.gameActive) {  // LOBBY
        // Check if clicked START button
        if (mousePos.x >= 350 && mousePos.x <= 650 &&
            mousePos.y >= 400 && mousePos.y <= 480) {
            client.sendStartGame();
        }
    }
}
```

### Button Coordinates

```
Window: 600×400 pixels
X: 350-650 (center, 300 wide)
Y: 400-480 (start button)
Y: 480-560 (end screen button)
```

## Multi-Player Scenarios

### Scenario 1: 2 PCs, 1 Controller Each

```
PC 1: keyboard → local player 0 → global player 0
PC 2: keyboard → local player 0 → global player 1

Server receives:
- CONNECT from PC1: assigned [0]
- CONNECT from PC2: assigned [1]
- INPUT from PC1 player 0 → converts to global 0
- INPUT from PC2 player 0 → converts to global 1
- Broadcasts state with 2 snakes
```

### Scenario 2: 1 PC, 2 Joysticks + Keyboard

```
PC 1: keyboard → local player 0 → global player 0
      joystick 0 → local player 1 → global player 1
      joystick 1 → local player 2 → global player 2

Server receives:
- CONNECT from PC1: assigned [0,1,2]
- INPUT from PC1 player 0 → converts to global 0
- INPUT from PC1 player 1 → converts to global 1
- INPUT from PC1 player 2 → converts to global 2
- Broadcasts state with 3 snakes
```

## Performance Characteristics

| Metric | Value |
|--------|-------|
| **Frame Rate** | 60 FPS (window.setFramerateLimit) |
| **Network Update** | ~8.3 updates/sec (120ms server tick) |
| **Input Send Rate** | ~10 Hz (100ms throttle) |
| **Memory Usage** | ~5-10 MB per client |
| **CPU Usage** | <1% idle, 5-10% during gameplay |

## Extending Snake

### Add Power-ups

1. Add to `GameState`:
   ```cpp
   std::vector<PowerUp> powerups;
   ```

2. Update serialization in `serializeGameState()`

3. Add collision detection in `tick()`

### Add Obstacles

Same pattern: add to game state, serialize, detect collisions in tick.

### Increase Grid Size

Change in `GameLogic::init()`:
```cpp
static constexpr int GRID_W = 100;  // Was 60
static constexpr int GRID_H = 60;   // Was 40
```

Adjust window size in snake.cpp to match.

## Troubleshooting

### Game Won't Connect

- Check server running on correct port
- Verify firewall allows port 8765
- Check command-line arguments (host, port)

### Controls Unresponsive

- Verify joystick detected (SFML may need reinitialization)
- Check keyboard focus on window
- Increase input throttle if too many messages

### Rendering Artifacts

- Try rebuilding SFML
- Check video driver compatibility
- Verify window size calculations

### Multi-player Issues

- See [Player ID Attribution](02_SERVER_AND_NETWORKING.md#multi-device-player-attribution)
- Check server logs for connection/assignment messages
- Verify all clients received correct global player IDs
