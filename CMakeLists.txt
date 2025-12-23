cmake_minimum_required(VERSION 3.16)
project(GameLibraryLauncher VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Find ViGEmClient
find_path(VIGEM_INCLUDE_DIR ViGEm/Client.h
    PATHS 
        "C:/ViGEmClient/include"
        "C:/Program Files/ViGEmClient/include"
        "C:/vcpkg/installed/x64-windows/include"
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
# Source files organized by module
# ============================================================================

# Core application files
set(CORE_SOURCES
    src/core/main.cpp
    src/core/MainWindow.cpp
)

set(CORE_HEADERS
    src/core/MainWindow.h
)

# UI components
set(UI_SOURCES
    src/ui/VirtualControllerWindow.cpp
    src/ui/ControllerTab.cpp
    src/ui/GameLibraryTab.cpp
)

set(UI_HEADERS
    src/ui/VirtualControllerWindow.h
    src/ui/ControllerTab.h
    src/ui/GameLibraryTab.h
)

# Input sources
set(INPUT_SOURCES
    src/input/LocalInputSource.cpp
)

set(INPUT_HEADERS
    src/input/LocalInputSource.h
)

# Managers
set(MANAGER_SOURCES
    src/managers/MultiControllerManager.cpp
)

set(MANAGER_HEADERS
    src/managers/MultiControllerManager.h
)

# Interfaces
set(INTERFACE_HEADERS
    src/interfaces/IInputSource.h
)

# Scanner
set(SCANNER_SOURCES
    src/scanner/GameScanner.cpp
)

set(SCANNER_HEADERS
    src/scanner/GameScanner.h
    src/scanner/GameInfo.h
)

# Combine all sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${INPUT_SOURCES}
    ${MANAGER_SOURCES}
    ${SCANNER_SOURCES}
)

set(ALL_HEADERS
    ${CORE_HEADERS}
    ${UI_HEADERS}
    ${INPUT_HEADERS}
    ${MANAGER_HEADERS}
    ${INTERFACE_HEADERS}
    ${SCANNER_HEADERS}
)

# ============================================================================
# Create executable
# ============================================================================

add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${ALL_HEADERS})

# Add include directories for each module
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner
)

# Add ViGEm include if found
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

# ============================================================================
# Module summary
# ============================================================================

message(STATUS "=== VirtualController v2.0 Build Configuration ===")
message(STATUS "Modules:")
message(STATUS "  - Core:       ${CORE_SOURCES}")
message(STATUS "  - UI:         ${UI_SOURCES}")
message(STATUS "  - Input:      ${INPUT_SOURCES}")
message(STATUS "  - Managers:   ${MANAGER_SOURCES}")
message(STATUS "  - Scanner:    ${SCANNER_SOURCES}")
message(STATUS "  - Interfaces: ${INTERFACE_HEADERS}")
message(STATUS "==================================================")
