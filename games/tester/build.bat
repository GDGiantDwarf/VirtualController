@echo off
echo ================================================
echo tester Game - Build Script
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
copy /Y "build\bin\Release\tester.exe" "tester.exe"

if %errorlevel% neq 0 (
    echo ERROR: Could not find tester.exe
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
echo Executable: tester.exe
echo.
echo To run: .\tester.exe
echo ================================================
pause