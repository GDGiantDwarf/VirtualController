#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <optional>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

// ============================================================
// Config
// ============================================================

constexpr int MAX_PLAYERS = 4;
constexpr int LAP_GOAL = 3;
constexpr float PI = 3.14159265359f;

// ============================================================
// Shared types (matching server Protocol.h for karting)
// ============================================================

struct Vec2 {
    float x{};
    float y{};
};

struct CarState {
    int playerId{};
    Vec2 position;
    Vec2 velocity;
    float rotation{0.0f};
    float speed{0.0f};
    int lapsCompleted{0};
    bool finishedRace{false};
};

struct GameState {
    std::vector<CarState> cars;
    std::vector<int> finishOrder;
    bool gameActive{false};
    int connectedPlayers{0};
    int state{0}; // 0=LOBBY, 1=ACTIVE, 2=ENDED
    bool valid{false}; // Set to true when we successfully parse JSON
};

// ============================================================
// NetworkClient - handles connection to game server
// ============================================================

class NetworkClient {
public:
    NetworkClient(const std::string& host, int port)
        : m_host(host), m_port(port), m_connected(false) {
        #ifdef _WIN32
            m_socket = INVALID_SOCKET;
        #else
            m_socket = -1;
        #endif
    }
    
    ~NetworkClient() {
        disconnect();
    }
    
    bool connect() {
        #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                std::cerr << "WSAStartup failed" << std::endl;
                return false;
            }
        #endif
        
        // Use getaddrinfo for DNS resolution (supports both hostnames and IPs)
        addrinfo hints{};
        hints.ai_family = AF_INET;      // IPv4
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        
        addrinfo* result = nullptr;
        std::string portStr = std::to_string(m_port);
        
        int ret = getaddrinfo(m_host.c_str(), portStr.c_str(), &hints, &result);
        if (ret != 0) {
            #ifdef _WIN32
                std::cerr << "getaddrinfo failed for " << m_host << ": " << gai_strerror(ret) << std::endl;
                WSACleanup();
            #else
                std::cerr << "getaddrinfo failed for " << m_host << ": " << gai_strerror(ret) << std::endl;
            #endif
            return false;
        }
        
        // Create socket
        #ifdef _WIN32
            m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (m_socket == INVALID_SOCKET) {
                std::cerr << "Socket creation failed" << std::endl;
                freeaddrinfo(result);
                WSACleanup();
                return false;
            }
        #else
            m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (m_socket < 0) {
                std::cerr << "Socket creation failed" << std::endl;
                freeaddrinfo(result);
                return false;
            }
        #endif
        
        // Connect using resolved address
        if (::connect(m_socket, result->ai_addr, result->ai_addrlen) < 0) {
            #ifdef _WIN32
                std::cerr << "Connection to " << m_host << ":" << m_port << " failed (error: " << WSAGetLastError() << ")" << std::endl;
            #else
                std::cerr << "Connection to " << m_host << ":" << m_port << " failed" << std::endl;
            #endif
            freeaddrinfo(result);
            disconnect();
            return false;
        }
        
        freeaddrinfo(result);
        
        // Set non-blocking mode
        #ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(m_socket, FIONBIO, &mode);
        #else
            int flags = fcntl(m_socket, F_GETFL, 0);
            fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
        #endif
        
