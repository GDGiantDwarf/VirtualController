cmake_minimum_required(VERSION 3.16)
project(snake VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: If using vcpkg, set the toolchain file
# set(CMAKE_TOOLCHAIN_FILE "<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find SFML (version 3.0 compatible)
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

# Build networked client as "snake.exe" (launcher will find this)
add_executable(snake snake_client.cpp)

# Link SFML and socket libraries
target_link_libraries(snake 
    SFML::Graphics
    SFML::Window
    SFML::System
)

if(WIN32)
    target_link_libraries(snake ws2_32)
endif()

# Set output directory 
set_target_properties(snake PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Automatically copy SFML DLLs to output directory after build
add_custom_command(TARGET snake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE:SFML::Audio>
        $<TARGET_FILE:SFML::Network>
        "$<TARGET_FILE_DIR:snake>"
    COMMENT "Copying SFML DLLs to output directory"
)