@echo off
echo ================================================
echo Game Library Launcher - Build Script (Windows)
echo ================================================
echo.

if not exist "build" mkdir build
cd build

echo.
echo Configuring CMake...
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.10.1\msvc2022_64"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Please verify your Qt6 installation path
    pause
    exit /b 1
)

echo.
echo Building project (Release)...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Deploying Qt dependencies to build directory...
cd ..
C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe --release --no-translations "build\bin\Release\GameLibraryLauncher.exe"

if %errorlevel% neq 0 (
    echo WARNING: windeployqt failed
    pause
    exit /b 1
)

echo.
echo Creating shortcut to executable...
powershell -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%cd%\GameLibraryLauncher.lnk'); $Shortcut.TargetPath = '%cd%\build\bin\Release\GameLibraryLauncher.exe'; $Shortcut.WorkingDirectory = '%cd%'; $Shortcut.Save()"

if %errorlevel% neq 0 (
    echo WARNING: Could not create shortcut
) else (
    echo Shortcut created successfully!
)

echo.
echo ================================================
echo Build completed successfully!
echo.
echo Shortcut location: GameLibraryLauncher.lnk
echo Executable location: build\bin\Release\GameLibraryLauncher.exe
echo DLLs location: build\bin\Release\
echo.
echo To run: Double-click GameLibraryLauncher.lnk
echo ================================================
pause
```