cmake_minimum_required(VERSION 3.16)
project(GameServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
# Note: Using header-only libraries for WebSocket support
# You can use cpp-httplib or similar. For this example, we'll use Boost.Beast (optional)
# or a simpler approach with raw sockets and a lightweight JSON library

# For now, we'll use nlohmann/json (header-only) and implement basic socket server
# Add nlohmann/json - can be installed via vcpkg or manually added
find_package(nlohmann_json CONFIG)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found, will use fallback or add manually")
    # You can add it as a subdirectory or download it
endif()

# Server sources
set(SERVER_SOURCES
    src/main.cpp
    src/GameServer.cpp
    src/GameLogic.cpp
    src/Connection.cpp
)

set(SERVER_HEADERS
    src/GameServer.h
    src/GameLogic.h
    src/Connection.h
    src/Protocol.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SERVER_SOURCES} ${SERVER_HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

message(STATUS "=== Game Server Build Configuration ===")
message(STATUS "Sources: ${SERVER_SOURCES}")
message(STATUS "Headers: ${SERVER_HEADERS}")
message(STATUS "========================================")
