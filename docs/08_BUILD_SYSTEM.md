# Build System

Complete guide to the CMake-based build system and how to compile each component.

## Architecture Overview

VirtualController uses a unified CMake build system where all components output to a single directory: `build/bin/Release/`.

```
Root CMakeLists.txt
├── launcher/CMakeLists.txt
├── server/CMakeLists.txt
└── games/
    └── snake/CMakeLists.txt
    
All output → build/bin/Release/
```

**Key Principle**: No component has its own `build/` directory. This ensures:
- Portability (single directory contains everything)
- Simplicity (one place to copy and deploy)
- Game discovery (launcher scans one folder)

## Prerequisites

### Required Software

| Tool | Version | Purpose |
|------|---------|---------|
| Visual Studio 2022 | Latest | C++ compiler and build tools |
| CMake | 3.16+ | Build configuration |
| Qt | 6.10+ | Launcher UI framework |
| SFML | 3.0+ | Game graphics library |
| vcpkg | Latest | Package manager (optional but recommended) |

### Environment Setup

These must be findable by CMake:

1. **Visual Studio 2022** - Installed in default location
2. **Qt 6.10+** - Add to PATH or set `Qt6_DIR` environment variable:
   ```bash
   set Qt6_DIR=C:\Qt\6.10.0\msvc2022_64\lib\cmake\Qt6
   ```

3. **vcpkg** - Recommended for SFML and dependencies:
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   .\vcpkg\vcpkg install sfml:x64-windows
   ```

## Building

### Quick Build (Recommended)

Use the provided batch script from **root directory only**:

```bash
.\build.bat
```

This script:
1. Creates `build/` directory if needed
2. Runs CMake configure
3. Builds all targets in Release mode
4. Outputs to `build/bin/Release/`

### Manual Build

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64

# Build all targets (default: Release)
cmake --build . --config Release

# Or build specific target
cmake --build . --config Release --target GameServer
cmake --build . --config Release --target GameLibraryLauncher
cmake --build . --config Release --target snake
```

### Build Individual Components

```bash
# From root directory (build/ must exist)

# Server only
cmake --build build --config Release --target GameServer

# Launcher only
cmake --build build --config Release --target GameLibraryLauncher

# Snake game only
cmake --build build --config Release --target snake

# All games
cmake --build build --config Release --target snake  # (currently only one)
```

## Root CMakeLists.txt

Main build configuration that orchestrates all components.

```cmake
cmake_minimum_required(VERSION 3.16)
project(VirtualController LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets)
find_package(SFML 3.0 REQUIRED COMPONENTS Graphics Window System)

# Add all components
add_subdirectory(launcher)
add_subdirectory(server)
add_subdirectory(games/snake)
```

### Key Variables

```cmake
CMAKE_BINARY_DIR          # Root build directory (build/)
CMAKE_SOURCE_DIR          # Root source directory (VirtualController/)
CMAKE_CXX_STANDARD        # C++ version (17)
RUNTIME_OUTPUT_DIRECTORY  # Where executables go
```

## Component-Specific Build Configs

### Launcher (Qt6 Application)

**Location**: `launcher/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(GameLibraryLauncher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)  # Auto-generate Qt moc files
set(CMAKE_AUTORCC ON)  # Auto-generate resource files

find_package(Qt6 REQUIRED COMPONENTS 
    Core Gui Widgets Network WebSockets)

add_executable(GameLibraryLauncher
    src/main.cpp
    src/scanner/GameScanner.cpp
    src/ui/MainWindow.cpp
    # ... other sources
)

target_link_libraries(GameLibraryLauncher
    Qt6::Core Qt6::Gui Qt6::Widgets
    Qt6::Network Qt6::WebSockets)

set_target_properties(GameLibraryLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Deploy Qt runtime (Windows)
if(WIN32)
    add_custom_command(TARGET GameLibraryLauncher POST_BUILD
        COMMAND ${Qt6_BIN_DIR}/windeployqt --no-compiler-runtime
            "$<TARGET_FILE:GameLibraryLauncher>"
        COMMENT "Deploying Qt runtime"
    )
endif()
```

**Build Time**: ~30 seconds
**Output**: `build/bin/Release/GameLibraryLauncher.exe`

### Server

**Location**: `server/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(GameServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(GameServer
    src/main.cpp
    src/GameServer.cpp
    src/GameLogic.cpp
    src/Connection.cpp
)

if(WIN32)
    target_link_libraries(GameServer ws2_32)  # Windows sockets
endif()

set_target_properties(GameServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)
```

**Build Time**: ~10 seconds
**Output**: `build/bin/Release/GameServer.exe`
**No runtime dependencies** (pure C++ std library + OS sockets)

### Snake Game

**Location**: `games/snake/CMakeLists.txt` and `server/snake/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(snake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

add_executable(snake snake.cpp)

target_link_libraries(snake 
    SFML::Graphics SFML::Window SFML::System)

if(WIN32)
    target_link_libraries(snake ws2_32)
endif()

set_target_properties(snake PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Copy SFML DLLs
add_custom_command(TARGET snake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        "$<TARGET_FILE_DIR:snake>"
    COMMENT "Copying SFML DLLs"
)

# Copy icon
add_custom_command(TARGET snake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/snake.ico"
        "$<TARGET_FILE_DIR:snake>/snake.ico"
    COMMENT "Copying snake.ico"
)
```

**Build Time**: ~15 seconds
**Output**: `build/bin/Release/snake.exe`
**Runtime Dependencies**: SFML DLLs (copied at build time)

### Karting Game

**Location**: `games/karting/CMakeLists.txt` and `server/karting/CMakeLists.txt`

