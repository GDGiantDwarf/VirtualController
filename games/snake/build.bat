@echo off
echo ================================================
echo Snake Game - Build Script
echo ================================================
echo.

if not exist "build" mkdir build
cd build

echo.
echo Configuring CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Copying executable to game directory...
cd ..
copy /Y "build\bin\Release\snake.exe" "snake.exe"

if %errorlevel% neq 0 (
    echo ERROR: Could not find snake.exe
    pause
    exit /b 1
)

echo.
echo Copying all required DLLs from vcpkg...
REM Copy all DLLs from vcpkg bin directory
xcopy /Y /I "C:\vcpkg\installed\x64-windows\bin\*.dll" .

echo.
echo ================================================
echo Build completed!
echo Executable: snake.exe
echo.
echo To run: .\snake.exe
echo ================================================
pause