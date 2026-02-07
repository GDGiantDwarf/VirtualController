#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <functional>
#include <memory>
#ifdef _WIN32
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

/**
 * @brief Represents a single client connection to the game server
 */
class Connection {
public:
    #ifdef _WIN32
        using SocketHandle = SOCKET;
    #else
        using SocketHandle = int;
    #endif
    
    explicit Connection(SocketHandle socket, int id);
    ~Connection();
    
    /**
     * @brief Send data to this connection
     */
    bool send(const std::string& data);
    
    /**
     * @brief Receive data from this connection (non-blocking)
     */
    std::string receive();
    
    /**
     * @brief Check if connection is still alive
     */
    bool isAlive() const;
    
    /**
     * @brief Close the connection
     */
    void close();
    
    /**
     * @brief Get connection ID
     */
    int getId() const { return m_id; }
    
    /**
     * @brief Set player ID associated with this connection
     */
    void setPlayerId(int playerId) { m_playerId = playerId; }
    
    /**
     * @brief Get player ID associated with this connection
     */
    int getPlayerId() const { return m_playerId; }
    
private:
    SocketHandle m_socket;
    int m_id;
    int m_playerId{-1};
    bool m_alive{true};
};

#endif // CONNECTION_H