        m_connected = true;
        std::cout << "Connected to server at " << m_host << ":" << m_port << std::endl;
        return true;
    }
    
    void disconnect() {
        if (m_connected) {
            #ifdef _WIN32
                closesocket(m_socket);
                WSACleanup();
                m_socket = INVALID_SOCKET;
            #else
                ::close(m_socket);
                m_socket = -1;
            #endif
            m_connected = false;
        }
    }
    
    bool sendInput(int playerId, float throttle, float steer) {
        if (!m_connected) return false;
        
        std::ostringstream oss;
        oss << "{\"type\":\"input\",\"playerId\":" << playerId
            << ",\"throttle\":" << throttle
            << ",\"steer\":" << steer << "}\n";
        std::string msg = oss.str();
        
        #ifdef _WIN32
            int result = ::send(m_socket, msg.c_str(), static_cast<int>(msg.size()), 0);
        #else
            ssize_t result = ::send(m_socket, msg.c_str(), msg.size(), 0);
        #endif
        
        return result > 0;
    }
    
    bool sendStartGame() {
        if (!m_connected) return false;
        
        std::string msg = "{\"type\":\"start\"}\n";
        
        #ifdef _WIN32
            int result = ::send(m_socket, msg.c_str(), static_cast<int>(msg.size()), 0);
        #else
            ssize_t result = ::send(m_socket, msg.c_str(), msg.size(), 0);
        #endif
        
        return result > 0;
    }
    
    bool sendResetGame() {
        if (!m_connected) return false;
        
        std::string msg = "{\"type\":\"reset\"}\n";
        
        #ifdef _WIN32
            int result = ::send(m_socket, msg.c_str(), static_cast<int>(msg.size()), 0);
        #else
            ssize_t result = ::send(m_socket, msg.c_str(), msg.size(), 0);
        #endif
        
        return result > 0;
    }
    
    bool sendConnect(int controllerCount) {
        if (!m_connected) return false;
        
        std::string msg = "{\"type\":\"connect\",\"controllers\":" + std::to_string(controllerCount) + "}\n";
        
        #ifdef _WIN32
            int result = ::send(m_socket, msg.c_str(), static_cast<int>(msg.size()), 0);
        #else
            ssize_t result = ::send(m_socket, msg.c_str(), msg.size(), 0);
        #endif
        
        return result > 0;
    }
    
    GameState receiveState() {
        GameState state;
        if (!m_connected) return state;
        
        char buffer[8192];
        #ifdef _WIN32
            int result = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        #else
            ssize_t result = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        #endif
        
        if (result <= 0) {
            return state;
        }
        
        buffer[result] = '\0';
        std::string data(buffer);
        
        // Parse JSON (simple parser)
        state = parseGameState(data);
        return state;
    }
    
    bool isConnected() const { return m_connected; }
    
private:
    std::string m_host;
    int m_port;
    bool m_connected;
    
    #ifdef _WIN32
        SOCKET m_socket;
    #else
        int m_socket;
    #endif
    
    GameState parseGameState(const std::string& json) {
        GameState state;
        
        // Simple JSON parser - in production, use a proper JSON library
        size_t activePos = json.find("\"active\":");
        if (activePos != std::string::npos) {
            activePos += 9; // Skip "active":
            // Check specifically for true/false right after "active":
            size_t truePos = json.find("true", activePos);
            size_t falsePos = json.find("false", activePos);
            size_t nextCommaPos = json.find(",", activePos);
            
            // Only consider true/false if it appears before the next comma
            state.gameActive = (truePos != std::string::npos && 
                               truePos < nextCommaPos &&
                               (falsePos == std::string::npos || truePos < falsePos));
        }
        
        // Parse connected player count
        size_t connectedPos = json.find("\"connected\":");
        if (connectedPos != std::string::npos) {
            connectedPos += 12; // Skip "connected":
            state.connectedPlayers = std::stoi(json.substr(connectedPos));
        }
        
        // Parse game state (0=LOBBY, 1=ACTIVE, 2=ENDED)
        size_t statePos = json.find("\"state\":");
        if (statePos != std::string::npos) {
            statePos += 8; // Skip "state":
            state.state = std::stoi(json.substr(statePos, 1));
        }
        
        // Parse cars
        size_t carsPos = json.find("\"cars\":[");
        if (carsPos != std::string::npos) {
            state.cars = parseCars(json, carsPos);
        }
        
        // Parse finish order
        size_t finishPos = json.find("\"finishOrder\":[");
        if (finishPos != std::string::npos) {
            state.finishOrder = parseFinishOrder(json, finishPos);
        }
        
        // Mark as valid if we received JSON with state field
        if (statePos != std::string::npos || connectedPos != std::string::npos) {
            state.valid = true;
        }
        
        return state;
    }
    
    std::vector<CarState> parseCars(const std::string& json, size_t start) {
        std::vector<CarState> cars;
        
        size_t pos = start + 8; // Skip "cars":[
        
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            size_t idPos = json.find("\"playerId\":", pos);
            if (idPos == std::string::npos) break;
            
            CarState car;
            
            // Parse playerId
            idPos += 11; // Skip "playerId":
            car.playerId = std::stoi(json.substr(idPos));
            
            // Parse x position
            size_t xPos = json.find("\"x\":", idPos);
            if (xPos != std::string::npos) {
                xPos += 4;
                car.position.x = std::stof(json.substr(xPos));
            }
            
            // Parse y position
            size_t yPos = json.find("\"y\":", xPos);
            if (yPos != std::string::npos) {
                yPos += 4;
                car.position.y = std::stof(json.substr(yPos));
            }
            
            // Parse rotation
            size_t rotPos = json.find("\"rotation\":", yPos);
            if (rotPos != std::string::npos) {
                rotPos += 11;
                car.rotation = std::stof(json.substr(rotPos));
            }
            
            // Parse speed
            size_t speedPos = json.find("\"speed\":", rotPos);
            if (speedPos != std::string::npos) {
                speedPos += 8;
                car.speed = std::stof(json.substr(speedPos));
            }
            
            // Parse laps
            size_t lapsPos = json.find("\"laps\":", speedPos);
            if (lapsPos != std::string::npos) {
                lapsPos += 7;
                car.lapsCompleted = std::stoi(json.substr(lapsPos));
            }
            
            // Parse finished
            size_t finPos = json.find("\"finished\":", lapsPos);
            if (finPos != std::string::npos) {
                finPos += 11;
                car.finishedRace = (json.find("true", finPos) < json.find(",", finPos));
            }
            
            cars.push_back(car);
            pos = json.find("},{", finPos);
            if (pos == std::string::npos) break;
            pos += 2;
        }
        
        return cars;
    }
    
    std::vector<int> parseFinishOrder(const std::string& json, size_t start) {
        std::vector<int> order;
        size_t pos = start + 15; // Skip "finishOrder":["
        
        while (true) {
            size_t numStart = json.find_first_of("0123456789", pos);
            if (numStart == std::string::npos || numStart > json.find("]", pos)) break;
            
            int playerId = std::stoi(json.substr(numStart));
            order.push_back(playerId);
            
            pos = json.find(",", numStart);
            if (pos == std::string::npos) break;
            pos += 1;
        }
        
        return order;
    }
    
};

