cmake_minimum_required(VERSION 3.16)
project(KartingGameServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML for image loading (headless - no window/graphics needed, just sf::Image)
find_package(SFML 3.0 COMPONENTS Graphics QUIET)

if(NOT SFML_FOUND)
    message(WARNING "SFML not found. Karting server needs SFML for track collision detection.")
    message(WARNING "Please install SFML 3.0 or set SFML_DIR to SFML's CMake config location.")
    message(WARNING "Skipping KartingGameServer target because SFML is required.")
    return()
endif()

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

if(SFML_FOUND)
    target_link_libraries(${PROJECT_NAME} SFML::Graphics)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
endif()

# Set output directory (unified build output)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying karting server assets to build directory"
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
install(DIRECTORY assets DESTINATION bin)

message(STATUS "=== Karting Game Server Build Configuration ===")
message(STATUS "Output: KartingGameServer.exe")
message(STATUS "Location: ${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
message(STATUS "SFML Found: ${SFML_FOUND}")
message(STATUS "===============================================")
