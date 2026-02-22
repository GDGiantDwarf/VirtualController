#include "GameServer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

GameServer::GameServer(int port) : m_port(port) {
    #ifdef _WIN32
        m_serverSocket = INVALID_SOCKET;
    #else
        m_serverSocket = -1;
    #endif
    
    // Initialize pending inputs
    for (int i = 0; i < GameLogic::MAX_PLAYERS; ++i) {
        m_pendingInputs[i].playerId = i;
        m_pendingInputs[i].direction = Protocol::Direction::Right;
    }
}

GameServer::~GameServer() {
    stop();
}

bool GameServer::start() {
    if (m_running) return false;
    
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }
    #endif
    
    if (!initializeSocket()) {
        #ifdef _WIN32
            WSACleanup();
        #endif
        return false;
    }
    
    m_running = true;
    std::cout << "Server started on port " << m_port << std::endl;
    
    return true;
}

void GameServer::stop() {
    if (!m_running) return;
    
    m_running = false;
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections.clear();
    }
    
    // Close server socket
    #ifdef _WIN32
        if (m_serverSocket != INVALID_SOCKET) {
            closesocket(m_serverSocket);
            m_serverSocket = INVALID_SOCKET;
        }
        WSACleanup();
    #else
        if (m_serverSocket >= 0) {
            ::close(m_serverSocket);
            m_serverSocket = -1;
        }
    #endif
    
    // Join threads
    if (m_acceptThread.joinable()) m_acceptThread.join();
    if (m_gameThread.joinable()) m_gameThread.join();
    
    std::cout << "Server stopped" << std::endl;
}

void GameServer::run() {
    if (!m_running) {
        std::cerr << "Server not started" << std::endl;
        return;
    }
    
    // Start accept thread
    m_acceptThread = std::thread(&GameServer::acceptLoop, this);
    
    // Start game loop thread
    m_gameThread = std::thread(&GameServer::gameLoop, this);
    
    // Wait for game thread to finish
    if (m_gameThread.joinable()) {
        m_gameThread.join();
    }
    
    // Wait for accept thread to finish
    if (m_acceptThread.joinable()) {
        m_acceptThread.join();
    }
}

bool GameServer::initializeSocket() {
    #ifdef _WIN32
        m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
    #else
        m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_serverSocket < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
    #endif
    
    // Set socket options
    int opt = 1;
    #ifdef _WIN32
        setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    #else
        setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
    
    // Bind socket
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_port);
    
    if (bind(m_serverSocket, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return false;
    }
    
    // Listen
    if (listen(m_serverSocket, 4) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return false;
    }
    
    // Set non-blocking mode
    #ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(m_serverSocket, FIONBIO, &mode);
    #else
        int flags = fcntl(m_serverSocket, F_GETFL, 0);
        fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);
    #endif
    
    return true;
}

