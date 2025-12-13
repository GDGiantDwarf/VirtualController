cmake_minimum_required(VERSION 3.16)
project(GameLibraryLauncher VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Find ViGEmClient
# Try to find ViGEmClient headers
find_path(VIGEM_INCLUDE_DIR ViGEm/Client.h
    PATHS 
        "C:/ViGEmClient/include"
        "C:/Program Files/ViGEmClient/include"
        "C:/vcpkg/installed/x64-windows/include"
        "$ENV{VIGEM_SDK}/include"
    NO_DEFAULT_PATH
)

# Try to find ViGEmClient library
find_library(VIGEM_LIBRARY
    NAMES ViGEmClient
    PATHS 
        "C:/ViGEmClient/build/lib/Release"
        "C:/ViGEmClient/build/Release"
        "C:/ViGEmClient/lib/x64"
        "C:/Program Files/ViGEmClient/lib"
        "C:/vcpkg/installed/x64-windows/lib"
        "$ENV{VIGEM_SDK}/lib/x64"
    NO_DEFAULT_PATH
)

if(VIGEM_INCLUDE_DIR AND VIGEM_LIBRARY)
    message(STATUS "Found ViGEmClient include: ${VIGEM_INCLUDE_DIR}")
    message(STATUS "Found ViGEmClient library: ${VIGEM_LIBRARY}")
    set(VIGEM_FOUND TRUE)
else()
    message(WARNING "ViGEmClient not found.")
    if(NOT VIGEM_INCLUDE_DIR)
        message(WARNING "  - Include directory not found. Expected: C:/ViGEmClient/include/ViGEm/Client.h")
    endif()
    if(NOT VIGEM_LIBRARY)
        message(WARNING "  - Library not found. Expected: C:/ViGEmClient/build/lib/Release/ViGEmClient.lib")
    endif()
    message(WARNING "Virtual gamepad functionality will be disabled.")
    set(VIGEM_FOUND FALSE)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/GameLibraryTab.cpp
    src/ControllerTab.cpp
    src/VirtualControllerWindow.cpp
    src/VirtualGamepadManager.cpp
    src/GameScanner.cpp
)

set(HEADERS
    src/MainWindow.h
    src/GameLibraryTab.h
    src/ControllerTab.h
    src/VirtualControllerWindow.h
    src/VirtualGamepadManager.h
    src/GameScanner.h
    src/GameInfo.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Add include directories
if(VIGEM_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${VIGEM_INCLUDE_DIR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE VIGEM_AVAILABLE)
endif()

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

# Link ViGEmClient if found
if(VIGEM_FOUND)
    target_link_libraries(${PROJECT_NAME} ${VIGEM_LIBRARY})
    # Windows libraries required by ViGEm
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} setupapi)
    endif()
    message(STATUS "ViGEmClient will be linked")
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)