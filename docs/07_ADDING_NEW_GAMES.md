# Adding New Games

Step-by-step guide for creating and integrating a new multiplayer game into VirtualController.

## Overview

A game in VirtualController is a C++ executable that:
1. Connects to the game server via TCP
2. Receives authoritative game state every 120ms
3. Sends player input to the server
4. Renders the game state locally
5. Handles UI (start/reset buttons)

The server handles all physics, collision detection, and game logic.

## Game Template

### Folder Structure

```
games/
└── my_game/
    ├── CMakeLists.txt           # Build config (template provided)
    ├── my_game.cpp              # Main game file
    ├── my_game.ico              # 48×48 icon (for launcher)
    └── README.md                # Game-specific documentation
```

**Important**: Folder name MUST match executable name (e.g., `my_game/` → `my_game.exe`)

### CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_game VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML for graphics
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

# Create executable
add_executable(my_game my_game.cpp)

# Link libraries
target_link_libraries(my_game 
    SFML::Graphics
    SFML::Window
    SFML::System
)

# Windows sockets for networking
if(WIN32)
    target_link_libraries(my_game ws2_32)
endif()

# Output to root build directory
set_target_properties(my_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Copy DLLs to output directory
add_custom_command(TARGET my_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        "$<TARGET_FILE_DIR:my_game>"
    COMMENT "Copying SFML DLLs"
)

# Copy icon to output directory
add_custom_command(TARGET my_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/my_game.ico"
        "$<TARGET_FILE_DIR:my_game>/my_game.ico"
    COMMENT "Copying game icon"
)
```

### Minimal Game Code Structure

```cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

// ============================================================
// Protocol & Networking Setup
// ============================================================

// 1. Copy these from server/src/Protocol.h
enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };
struct Vec2 { int x{}, y{}; };
struct PlayerState {
    int id{};
    bool alive{true};
    Direction dir{Direction::Right};
    std::vector<Vec2> body;
    int score{0};
};
struct GameState {
    std::vector<PlayerState> players;
    std::vector<Vec2> food;
    bool gameActive{false};
    int state{0};  // 0=LOBBY, 1=ACTIVE, 2=ENDED
};

// 2. Implement NetworkClient class
class NetworkClient {
public:
    NetworkClient(const std::string& host, int port);
    ~NetworkClient();
    
    bool connect();
    bool sendConnect(int controllerCount);
    bool sendInput(int playerId, Direction direction);
    bool sendStartGame();
    bool sendResetGame();
    GameState receiveState();
    bool isConnected() const;
    
private:
    std::string m_host;
    int m_port;
    bool m_connected;
    // Socket members (Windows/Linux compatible)
};

// 3. Implement InputAdapter
class InputAdapter {
public:
    static std::optional<Direction> getInput(int player) {
        // Read keyboard for player 0
        // Read joystick[i] for player i
    }
    
    static int countConnectedControllers() {
        // Count keyboard + joysticks
    }
};

// 4. Implement Renderer
class Renderer {
public:
    static void drawGame(sf::RenderWindow& window, const GameState& state);
    static void drawLobby(sf::RenderWindow& window, const GameState& state, const sf::Font& font);
    static void drawEndScreen(sf::RenderWindow& window, const GameState& state, const sf::Font& font);
};

// ============================================================
// Main Game Loop
// ============================================================