Similar structure to Snake but with separate server implementation for physics simulation.

**Build Time**: ~15 seconds
**Output**: `build/bin/Release/karting.exe`
**Runtime Dependencies**: SFML DLLs (copied at build time)

## Adding a New Game

Add to root CMakeLists.txt:

```cmake
# At the end of CMakeLists.txt
add_subdirectory(games/my_game)
```

Create `games/my_game/CMakeLists.txt` following the Snake template:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

add_executable(my_game my_game.cpp)

target_link_libraries(my_game 
    SFML::Graphics SFML::Window SFML::System)

if(WIN32)
    target_link_libraries(my_game ws2_32)
endif()

set_target_properties(my_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

# Copy SFML DLLs
add_custom_command(TARGET my_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        "$<TARGET_FILE_DIR:my_game>"
    COMMENT "Copying SFML DLLs"
)

# Copy icon
add_custom_command(TARGET my_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/my_game.ico"
        "$<TARGET_FILE_DIR:my_game>/my_game.ico"
    COMMENT "Copying game icon"
)
```

Then build:
```bash
cmake --build build --config Release --target my_game
```

## Build Artifacts

### Output Directory Structure

```
build/bin/Release/
├── GameLibraryLauncher.exe          # Main launcher
├── SnakeGameServer.exe              # Snake game server
├── KartingGameServer.exe            # Karting game server
├── snake.exe                        # Snake game client
├── snake.ico                        # Game icon
├── karting.exe                      # Karting game client
├── karting.ico                      # Game icon
├── Qt6Core.dll                      # Qt libraries
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6Network.dll
├── sfml-graphics-3.dll              # SFML libraries
├── sfml-window-3.dll
├── sfml-system-3.dll
├── icuuc.dll                        # Qt dependencies
├── D3Dcompiler_47.dll
├── opengl32sw.dll
├── dxcompiler.dll
├── dxil.dll
└── [other standard Windows DLLs]
```

### Post-Build Actions

Each target runs post-build commands:

| Target | Action | Purpose |
|--------|--------|---------|
| Launcher | `windeployqt` | Copy Qt runtime |
| Server | None | Pure C++ binary |
| Snake | Copy SFML DLLs | Graphics runtime |
| Snake | Copy .ico file | Game icon |

These ensure all dependencies are in the output directory.

## Build Modes

### Release (Recommended)

```bash
cmake --build build --config Release
```

- Optimizations enabled
- Smallest binary size
- Best performance
- No debug symbols

### Debug

```bash
cmake --build build --config Debug
```

- Debug symbols included
- Slower execution
- Larger binary
- Use with debugger (Visual Studio)

## Troubleshooting Build Issues

### CMake Configuration Fails

**Error**: `Could not find Qt6`

**Solution**: Set Qt6_DIR environment variable:
```bash
set Qt6_DIR=C:\Qt\6.10.0\msvc2022_64\lib\cmake\Qt6
# Then reconfigure:
cd build && cmake ..
```

**Error**: `Could not find SFML`

**Solution**: Use vcpkg or install SFML manually, then set search path:
```bash
cmake .. -DSFML_DIR=C:\path\to\SFML\lib\cmake\SFML
```

### Build Fails with Link Errors

**Error**: `unresolved external symbol`

**Solution**: 
- Check all dependencies are installed
- Clear build directory and reconfigure:
  ```bash
  rm -r build
  mkdir build
  cd build
  cmake ..
  ```

### Post-Build Commands Fail

**Error**: Icon or DLL not copied

**Solution**:
- Verify file paths in CMakeLists.txt
- Run manually to check:
  ```bash
  cmake -E copy my_game.ico build/bin/Release/
  ```

### Dependencies Missing at Runtime

**Error**: Game won't run, DLL errors

**Solution**:
- Verify post-build commands executed (check build output)
- Manual copy if needed:
  ```bash
  copy games\snake\snake.ico build\bin\Release\
  ```

## Build Customization

### Custom Output Directory

Override in CMakeLists.txt:

```cmake
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/custom")
```

### Enable Address Sanitizer

For debugging memory issues:

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
```

### Parallel Build

Use all CPU cores:

```bash
cmake --build build --config Release -j 8
```

## Clean Build

Remove all artifacts and reconfigure:

```bash
# Windows
rmdir /s build
mkdir build
cd build
cmake ..

# Linux/Mac
rm -rf build
mkdir build
cd build
cmake ..
```

## Cross-Platform Considerations

### Windows (Primary)

- Uses MSVC compiler
- Uses Windows sockets (ws2_32)
- DLL runtime dependencies
- Visual Studio as standard IDE

### Linux

Would require:
- GCC/Clang compiler
- OpenGL drivers installed
- Qt6 packages: `libqt6gui6-dev`, etc.
- SFML from package manager or built from source

### macOS

Would require:
- Xcode command line tools
- Homebrew for dependencies: `brew install qt@6 sfml`

Current project is Windows-focused but structured for portability.

## Performance Tips

1. **Incremental builds**: Only changed targets rebuild
2. **Precompiled headers**: Consider for large C++ files
3. **Link-time optimization**: Add `/LTCG` for Release builds
4. **Parallel compilation**: Use `-j` flag with CMake

## Next Steps

- First build: `./build.bat`
- Check output in `build/bin/Release/`
- Run GameServer and launcher
- See [Troubleshooting](09_DEBUGGING_AND_TESTING.md) if issues

## Reference Documentation

- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [Qt 6 Build Guide](https://doc.qt.io/qt-6/)
- [SFML Building](https://www.sfml-dev.org/tutorials/2.6/)
- [vcpkg Package Manager](https://github.com/Microsoft/vcpkg)
