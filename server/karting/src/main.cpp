#include "GameServer.h"
#include <iostream>
#include <csignal>

// Global server pointer for signal handling
GameServer* g_server = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    int port = GameServer::DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port number" << std::endl;
            return 1;
        }
    }
    
    std::cout << "=== Game Server ===" << std::endl;
    std::cout << "Starting server on port " << port << std::endl;
    
    GameServer server(port);
    g_server = &server;
    
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
    
    // Run server (blocking)
    server.run();
    
    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}
