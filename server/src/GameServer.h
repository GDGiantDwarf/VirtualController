#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#include "GameLogic.h"
#include "Connection.h"
#include "Protocol.h"

/**
 * @brief Main game server that manages connections and game state
 */
class GameServer {
public:
    static constexpr int DEFAULT_PORT = 8765;
    static constexpr float TICK_RATE = 0.12f; // 120ms per game tick
    
    GameServer(int port = DEFAULT_PORT);
    ~GameServer();
    
    /**
     * @brief Start the server
     */
    bool start();
    
    /**
     * @brief Stop the server
     */
    void stop();
    
    /**
     * @brief Run the server (blocking)
     */
    void run();
    
private:
    int m_port;
    #ifdef _WIN32
        SOCKET m_serverSocket;
    #else
        int m_serverSocket;
    #endif
    
    std::atomic<bool> m_running{false};
    std::vector<std::unique_ptr<Connection>> m_connections;
    std::mutex m_connectionsMutex;
    
    GameLogic m_gameLogic;
    std::array<Protocol::InputCommand, GameLogic::MAX_PLAYERS> m_pendingInputs;
    std::mutex m_inputMutex;
    
    std::thread m_acceptThread;
    std::thread m_gameThread;
    
    bool initializeSocket();
    void acceptLoop();
    void gameLoop();
    void handleClientMessages();
    void broadcastGameState();
    
    std::string serializeGameState(const Protocol::GameState& state);
    Protocol::Message parseMessage(const std::string& data);
    bool isValidJson(const std::string& data);
};

#endif // GAMESERVER_H