// ============================================================
// InputAdapter - reads local keyboard/controller input
// ============================================================

class InputAdapter {
public:
    struct Input {
        float throttle{0.0f};
        float steer{0.0f};
    };
    
    static Input getInput(int player) {
        Input input;
        
        // Keyboard input (player 0)
        if (player == 0) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                input.throttle += 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                input.throttle -= 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                input.steer += 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                input.steer -= 1.0f;
            }
        }
        
        bool keyboardActive = (std::abs(input.throttle) > 0.01f || std::abs(input.steer) > 0.01f);
        
        // Controller input
        const float deadzone = 15.0f;
        if (sf::Joystick::isConnected(player) && (player > 0 || !keyboardActive)) {
            float joyX = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::X);
            float joyY = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::Y);
            if (std::abs(joyX) > deadzone) {
                input.steer = joyX / 100.0f;
            }
            if (std::abs(joyY) > deadzone) {
                input.throttle = -joyY / 100.0f;
            }
        }
        
        return input;
    }
    
    static int countConnectedControllers() {
        int count = 1;  // Always count keyboard as player 0
        for (int i = 1; i < MAX_PLAYERS; ++i) {
            if (sf::Joystick::isConnected(i)) {
                count++;
            }
        }
        return count;
    }
};

// ============================================================
// Renderer - renders game state
// ============================================================

class Renderer {
public:
    static void init() {
        // Load track texture
        if (!s_trackTexture.loadFromFile("sprites/track.png")) {
            std::cerr << "Failed to load sprites/track.png" << std::endl;
        }
        
        // Load car texture
        if (!s_carTexture.loadFromFile("sprites/car.png")) {
            std::cerr << "Failed to load sprites/car.png" << std::endl;
        }
        
        s_initialized = true;
    }
    
    static sf::Vector2u getTrackSize() {
        return s_trackTexture.getSize();
    }
    
