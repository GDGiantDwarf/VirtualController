#include "Connection.h"
#include <cstring>

Connection::Connection(SocketHandle socket, int id)
    : m_socket(socket), m_id(id), m_alive(true) {
}

Connection::~Connection() {
    close();
}

bool Connection::send(const std::string& data) {
    if (!m_alive) return false;
    
    const char* buf = data.c_str();
    size_t total = data.size();
    size_t sent = 0;
    
    while (sent < total) {
        #ifdef _WIN32
            int result = ::send(m_socket, buf + sent, static_cast<int>(total - sent), 0);
        #else
            ssize_t result = ::send(m_socket, buf + sent, total - sent, 0);
        #endif
        
        if (result <= 0) {
            m_alive = false;
            return false;
        }
        sent += result;
    }
    
    return true;
}

std::string Connection::receive() {
    if (!m_alive) return "";
    
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    
    #ifdef _WIN32
        int result = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    #else
        ssize_t result = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    #endif
    
    if (result <= 0) {
        if (result == 0) {
            // Connection closed gracefully
            m_alive = false;
        }
        return "";
    }
    
    return std::string(buffer, result);
}

bool Connection::isAlive() const {
    return m_alive;
}

void Connection::close() {
    if (m_alive) {
        #ifdef _WIN32
            closesocket(m_socket);
        #else
            ::close(m_socket);
        #endif
        m_alive = false;
    }
}
