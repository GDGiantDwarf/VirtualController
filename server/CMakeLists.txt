cmake_minimum_required(VERSION 3.16)
project(GameServers VERSION 1.0 LANGUAGES CXX)

# Game server implementations

# Snake game server
add_subdirectory(snake)

# Karting game server
add_subdirectory(karting)

message(STATUS "=== Game Servers Build Configuration ===")
message(STATUS "  - Snake Game Server: server/snake/")
message(STATUS "  - Karting Game Server: server/karting/")
message(STATUS "========================================")