void GameServer::acceptLoop() {
    std::cout << "Accept loop started" << std::endl;
    
    while (m_running) {
        sockaddr_in clientAddr;
        #ifdef _WIN32
            int addrLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(m_serverSocket, (sockaddr*)&clientAddr, &addrLen);
            if (clientSocket == INVALID_SOCKET) {
        #else
            socklen_t addrLen = sizeof(clientAddr);
            int clientSocket = accept(m_serverSocket, (sockaddr*)&clientAddr, &addrLen);
            if (clientSocket < 0) {
        #endif
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        
        if (m_connections.size() >= GameLogic::MAX_PLAYERS) {
            std::cout << "Max connections reached, rejecting client" << std::endl;
            #ifdef _WIN32
                closesocket(clientSocket);
            #else
                ::close(clientSocket);
            #endif
            continue;
        }
        
        int connId = m_connections.size();
        
        // Set client socket to non-blocking mode
        #ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(clientSocket, FIONBIO, &mode);
        #else
            int flags = fcntl(clientSocket, F_GETFL, 0);
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
        #endif
        
        auto conn = std::make_unique<Connection>(clientSocket, connId);
        conn->setPlayerId(connId);
        
        std::cout << "Client connected: ID=" << connId << std::endl;
        m_connections.push_back(std::move(conn));
    }
}

int GameServer::getTotalControllerCount() const {
    int total = 0;
    for (const auto& conn : m_connections) {
        total += conn->getControllerCount();
    }
    return total;
}

void GameServer::gameLoop() {
    std::cout << "Game loop started" << std::endl;
    
    auto lastTick = std::chrono::steady_clock::now();
    const auto tickDuration = std::chrono::milliseconds(static_cast<int>(TICK_RATE * 1000));
    
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        
        // Handle incoming messages
        handleClientMessages();
        
        // Game tick
        if (now - lastTick >= tickDuration) {
            {
                std::lock_guard<std::mutex> lock(m_inputMutex);
                m_gameLogic.applyInputs(m_pendingInputs);
            }
            
            m_gameLogic.tick();
            broadcastGameState();
            
            lastTick = now;
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GameServer::recalculatePlayerIds() {
    /**
     * Recalculate and assign player IDs to all connections sequentially.
     * Should be called whenever connections change (join/leave) or after game reset.
     * 
     * Each connection gets a contiguous block of player IDs based on its controller count.
     * Example: PC1(2 ctrl) gets IDs [0,1], PC2(2 ctrl) gets IDs [2,3]
     */
    int nextId = 0;
    
    for (auto& conn : m_connections) {
        std::vector<int> playerIds;
        int controllerCount = conn->getControllerCount();
        
        // Assign sequential IDs to this connection
        for (int i = 0; i < controllerCount && nextId < GameLogic::MAX_PLAYERS; ++i) {
            playerIds.push_back(nextId++);
        }
        
        conn->setPlayerIds(playerIds);
        
        std::cout << "Connection " << conn->getId() << " assigned player IDs: ";
        for (int pid : playerIds) {
            std::cout << pid << " ";
        }
        std::cout << std::endl;
    }
    
    // Update game logic with total player count
    int totalControllers = getTotalControllerCount();
    if (totalControllers > 0) {
        m_gameLogic.init(totalControllers);
        std::cout << "Lobby recalculated with " << totalControllers << " player(s)" << std::endl;
    }
}

void GameServer::handleClientMessages() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    // Check for dead connections and remove them
    bool connectionsChanged = false;
    int oldConnectionCount = m_connections.size();
    
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [](const std::unique_ptr<Connection>& conn) {
                return !conn->isAlive();
            }),
        m_connections.end()
    );
    
    // If a connection was removed, recalculate IDs (but only if game is not active)
    if (m_connections.size() < oldConnectionCount) {
        connectionsChanged = true;
        std::cout << "Connection dropped. Active connections: " << m_connections.size() << std::endl;
        
        // Only recalculate if game is in LOBBY state (not during active game)
        if (!m_gameLogic.isGameActive()) {
            recalculatePlayerIds();
        }
    }
    
    // Process messages from each connection
    for (auto& conn : m_connections) {
        std::string data = conn->receive();
        if (data.empty()) continue;
        
        // Split by newlines - multiple messages may arrive in one recv
        std::istringstream stream(data);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            
            Protocol::Message msg = parseMessage(line);
        
            if (msg.type == Protocol::MessageType::CONNECT) {
                /**
                 * New client connected or reconnected.
                 * Update controller count and recalculate all IDs.
                 */
                conn->setControllerCount(msg.controllerCount);
                
                // Recalculate IDs for all connections (only if game not active)
                if (!m_gameLogic.isGameActive()) {
                    recalculatePlayerIds();
                    std::cout << "Total players ready: " << getTotalControllerCount() << std::endl;
                }
            }
            else if (msg.type == Protocol::MessageType::INPUT) {
                // Map local player ID to global player ID using connection's mapping
                int globalPlayerId = conn->getGlobalPlayerId(msg.playerId);
                
                if (globalPlayerId >= 0 && globalPlayerId < GameLogic::MAX_PLAYERS) {
                    std::lock_guard<std::mutex> inputLock(m_inputMutex);
                    m_pendingInputs[globalPlayerId].playerId = globalPlayerId;
                    m_pendingInputs[globalPlayerId].direction = msg.direction;
                }
            }
            else if (msg.type == Protocol::MessageType::START_GAME) {
                // Lock IDs and start the game
                if (!m_gameLogic.isGameActive()) {
                    std::cout << "Game started by player " << conn->getPlayerId() << std::endl;
                    m_gameLogic.startGame();
                    
                    // Clear pending inputs for a clean start
                    for (int i = 0; i < GameLogic::MAX_PLAYERS; ++i) {
                        m_pendingInputs[i].playerId = i;
                        m_pendingInputs[i].direction = Protocol::Direction::Right;
                    }
                }
            }
            else if (msg.type == Protocol::MessageType::RESET_GAME) {
                // Game ended, reset and recalculate IDs
                std::cout << "Game reset by player " << conn->getPlayerId() << std::endl;
                
                // Recalculate IDs based on current connections
                recalculatePlayerIds();
                
                // Clear pending inputs
                for (int i = 0; i < GameLogic::MAX_PLAYERS; ++i) {
                    m_pendingInputs[i].playerId = i;
                    m_pendingInputs[i].direction = Protocol::Direction::Right;
                }
            }
        } // end while(getline)
    }
}

