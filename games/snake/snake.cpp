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
    #include <unistd.h>
#endif

// ============================================================
// Config
// ============================================================

constexpr int GRID_SIZE = 20;
constexpr int GRID_W = 60;
constexpr int GRID_H = 40;
constexpr int MAX_PLAYERS = 4;

// ============================================================
// Shared types (matching server Protocol.h)
// ============================================================

enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };

struct Vec2 {
    int x{};
    int y{};
};

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
            
            m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (m_socket == INVALID_SOCKET) {
                std::cerr << "Socket creation failed" << std::endl;
                WSACleanup();
                return false;
            }
        #else
            m_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (m_socket < 0) {
                std::cerr << "Socket creation failed" << std::endl;
                return false;
            }
        #endif
        
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(m_port);
        
        #ifdef _WIN32
            inet_pton(AF_INET, m_host.c_str(), &serverAddr.sin_addr);
        #else
            inet_aton(m_host.c_str(), &serverAddr.sin_addr);
        #endif
        
        if (::connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection to server failed" << std::endl;
            disconnect();
            return false;
        }
        
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
    
    bool sendInput(int playerId, Direction dir) {
        if (!m_connected) return false;
        
        std::ostringstream oss;
        oss << "{\"type\":\"input\",\"playerId\":" << playerId
            << ",\"direction\":" << static_cast<int>(dir) << "}\n";
        std::string msg = oss.str();
        
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
            state.gameActive = json.find("true", activePos) != std::string::npos;
        }
        
        // Parse players
        size_t playersPos = json.find("\"players\":[");
        if (playersPos != std::string::npos) {
            state.players = parsePlayers(json, playersPos);
        }
        
        // Parse food
        size_t foodPos = json.find("\"food\":[");
        if (foodPos != std::string::npos) {
            state.food = parseFood(json, foodPos);
        }
        
        return state;
    }
    
    std::vector<PlayerState> parsePlayers(const std::string& json, size_t start) {
        std::vector<PlayerState> players;
        
        size_t pos = start + 11; // Skip "players":["
        
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            size_t idPos = json.find("\"id\":", pos);
            if (idPos == std::string::npos) break;
            
            PlayerState p;
            
            // Parse id
            idPos += 5;
            p.id = std::stoi(json.substr(idPos, 1));
            
            // Parse alive
            size_t alivePos = json.find("\"alive\":", idPos);
            if (alivePos != std::string::npos) {
                p.alive = json.find("true", alivePos) < json.find("false", alivePos);
            }
            
            // Parse direction
            size_t dirPos = json.find("\"dir\":", alivePos);
            if (dirPos != std::string::npos) {
                dirPos += 6;
                p.dir = static_cast<Direction>(std::stoi(json.substr(dirPos, 1)));
            }
            
            // Parse score
            size_t scorePos = json.find("\"score\":", dirPos);
            if (scorePos != std::string::npos) {
                scorePos += 8;
                size_t endPos = json.find_first_of(",}", scorePos);
                p.score = std::stoi(json.substr(scorePos, endPos - scorePos));
            }
            
            // Parse body
            size_t bodyPos = json.find("\"body\":[", scorePos);
            if (bodyPos != std::string::npos) {
                p.body = parseVec2Array(json, bodyPos + 8);
            }
            
            players.push_back(p);
            pos = json.find("},{", bodyPos);
            if (pos == std::string::npos) break;
            pos += 2;
        }
        
        return players;
    }
    
    std::vector<Vec2> parseFood(const std::string& json, size_t start) {
        return parseVec2Array(json, start + 8); // Skip "food":["
    }
    
    std::vector<Vec2> parseVec2Array(const std::string& json, size_t start) {
        std::vector<Vec2> result;
        size_t pos = start;
        
        while (true) {
            size_t xPos = json.find("\"x\":", pos);
            if (xPos == std::string::npos || xPos > json.find("]", pos)) break;
            
            Vec2 v;
            xPos += 4;
            size_t commaPos = json.find(",", xPos);
            v.x = std::stoi(json.substr(xPos, commaPos - xPos));
            
            size_t yPos = json.find("\"y\":", commaPos);
            yPos += 4;
            size_t endPos = json.find("}", yPos);
            v.y = std::stoi(json.substr(yPos, endPos - yPos));
            
            result.push_back(v);
            
            pos = json.find(",{", endPos);
            if (pos == std::string::npos) break;
            pos += 2;
        }
        
        return result;
    }
};

