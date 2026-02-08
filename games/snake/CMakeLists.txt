cmake_minimum_required(VERSION 3.16)
project(snake VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain file is passed from root CMakeLists.txt
# via CMAKE_TOOLCHAIN_FILE - no need to set it here

# Find SFML (vcpkg will provide this automatically)
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

# Build networked client as "snake.exe" (launcher will find this)
add_executable(snake snake.cpp)

# Link SFML libraries
target_link_libraries(snake 
    SFML::Graphics
    SFML::Window
    SFML::System
)

# Windows socket library for TCP networking
if(WIN32)
    target_link_libraries(snake ws2_32)
endif()

# Set output directory
set_target_properties(snake PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Copy SFML DLLs to output directory after build
# (vcpkg-managed DLLs will be automatically found)
add_custom_command(TARGET snake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        "$<TARGET_FILE_DIR:snake>"
    COMMENT "Copying SFML DLLs to output directory"
)

message(STATUS "Snake game configured with SFML from vcpkg")