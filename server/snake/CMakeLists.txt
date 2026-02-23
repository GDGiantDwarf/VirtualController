cmake_minimum_required(VERSION 3.16)
project(SnakeGameServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find nlohmann/json (optional, fallback to manual JSON parsing)
find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found, using built-in JSON parsing")
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

# Set output directory (unified build output)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

message(STATUS "=== Snake Game Server Build Configuration ===")
message(STATUS "Output: SnakeGameServer.exe")
message(STATUS "Location: ${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
message(STATUS "==============================================")
