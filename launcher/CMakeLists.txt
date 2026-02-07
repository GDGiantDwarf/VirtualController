cmake_minimum_required(VERSION 3.16)
project(GameLibraryLauncher VERSION 2.0 LANGUAGES CXX)

# Disable ZERO_CHECK target to prevent auto-rebuilds
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Qt6 Configuration
# ============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network WebSockets Svg)

# ============================================================================
# ViGEmClient Configuration
# ============================================================================
if(DEFINED VIGEM_SDK)
    find_path(VIGEM_INCLUDE_DIR
        NAMES ViGEm/Client.h
        PATHS "${VIGEM_SDK}"
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
    )
    find_library(VIGEM_LIBRARY
        NAMES ViGEmClient
        PATHS "${VIGEM_SDK}"
        PATH_SUFFIXES lib build/Release build_vs2022/Release
        NO_DEFAULT_PATH
    )
endif()

if(VIGEM_INCLUDE_DIR AND VIGEM_LIBRARY)
    add_compile_definitions(VIGEM_AVAILABLE)
    message(STATUS "ViGEmClient found:")
    message(STATUS "  Include: ${VIGEM_INCLUDE_DIR}")
    message(STATUS "  Library: ${VIGEM_LIBRARY}")
else()
    message(WARNING "ViGEmClient not found - virtual controller features will be disabled")
    message(STATUS "  Set VIGEM_SDK to enable: -DVIGEM_SDK=<path>")
endif()

# ============================================================================
# Source Files
# ============================================================================
set(LAUNCHER_SOURCES
    src/core/main.cpp
    src/core/MainWindow.cpp
    src/ui/VirtualControllerWindow.cpp
    src/ui/ControllerTab.cpp
    src/ui/GameLibraryTab.cpp
    src/ui/WebSocketTab.cpp
    src/input/LocalInputSource.cpp
    src/managers/MultiControllerManager.cpp
    src/scanner/GameScanner.cpp
    ../src/network/WebSocketInputSource.cpp
    ../src/qrcode/QrCodeGenerator.cpp
    ../src/qrcode/qrcodegen/qrcodegen.cpp
)

set(LAUNCHER_HEADERS
    src/core/MainWindow.h
    src/ui/VirtualControllerWindow.h
    src/ui/ControllerTab.h
    src/ui/GameLibraryTab.h
    src/ui/WebSocketTab.h
    src/input/LocalInputSource.h
    src/managers/MultiControllerManager.h
    src/interfaces/IInputSource.h
    src/scanner/GameScanner.h
    src/scanner/GameInfo.h
    ../src/network/WebSocketInputSource.h
    ../src/qrcode/QrCodeGenerator.h
    ../src/qrcode/qrcodegen/qrcodegen.h
)

# ============================================================================
# Executable Target
# ============================================================================
add_executable(${PROJECT_NAME}
    WIN32
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_HEADERS}
)

# ============================================================================
# Include Directories
# ============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/qrcode
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/qrcode/qrcodegen
)

if(VIGEM_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${VIGEM_INCLUDE_DIR})
endif()

# ============================================================================
# Compiler Flags
# ============================================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /Zc:__cplusplus
        /utf-8
        /external:W0
        $<$<CONFIG:Release>:/O2 /Ob2>
        $<$<CONFIG:Debug>:/Zi /Od>
    )
endif()

# ============================================================================
# Link Libraries
# ============================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::WebSockets
    Qt6::Svg
)

if(VIGEM_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${VIGEM_LIBRARY}
        setupapi.lib
    )
endif()

# ============================================================================
# Output Directory
# ============================================================================
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# ============================================================================
# Qt Deployment (windeployqt)
# ============================================================================
if(Qt6_FOUND)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime with windeployqt"
        )
    endif()
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "=== GameLibraryLauncher Configuration ===")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "ViGEmClient: ${VIGEM_INCLUDE_DIR}")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "=========================================")
