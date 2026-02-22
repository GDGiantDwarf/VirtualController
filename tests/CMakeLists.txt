cmake_minimum_required(VERSION 3.16)

# C++ Unit Tests using Google Test
# These test core logic components without requiring full application startup

project(VirtualControllerTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Google Test Setup
# ============================================================================

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ============================================================================
# Server Tests
# ============================================================================

# Test: Game Logic
add_executable(test_game_logic
    server/test_game_logic.cpp
    ../server/src/GameLogic.cpp
)
target_include_directories(test_game_logic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(test_game_logic PRIVATE
    GTest::gtest_main
)
add_test(NAME GameLogicTest COMMAND test_game_logic)

# Test: Player ID Assignment
add_executable(test_player_ids
    server/test_player_ids.cpp
)
target_include_directories(test_player_ids PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(test_player_ids PRIVATE
    GTest::gtest_main
)
add_test(NAME PlayerIdTest COMMAND test_player_ids)

# ============================================================================
# Launcher Tests
# ============================================================================

# Test: Controller Count Tracking (pure logic, no Qt/network required)
add_executable(test_controller_management
    launcher/test_server_sync.cpp
)
target_include_directories(test_controller_management PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(test_controller_management PRIVATE
    GTest::gtest_main
)
add_test(NAME ControllerManagementTest COMMAND test_controller_management)

# ============================================================================
# Snake Game Tests
# ============================================================================

# Test: Game Mechanics (pure logic, no SFML required)
add_executable(test_game_mechanics
    snake/test_game_mechanics.cpp
)
target_include_directories(test_game_mechanics PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(test_game_mechanics PRIVATE
    GTest::gtest_main
)
add_test(NAME GameMechanicsTest COMMAND test_game_mechanics)

# ============================================================================
# Karting Game Tests
# ============================================================================

# Test: Karting Mechanics (pure logic, no SFML required)
add_executable(test_karting_mechanics
    karting/test_karting_mechanics.cpp
)
target_include_directories(test_karting_mechanics PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(test_karting_mechanics PRIVATE
    GTest::gtest_main
)
add_test(NAME KartingMechanicsTest COMMAND test_karting_mechanics)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "C++ Unit Tests configured:")
message(STATUS "  - Server: GameLogic, PlayerIds")
message(STATUS "  - Launcher: ControllerManagement")
message(STATUS "  - Snake: GameMechanics")
message(STATUS "  - Karting: KartingMechanics")
message(STATUS "")
message(STATUS "Run tests with: ctest --test-dir build/tests")
