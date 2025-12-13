cmake_minimum_required(VERSION 3.16)
project(tester VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Si vous utilisez vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find SFML (version 3.0 compatible)
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

# Create executable
add_executable(tester tester.cpp)

# Link SFML
target_link_libraries(tester 
    SFML::Graphics
    SFML::Window
    SFML::System
)

# Set output directory 
set_target_properties(tester PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)