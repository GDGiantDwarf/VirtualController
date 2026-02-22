cmake_minimum_required(VERSION 3.16)
project(VirtualController VERSION 2.0 LANGUAGES CXX)

# Disable ZERO_CHECK target to prevent auto-rebuilds
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network WebSockets svg)

# Find ViGEmClient
find_path(VIGEM_INCLUDE_DIR ViGEm/Client.h
    PATHS 
        "C:/ViGEmClient/include"
        "C:/Program Files/ViGEmClient/include"
        "C:/vcpkg/installed/x64-windows/include"
        "${VIGEM_SDK}/include"
        "$ENV{VIGEM_SDK}/include"
    NO_DEFAULT_PATH
)

find_library(VIGEM_LIBRARY
    NAMES ViGEmClient
    PATHS 
        "C:/ViGEmClient/build/lib/Release"
        "C:/ViGEmClient/build/Release"
        "C:/ViGEmClient/lib/x64"
        "C:/Program Files/ViGEmClient/lib"
        "C:/vcpkg/installed/x64-windows/lib"
        "${VIGEM_SDK}/lib/x64"
        "$ENV{VIGEM_SDK}/lib/x64"
    NO_DEFAULT_PATH
)

if(VIGEM_INCLUDE_DIR AND VIGEM_LIBRARY)
    message(STATUS "Found ViGEmClient include: ${VIGEM_INCLUDE_DIR}")
    message(STATUS "Found ViGEmClient library: ${VIGEM_LIBRARY}")
    set(VIGEM_FOUND TRUE)
else()
    message(WARNING "ViGEmClient not found.")
    set(VIGEM_FOUND FALSE)
endif()

# ============================================================================
# Build components
# ============================================================================

# Desktop launcher application (optional - requires ViGEmClient)
option(BUILD_LAUNCHER "Build desktop launcher (requires ViGEmClient)" ON)
if(BUILD_LAUNCHER)
    add_subdirectory(launcher)
endif()

# Game server (optional, can be built separately)
add_subdirectory(server)

# Games (optional - requires SFML)
option(BUILD_GAMES "Build game clients (requires SFML)" ON)
if(BUILD_GAMES)
    add_subdirectory(games/snake)
    add_subdirectory(games/karting)
endif()

# C++ Unit Tests (Google Test)
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "=== VirtualController v2.0 Build Configuration ===")
message(STATUS "Components:")
message(STATUS "  - Desktop Launcher (Qt6)  : launcher/")
message(STATUS "  - Game Server (TCP)        : server/")
message(STATUS "  - Snake Game Client (SFML) : games/snake/")
message(STATUS "  - Karting Game Client (SFML) : games/karting/")
message(STATUS "  - Mobile Controller       : mobile_controller/")
if(BUILD_TESTS)
    message(STATUS "  - C++ Unit Tests (GTest)  : tests/")
endif()
message(STATUS "==================================================")
