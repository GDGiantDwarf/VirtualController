@echo off
echo ================================================
echo VirtualController - Complete Build Script
echo ================================================
echo.

REM Set vcpkg toolchain path (adjust if your vcpkg is elsewhere)
set VCPKG_ROOT=C:\vcpkg
set VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM Check if vcpkg exists
if not exist "%VCPKG_TOOLCHAIN%" (
    echo ERROR: vcpkg toolchain not found at %VCPKG_TOOLCHAIN%
    echo Please install vcpkg or adjust VCPKG_ROOT in this script
    echo.
    echo To install vcpkg:
    echo   git clone https://github.com/microsoft/vcpkg
    echo   cd vcpkg
    echo   bootstrap-vcpkg.bat
    echo   vcpkg install sfml:x64-windows
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

echo.
echo Configuring CMake with vcpkg...
echo This will build: Launcher, Snake Game, and Server
echo.
cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" ^
    -DCMAKE_PREFIX_PATH="C:\Qt\6.10.1\msvc2022_64"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Common issues:
    echo   - Qt6 not found: Check Qt installation path
    echo   - SFML not found: Run 'vcpkg install sfml:x64-windows'
    echo   - ViGEmClient not found: This is optional
    pause
    exit /b 1
)

echo.
echo Building all projects (Release)...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Deploying Qt dependencies to launcher...
cd ..
C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe --release --no-translations "build\bin\Release\GameLibraryLauncher.exe"

if %errorlevel% neq 0 (
    echo WARNING: windeployqt failed - you may need to copy Qt DLLs manually
)

echo.
echo ================================================
echo Build completed successfully!
echo ================================================
echo.
echo Executables built:
echo   - Launcher: build\bin\Release\GameLibraryLauncher.exe
echo   - Snake:    build\bin\Release\snake.exe
echo   - Server:   build\bin\Release\GameServer.exe
echo.
echo To run the launcher from PowerShell, execute:
echo   .\build\bin\Release\GameLibraryLauncher.exe
echo ================================================
pause