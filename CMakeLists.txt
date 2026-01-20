cmake_minimum_required(VERSION 3.16)
project(VirtualController VERSION 2.0 LANGUAGES CXX)

# Disable ZERO_CHECK target to prevent auto-rebuilds
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Build components
# ============================================================================

# Desktop launcher application
add_subdirectory(launcher)

# Game server (optional, can be built separately)
add_subdirectory(server)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "=== VirtualController v2.0 Build Configuration ===")
message(STATUS "Components:")
message(STATUS "  - Desktop Launcher (Qt6)  : launcher/")
message(STATUS "  - Game Server (TCP)        : server/")
message(STATUS "  - Snake Game Client (SFML) : games/snake/")
message(STATUS "==================================================")