void GameServer::broadcastGameState() {
    Protocol::GameState state = m_gameLogic.getState();
    
    // Count actual players in the game, not network connections
    int connectedCount = state.players.size();
    
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    std::string stateJson = serializeGameState(state, connectedCount);
    
    for (auto& conn : m_connections) {
        conn->send(stateJson);
    }
}

std::string GameServer::serializeGameState(const Protocol::GameState& state, int connectedCount) {
    std::ostringstream oss;
    oss << "{\"type\":\"state\",\"active\":" << (state.gameActive ? "true" : "false");
    oss << ",\"connected\":" << connectedCount;
    oss << ",\"state\":" << static_cast<int>(state.state);
    
    // Serialize players
    oss << ",\"players\":[";
    for (size_t i = 0; i < state.players.size(); ++i) {
        const auto& p = state.players[i];
        if (i > 0) oss << ",";
        oss << "{\"id\":" << p.id 
            << ",\"alive\":" << (p.alive ? "true" : "false")
            << ",\"dir\":" << static_cast<int>(p.dir)
            << ",\"score\":" << p.score
            << ",\"body\":[";
        for (size_t j = 0; j < p.body.size(); ++j) {
            if (j > 0) oss << ",";
            oss << "{\"x\":" << p.body[j].x << ",\"y\":" << p.body[j].y << "}";
        }
        oss << "]}";
    }
    oss << "]";
    
    // Serialize food
    oss << ",\"food\":[";
    for (size_t i = 0; i < state.food.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"x\":" << state.food[i].x << ",\"y\":" << state.food[i].y << "}";
    }
    oss << "]}\n";
    
    return oss.str();
}

Protocol::Message GameServer::parseMessage(const std::string& data) {
    Protocol::Message msg;
    msg.type = Protocol::MessageType::MSG_ERROR;
    
    // Simple JSON parsing (in production, use a proper JSON library)
    const size_t npos = static_cast<size_t>(-1);
    
    // Check for "connect" message with controller count
    size_t connectPos = data.find("\"type\":\"connect\"");
    if (connectPos != npos) {
        msg.type = Protocol::MessageType::CONNECT;
        
        // Extract controller count
        size_t ctrlPos = data.find("\"controllers\":");
        if (ctrlPos != npos) {
            ctrlPos += 14; // Skip "controllers":
            msg.controllerCount = std::stoi(data.substr(ctrlPos));
        } else {
            msg.controllerCount = 1;  // Default to 1 if not specified
        }
        
        return msg;
    }
    
    size_t typePos = data.find("\"type\":\"input\"");
    if (typePos != npos) {
        msg.type = Protocol::MessageType::INPUT;
        
        // Extract direction
        size_t dirPos = data.find("\"direction\":");
        if (dirPos != npos) {
            dirPos += 12; // Skip "direction":
            int dirValue = std::stoi(data.substr(dirPos, 1));
            msg.direction = static_cast<Protocol::Direction>(dirValue);
        }

        // Extract optional playerId
        size_t pidPos = data.find("\"playerId\":");
        if (pidPos != npos) {
            pidPos += 11; // Skip "playerId":
            msg.playerId = std::stoi(data.substr(pidPos));
        }
        
        return msg;
    }
    
    size_t startPos = data.find("\"type\":\"start\"");
    if (startPos != npos) {
        msg.type = Protocol::MessageType::START_GAME;
        return msg;
    }
    
    size_t resetPos = data.find("\"type\":\"reset\"");
    if (resetPos != npos) {
        msg.type = Protocol::MessageType::RESET_GAME;
        return msg;
    }
    
    return msg;
}

bool GameServer::isValidJson(const std::string& data) {
    // Basic validation
    return !data.empty() && data[0] == '{' && data[data.size() - 1] == '}';
}