int main(int argc, char* argv[]) {
    std::string serverHost = "127.0.0.1";
    int serverPort = 8765;
    
    if (argc > 1) serverHost = argv[1];
    if (argc > 2) serverPort = std::stoi(argv[2]);
    
    // Create window
    sf::RenderWindow window(sf::VideoMode({800, 600}), "My Game");
    window.setFramerateLimit(60);
    
    // Connect to server
    NetworkClient client(serverHost, serverPort);
    if (!client.connect()) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    // Report controllers
    int controllerCount = InputAdapter::countConnectedControllers();
    client.sendConnect(controllerCount);
    
    // Load font for UI
    sf::Font font;
    font.openFromFile("C:/Windows/Fonts/arial.ttf");
    
    // Game variables
    GameState currentState;
    sf::Clock inputClock;
    
    // Main loop
    while (window.isOpen()) {
        // Handle events
        while (auto e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>()) {
                window.close();
            }
            // Handle mouse clicks for START/RESET buttons
        }
        
        // Send input every 100ms
        if (inputClock.getElapsedTime().asMilliseconds() > 100) {
            if (currentState.gameActive) {
                for (int i = 0; i < 4; ++i) {
                    auto input = InputAdapter::getInput(i);
                    if (input) {
                        client.sendInput(i, *input);
                    }
                }
            }
            inputClock.restart();
        }
        
        // Receive state
        auto newState = client.receiveState();
        if (!newState.players.empty()) {
            currentState = newState;
        }
        
        // Render
        window.clear(sf::Color::Black);
        if (currentState.state == 0) {       // LOBBY
            Renderer::drawLobby(window, currentState, font);
        } else if (currentState.state == 1) { // ACTIVE
            Renderer::drawGame(window, currentState);
        } else {                              // ENDED
            Renderer::drawEndScreen(window, currentState, font);
        }
        window.display();
    }
    
    return 0;
}
```

## Key Components to Implement

### 1. NetworkClient

Must implement JSON protocol handling:

**Methods:**
- `connect()` - TCP socket to server
- `sendConnect(controllerCount)` - Report input devices
- `sendInput(playerId, direction)` - Send movement command
- `sendStartGame()` - Transition LOBBY → ACTIVE
- `sendResetGame()` - Transition ENDED → LOBBY
- `receiveState()` - Receive complete game state
- `parseGameState()` - Parse JSON to GameState struct

**Reference**: See `games/snake/snake.cpp` lines 60-240 for full implementation.

### 2. InputAdapter

Reads local input (keyboard + joysticks):

```cpp
// Read keyboard for player 0
if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) return Direction::Up;

// Read joystick for other players
if (sf::Joystick::isConnected(player)) {
    float x = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::X);
    float y = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::Y);
    // Map to direction based on stick position
}
```

**Reference**: See `games/snake/snake.cpp` lines 385-430.

### 3. Renderer

Displays game state based on current screen:

```cpp
switch (gameState.state) {
    case 0:  // LOBBY
        drawLobby(...);
        break;
    case 1:  // ACTIVE
        drawGame(...);
        break;
    case 2:  // ENDED
        drawEndScreen(...);
        break;
}
```

Each draw function receives the complete GameState and renders accordingly.

**Reference**: See `games/snake/snake.cpp` lines 455-630.

### 4. Game Server Component

Each game needs a corresponding server-side implementation:

```cpp
// GameLogic.cpp
void GameLogic::init(int playerCount) {
    // Initialize game with N players
    // Set starting positions and state
}

void GameLogic::tick() {
    // Apply physics
    // Check collisions
    // Update scores
    // Check win/lose conditions
}
```

Server-side logic is per-game, but the framework (connections, message handling) is shared.

## Building Your Game

### Add to Root CMakeLists.txt

Edit `CMakeLists.txt` in root directory:

```cmake
add_subdirectory(games/my_game)
```

### Build

```bash
# From root directory
./build.bat

# Or manually:
cd build
cmake --build . --config Release --target my_game
```

Output: `build/bin/Release/my_game.exe`

## Game Discovery

The launcher automatically discovers your game:

1. **Scans** `build/bin/Release/` for `.exe` files
2. **Filters** by name (skips GameLibraryLauncher, GameServer)
3. **Looks for icon** at `build/bin/Release/my_game.ico`
4. **Displays** in launcher UI

### Icon Requirements

- **Filename**: `my_game.ico`
- **Size**: 48×48 pixels (minimum)
- **Format**: Windows ICO format
- **Location**: Copied to `build/bin/Release/` at build time

Create icon from image:
```bash
# Using ImageMagick
convert my_game_image.png -define icon:auto-resize=48 my_game.ico
```

# Game Launch

When user clicks your game in launcher:

```cpp
// Launcher executes:
./build/bin/Release/my_game.exe 127.0.0.1 8765