// ============================================================
// InputAdapter - reads local keyboard/controller input
// ============================================================

class InputAdapter {
public:
    static std::optional<Direction> getInput(int player) {
        std::optional<Direction> dir;
        
        // Keyboard input (player 0)
        if (player == 0) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                dir = Direction::Up;
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                dir = Direction::Down;
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                dir = Direction::Left;
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                dir = Direction::Right;
            }
        }
        
        // Controller input
        if (!dir && sf::Joystick::isConnected(player)) {
            float x = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::X);
            float y = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::Y);
            
            if (std::abs(x) > std::abs(y)) {
                if (x > 50) {
                    dir = Direction::Right;
                } else if (x < -50) {
                    dir = Direction::Left;
                }
            } else {
                if (y > 50) {
                    dir = Direction::Down;
                } else if (y < -50) {
                    dir = Direction::Up;
                }
            }
        }
        
        return dir; // std::nullopt when no input is active
    }
};

// ============================================================
// Renderer - renders game state
// ============================================================

class Renderer {
public:
    static void drawState(sf::RenderWindow& window, const GameState& state) {
        window.clear(sf::Color(30, 30, 30));
        
        sf::RectangleShape cell(sf::Vector2f(GRID_SIZE - 2.f, GRID_SIZE - 2.f));
        
        // Draw food
        for (const auto& f : state.food) {
            cell.setFillColor(sf::Color::Red);
            cell.setPosition(sf::Vector2f(
                f.x * GRID_SIZE + 1.f,
                f.y * GRID_SIZE + 1.f
            ));
            window.draw(cell);
        }
        
        // Draw players
        static std::array<sf::Color, 4> colors{
            sf::Color::Green,
            sf::Color::Blue,
            sf::Color(255, 165, 0), // Orange
            sf::Color::Yellow
        };
        
        for (const auto& p : state.players) {
            if (!p.alive) continue;
            
            for (size_t i = 0; i < p.body.size(); ++i) {
                cell.setFillColor(i == 0 ? colors[p.id] : sf::Color(120, 120, 120));
                cell.setPosition(sf::Vector2f(
                    p.body[i].x * GRID_SIZE + 1.f,
                    p.body[i].y * GRID_SIZE + 1.f
                ));
                window.draw(cell);
            }
        }
        
        window.display();
    }
};

// ============================================================
// Main (networked client)
// ============================================================

int main(int argc, char* argv[]) {
    std::string serverHost = "127.0.0.1";
    int serverPort = 8765;
    
    // Parse command line arguments
    if (argc > 1) serverHost = argv[1];
    if (argc > 2) serverPort = std::stoi(argv[2]);
    
    sf::RenderWindow window(
        sf::VideoMode({GRID_W * GRID_SIZE, GRID_H * GRID_SIZE}),
        "Multiplayer Snake Client"
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
    std::array<Direction, MAX_PLAYERS> lastInputs{};
    lastInputs.fill(Direction::Right);
    sf::Clock inputClock;
    
    while (window.isOpen()) {
        // Handle events
        while (auto e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        
        // Send input every 100ms to avoid flooding
        if (inputClock.getElapsedTime().asMilliseconds() > 100) {
            for (int player = 0; player < MAX_PLAYERS; ++player) {
                auto newInput = InputAdapter::getInput(player); // player maps to joystick index
                if (newInput && *newInput != lastInputs[player]) {
                    client.sendInput(player, *newInput);
                    lastInputs[player] = *newInput;
                }
            }
            inputClock.restart();
        }
        
        // Receive game state
        GameState newState = client.receiveState();
        static int noStateTicks = 0;
        
        if (!newState.players.empty()) {
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
        
        // Render
        Renderer::drawState(window, currentState);
    }
    
    client.disconnect();
    return 0;
}