    static void drawState(sf::RenderWindow& window, const GameState& state, const sf::Font& font) {
        if (!s_initialized) init();
        
        window.clear(sf::Color(50, 120, 50)); // Green grass background
        
        // Draw track
        sf::Sprite trackSprite(s_trackTexture);
        window.draw(trackSprite);
        
        // Draw cars
        static std::array<sf::Color, 4> colors{
            sf::Color::Red,
            sf::Color::Blue,
            sf::Color(255, 165, 0), // Orange
            sf::Color::Yellow
        };
        
        sf::Sprite carSprite(s_carTexture);
        
        // Get car texture size to set origin at center
        sf::Vector2u carSize = s_carTexture.getSize();
        carSprite.setOrigin(sf::Vector2f(carSize.x / 2.f, carSize.y / 2.f));
        
        for (const auto& c : state.cars) {
            carSprite.setPosition(sf::Vector2f(c.position.x, c.position.y));
            carSprite.setRotation(sf::degrees(c.rotation));
            carSprite.setColor(colors[c.playerId % 4]);
            window.draw(carSprite);
        }
        
        // Draw HUD - lap information
        sf::Text hudText(font);
        hudText.setCharacterSize(20);
        hudText.setFillColor(sf::Color::White);
        hudText.setPosition({10.f, 10.f});
        
        std::ostringstream hud;
        for (const auto& c : state.cars) {
            hud << "P" << (c.playerId + 1) << ": " << c.lapsCompleted << " laps\n";
        }
        
        hudText.setString(hud.str());
        window.draw(hudText);
        
        window.display();
    }
    
    static void drawLobby(sf::RenderWindow& window, const GameState& state, const sf::Font& font, const sf::Vector2f& mousePos, bool mousePressed) {
        window.clear(sf::Color(30, 30, 30));
        
        // Title
        sf::Text title(font);
        title.setString("Multiplayer Karting");
        title.setCharacterSize(48);
        title.setFillColor(sf::Color::White);
        title.setPosition({180.f, 100.f});
        window.draw(title);
        
        // Connected players count
        sf::Text playerCount(font);
        std::ostringstream oss;
        oss << "Players Connected: " << state.connectedPlayers << " / " << MAX_PLAYERS;
        playerCount.setString(oss.str());
        playerCount.setCharacterSize(32);
        playerCount.setFillColor(sf::Color(200, 200, 200));
        playerCount.setPosition({280.f, 220.f});
        window.draw(playerCount);
        
        // Waiting message
        if (state.connectedPlayers == 0) {
            sf::Text waiting(font);
            waiting.setString("Waiting for players...");
            waiting.setCharacterSize(24);
            waiting.setFillColor(sf::Color(150, 150, 150));
            waiting.setPosition({330.f, 300.f});
            window.draw(waiting);
        }
        
        // Start button (only if at least one player)
        if (state.connectedPlayers > 0) {
            sf::RectangleShape button(sf::Vector2f(300.f, 80.f));
            button.setPosition({350.f, 400.f});
            
            // Check if mouse is hovering
            bool isHovered = mousePos.x >= 350.f && mousePos.x <= 650.f &&
                           mousePos.y >= 400.f && mousePos.y <= 480.f;
            
            if (isHovered) {
                button.setFillColor(sf::Color(0, 180, 0));
            } else {
                button.setFillColor(sf::Color(0, 150, 0));
            }
            
            button.setOutlineColor(sf::Color::White);
            button.setOutlineThickness(3.f);
            window.draw(button);
            
            sf::Text buttonText(font);
            buttonText.setString("START GAME");
            buttonText.setCharacterSize(36);
            buttonText.setFillColor(sf::Color::White);
            buttonText.setPosition({400.f, 415.f});
            window.draw(buttonText);
        }
        
        // Instructions
        sf::Text instructions(font);
        instructions.setString("Click 'Start Game' to begin");
        instructions.setCharacterSize(20);
        instructions.setFillColor(sf::Color(120, 120, 120));
        instructions.setPosition({320.f, 550.f});
        window.draw(instructions);
        
        window.display();
    }
    
