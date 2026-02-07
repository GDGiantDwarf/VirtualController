#include "GameServer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #pragma comment(lib, "ws2_32.lib")
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
        auto conn = std::make_unique<Connection>(clientSocket, connId);
        conn->setPlayerId(connId);
        
        std::cout << "Client connected: ID=" << connId << std::endl;
        m_connections.push_back(std::move(conn));
        
        // Start game if we have at least one player and game not started
        if (m_connections.size() == 1 && !m_gameLogic.isGameActive()) {
            m_gameLogic.init(GameLogic::MAX_PLAYERS);
            std::cout << "Game initialized with " << GameLogic::MAX_PLAYERS << " players" << std::endl;
        }
    }
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

void GameServer::handleClientMessages() {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    // Remove dead connections
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [](const std::unique_ptr<Connection>& conn) {
                return !conn->isAlive();
            }),
        m_connections.end()
    );
    
    // Process messages from each connection
    for (auto& conn : m_connections) {
        std::string data = conn->receive();
        if (data.empty()) continue;
        
        Protocol::Message msg = parseMessage(data);
        
        if (msg.type == Protocol::MessageType::INPUT) {
            std::lock_guard<std::mutex> inputLock(m_inputMutex);
            int playerId = msg.playerId >= 0 ? msg.playerId : conn->getPlayerId();
            if (playerId >= 0 && playerId < GameLogic::MAX_PLAYERS) {
                m_pendingInputs[playerId].playerId = playerId;
                m_pendingInputs[playerId].direction = msg.direction;
            }
        }
    }
}

void GameServer::broadcastGameState() {
    Protocol::GameState state = m_gameLogic.getState();
    std::string stateJson = serializeGameState(state);
    
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    for (auto& conn : m_connections) {
        conn->send(stateJson);
    }
}

std::string GameServer::serializeGameState(const Protocol::GameState& state) {
    std::ostringstream oss;
    oss << "{\"type\":\"state\",\"active\":" << (state.gameActive ? "true" : "false");
    
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
    }
    
    return msg;
}

bool GameServer::isValidJson(const std::string& data) {
    // Basic validation
    return !data.empty() && data[0] == '{' && data[data.size() - 1] == '}';
}