// Your main() receives:
argv[1] = server address
argv[2] = server port
```

## Game Protocol Requirements

Every game must:

1. **Report capabilities on connect**:
   ```json
   {"type": "connect", "controllers": 2}
   ```

2. **Send input every 100-500ms**:
   ```json
   {"type": "input", "playerId": 0, "direction": 3}
   ```

3. **Handle three game states**:
   - LOBBY (state=0): Show START button
   - ACTIVE (state=1): Game running
   - ENDED (state=2): Show RESET button

4. **Parse JSON state broadcasts**:
   ```json
   {
     "type": "state",
     "state": 1,
     "connected": 2,
     "players": [...],
     "food": [...]
   }
   ```

See [Game Protocol Specification](03_GAME_PROTOCOL.md) for details.

## Testing Multi-Player

### Local Testing (2 Controllers on 1 PC)

```bash
# Terminal 1: Start server
.\build\bin\Release\GameServer.exe

# Terminal 2: Start game
.\build\bin\Release\my_game.exe 127.0.0.1 8765

# Use keyboard (player 0) + joystick (player 1)
```

### Network Testing (Multiple PCs)

```bash
# PC1 - Start server
.\build\bin\Release\GameServer.exe

# PC2 - Start game (use PC1's IP)
.\build\bin\Release\my_game.exe 192.168.1.100 8765

# PC3 - Start another game
.\build\bin\Release\my_game.exe 192.168.1.100 8765

# Each PC should show unique players
```

## Common Pitfalls

### Socket Errors

- **"Connection refused"**: Server not running
- **"Permission denied"**: Port in use (try port 9000)
- Solution: Check server is running before launching game

### Protocol Mismatches

- **Game freezes**: Wrong JSON format, server can't parse
- **Input not working**: playerId out of range
- Solution: Compare with snake.cpp protocol implementation

### Missing Dependencies

- **SFML DLLs not copied**: CMakeLists.txt post_build command failed
- **Font not loading**: Font path hardcoded (C:/Windows/Fonts/arial.ttf)
- Solution: Use debug_proxy.py to see actual network traffic

### Controller Not Detected

- **Joystick index mismatch**: SFML numbers differently than OS
- **Deadzone too large**: Default 50 units may be too aggressive
- Solution: Test with DirectInput viewer before deploying

## Example Games to Study

**Reference implementations** are available to learn from:

### Snake - 800 lines, turn-based game
- Single-file structure
- Discrete direction input (4 choices)
- Network client sends input on change only
- Server tick rate: 120ms (8.3 updates/sec)
- Grid-based collision detection
- Multi-player tested

### Karting - 900 lines, continuous-physics game  
- Single-file structure
- Analog input (throttle + steer, -1..1 range)
- Network client sends input continuously
- Server tick rate: 30ms (33 updates/sec)
- Pixel-based collision detection
- Multi-player tested

Extend either implementation or copy-paste their structure for your new game. Choose Snake as model for turn-based games, or Karting for real-time physics games.

## Performance Targets

- **Network**: <50ms latency, <10 KB/s per client
- **Graphics**: 60 FPS on mid-range hardware
- **Logic**: <5ms per game tick
- **Memory**: <20 MB per client

## Next Steps

1. Create `games/my_game/` folder
2. Copy CMakeLists.txt template
3. Implement minimal game code (start with Snake copy)
4. Build: `./build.bat`
5. Test: Run `my_game.exe` with server
6. Debug: Use debug_proxy.py for network inspection
7. Extend: Add your game mechanics

## Getting Help

- Compare with `games/snake/snake.cpp` line-by-line
- Check [Snake Game Implementation](05_SNAKE_GAME_IMPLEMENTATION.md) for detailed explanations
- See [Game Protocol Specification](03_GAME_PROTOCOL.md) for message format details
- Review [Server & Networking](02_SERVER_AND_NETWORKING.md) for server behavior