    static void drawEndScreen(sf::RenderWindow& window, const GameState& state, const sf::Font& font, const sf::Vector2f& mousePos) {
        window.clear(sf::Color(30, 30, 30));
        
        // Title
        sf::Text title(font);
        title.setString("GAME OVER");
        title.setCharacterSize(60);
        title.setFillColor(sf::Color::Red);
        title.setPosition({320.f, 50.f});
        window.draw(title);
        
        // Final scores
        sf::Text scoresLabel(font);
        scoresLabel.setString("Final Scores:");
        scoresLabel.setCharacterSize(28);
        scoresLabel.setFillColor(sf::Color::White);
        scoresLabel.setPosition({340.f, 150.f});
        window.draw(scoresLabel);
        
        // Display each player's score
        std::array<sf::Color, 4> colors{
            sf::Color::Green,
            sf::Color::Blue,
            sf::Color(255, 165, 0), // Orange
            sf::Color::Yellow
        };
        
        // Display finish order
        for (size_t i = 0; i < state.finishOrder.size() && i < state.cars.size(); ++i) {
            int carId = state.finishOrder[i];
            // Find the car with this ID
            const CarState* finisher = nullptr;
            for (const auto& c : state.cars) {
                if (c.playerId == carId) {
                    finisher = &c;
                    break;
                }
            }
            
            if (finisher) {
                sf::Text posText(font);
                std::ostringstream oss;
                oss << (i + 1) << ". Player " << (finisher->playerId + 1) 
                    << " - " << finisher->lapsCompleted << " laps";
                posText.setString(oss.str());
                posText.setCharacterSize(24);
                posText.setFillColor(colors[finisher->playerId % 4]);
                posText.setPosition({340.f, 210.f + (i * 40.f)});
                window.draw(posText);
            }
        }
        
        // Back to Start button
        sf::RectangleShape button(sf::Vector2f(300.f, 80.f));
        button.setPosition({350.f, 480.f});
        
        // Check if mouse is hovering
        bool isHovered = mousePos.x >= 350.f && mousePos.x <= 650.f &&
                       mousePos.y >= 480.f && mousePos.y <= 560.f;
        
        if (isHovered) {
            button.setFillColor(sf::Color(200, 0, 0));
        } else {
            button.setFillColor(sf::Color(150, 0, 0));
        }
        
        button.setOutlineColor(sf::Color::White);
        button.setOutlineThickness(3.f);
        window.draw(button);
        
        sf::Text buttonText(font);
        buttonText.setString("BACK TO START");
        buttonText.setCharacterSize(32);
        buttonText.setFillColor(sf::Color::White);
        buttonText.setPosition({375.f, 500.f});
        window.draw(buttonText);
        
        window.display();
    }
    
private:
    static inline bool s_initialized = false;
    static inline sf::Texture s_trackTexture;
    static inline sf::Texture s_carTexture;
};

// ============================================================
// Main (networked client)
// ============================================================

int main(int argc, char* argv[]) {
    std::string serverHost = "127.0.0.1";
    int serverPort = 8766;
    
    // Parse command line arguments
    if (argc > 1) serverHost = argv[1];
    if (argc > 2) serverPort = std::stoi(argv[2]);
    
    // Load track to get window size
    Renderer::init();
    sf::Vector2u trackSize = Renderer::getTrackSize();
    
    sf::RenderWindow window(
        sf::VideoMode(trackSize),
        "Multiplayer Karting Client"
    );
    window.setFramerateLimit(60);
    
    NetworkClient client(serverHost, serverPort);
    
    if (!client.connect()) {
        // Show error in window
        sf::RenderWindow errorWindow(
            sf::VideoMode({450, 150}),
            "Connection Failed"
        );
        
        // Load system font for error message
        sf::Font font;
        bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
        
        // Create text only if font loaded
        std::unique_ptr<sf::Text> errorText;
        if (fontLoaded) {
            std::ostringstream msg;
            msg << "Server Not Running\n\nCannot connect to " << serverHost << ":" << serverPort
                << "\n\nClose this window to exit";
            errorText = std::make_unique<sf::Text>(font);
            errorText->setString(msg.str());
            errorText->setCharacterSize(16);
            errorText->setFillColor(sf::Color::White);
            errorText->setPosition({20.f, 20.f});
        }
        
        while (errorWindow.isOpen()) {
            while (const std::optional event = errorWindow.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    errorWindow.close();
                }
            }
            
            errorWindow.clear(sf::Color(40, 40, 40));
            if (errorText) {
                errorWindow.draw(*errorText);
            }
            errorWindow.display();
        }
        
        return 1;
    }
    
    GameState currentState;
    sf::Clock inputClock;
    sf::Clock controllerCheckClock;
    
    // Send connect message with controller count
    int controllerCount = InputAdapter::countConnectedControllers();
    std::cout << "Sending connect message: " << controllerCount << " controller(s)" << std::endl;
    client.sendConnect(controllerCount);
    
    // Load font for lobby UI
    sf::Font font;
    bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
    
    sf::Vector2f mousePos;
    bool startButtonClicked = false;
    
    while (window.isOpen()) {
        // Check for controller hotplug/unplug every 500ms
        if (controllerCheckClock.getElapsedTime().asMilliseconds() > 500) {
            int newControllerCount = InputAdapter::countConnectedControllers();
            if (newControllerCount != controllerCount) {
                std::cout << "Controller count changed: " << controllerCount << " -> " << newControllerCount << std::endl;
                controllerCount = newControllerCount;
                client.sendConnect(controllerCount);
                std::cout << "Sent CONNECT message with " << controllerCount << " controller(s)" << std::endl;
            }
            controllerCheckClock.restart();
        }
        // Handle events
        while (auto e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto* mouseMoved = e->getIf<sf::Event::MouseMoved>()) {
                mousePos = sf::Vector2f(static_cast<float>(mouseMoved->position.x), 
                                       static_cast<float>(mouseMoved->position.y));
            }
            
            if (e->is<sf::Event::MouseButtonPressed>()) {
                if (currentState.state == 2) {  // ENDED - check for back to start button
                    if (mousePos.x >= 350.f && mousePos.x <= 650.f &&
                        mousePos.y >= 480.f && mousePos.y <= 560.f) {
                        client.sendResetGame();
                    }
                } else if (!currentState.gameActive) {  // LOBBY - check for start button
                    // Check if start button was clicked
                    if (mousePos.x >= 350.f && mousePos.x <= 650.f &&
                        mousePos.y >= 400.f && mousePos.y <= 480.f &&
                        currentState.connectedPlayers > 0) {
                        client.sendStartGame();
                        startButtonClicked = true;
                    }
                }
            }
        }
        
        // Send input every 100ms to avoid flooding (only when game is active)
        if (currentState.gameActive && inputClock.getElapsedTime().asMilliseconds() > 100) {
            for (int player = 0; player < MAX_PLAYERS; ++player) {
                auto input = InputAdapter::getInput(player);
                client.sendInput(player, input.throttle, input.steer);
            }
            inputClock.restart();
        }
        
        // Receive game state
        GameState newState = client.receiveState();
        static int noStateTicks = 0;
        
        if (newState.valid) {
            currentState = newState;
            noStateTicks = 0; // Reset counter on successful receive
        } else {
            // Check if connection is still alive
            // If we're not receiving state, connection may be lost
            noStateTicks++;
            if (noStateTicks > 100) { // ~1.6 seconds without state
                // Connection lost - show error window
                window.close();
                
                sf::RenderWindow errorWindow(
                    sf::VideoMode({450, 150}),
                    "Connection Lost"
                );
                
                // Load system font for error message
                sf::Font font;
                bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
                
                // Create text only if font loaded
                std::unique_ptr<sf::Text> errorText;
                if (fontLoaded) {
                    std::ostringstream msg;
                    msg << "Server Disconnected\n\nConnection to " << serverHost << ":" << serverPort
                        << " lost\n\nClose this window to exit";
                    errorText = std::make_unique<sf::Text>(font);
                    errorText->setString(msg.str());
                    errorText->setCharacterSize(16);
                    errorText->setFillColor(sf::Color::White);
                    errorText->setPosition({20.f, 20.f});
                }
                
                while (errorWindow.isOpen()) {
                    while (const std::optional event = errorWindow.pollEvent()) {
                        if (event->is<sf::Event::Closed>()) {
                            errorWindow.close();
                        }
                    }
                    
                    errorWindow.clear(sf::Color(40, 40, 40));
                    if (errorText) {
                        errorWindow.draw(*errorText);
                    }
                    errorWindow.display();
                }
                
                break; // Exit game loop
            }
        }
        
        // Render lobby, game, or end screen
        if (currentState.state == 2) {  // ENDED
            if (fontLoaded) {
                Renderer::drawEndScreen(window, currentState, font, mousePos);
            } else {
                window.clear(sf::Color(30, 30, 30));
                window.display();
            }
        } else if (!currentState.gameActive || currentState.state == 0) {  // LOBBY
            if (fontLoaded) {
                Renderer::drawLobby(window, currentState, font, mousePos, startButtonClicked);
            } else {
                // Fallback if font doesn't load
                window.clear(sf::Color(30, 30, 30));
                window.display();
            }
        } else {  // ACTIVE
            Renderer::drawState(window, currentState, font);
        }
    }
    
    client.disconnect();
    return 0;
}
