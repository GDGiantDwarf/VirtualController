# Launcher and Game Discovery

Complete guide to the GameLibraryLauncher, how games are discovered, and how the UI displays available games.

## Launcher Overview

The GameLibraryLauncher is the main entry point for playing games. It:

1. **Discovers** available games in the build directory
2. **Displays** games with icons and names
3. **Launches** games with server connection info
4. **Exits** - rest is handled by the game itself

**Location**: `/launcher/` directory
**Main Files**:
- `src/core/MainWindow.cpp/h` (Main window container)
- `src/scanner/GameScanner.cpp/h` (Game discovery)
- `src/ui/GameLibraryTab.cpp/h` (Game list display)
- `src/ui/VirtualControllerWindow.cpp/h` (Game window)
- `src/ui/ControllerTab.cpp/h` (Input configuration)

## Game Discovery (GameScanner)

### How It Works

GameScanner automatically finds all available games by scanning the build output directory.

```
build/bin/Release/
├── GameLibraryLauncher.exe      ← excluded
├── GameServer.exe               ← excluded
├── snake.exe                    ← found!
├── snake.ico                    ← matched and loaded
├── karting.exe                  ← found!
├── karting.ico                  ← matched and loaded
├── mygame.exe                   ← would be found
└── mygame.ico                   ← would be matched
```

### Scanning Process

When launcher starts, GameScanner::scanGames() runs:

```cpp
// GameScanner.cpp
std::vector<GameInfo> GameScanner::scanGames() {
    std::vector<GameInfo> games;
    
    // 1. Find build directory
    std::string buildBinPath = "../bin/Release";  // Relative to launcher
    
    // 2. List all exe files
    for (auto entry : fs::directory_iterator(buildBinPath)) {
        if (entry.path().extension() == ".exe") {
            std::string exeName = entry.path().filename().string();
            
            // 3. Skip launcher and server
            if (exeName == "GameLibraryLauncher.exe" ||
                exeName == "GameServer.exe") {
                continue;
            }
            
            // 4. Look for matching icon
            std::string iconPath = findIconForGame(exeName);
            
            // 5. Create GameInfo
            GameInfo info;
            info.name = exeName; // "snake"
            info.executablePath = entry.path().string();
            info.iconPath = iconPath;
            
            games.push_back(info);
        }
    }
    
    return games;
}
```

### GameInfo Structure

```cpp
struct GameInfo {
    QString name;                  // "snake" (from exe filename)
    QString folderPath;            // "./games/snake" (legacy, not used)
    QString executablePath;        // Full path to .exe
    QString iconPath;              // Full path to .ico (optional)
    QIcon icon;                    // Loaded icon object
};
```

Note: GameInfo stores only basic metadata. It does NOT contain runtime information like player count or game state - that data is outside the discovery mechanism.

### Icon Matching

For each .exe file, scanner looks for matching .ico:

- `snake.exe` → looks for `snake.ico`
- `karting.exe` → looks for `karting.ico`
- `mygame.exe` → looks for `mygame.ico`

Icons are:
- **Size**: 48×48 pixels (squared)
- **Format**: .ico (Windows icon format)
- **Location**: Same directory as executable

If icon not found, game displays with default icon.

### Example Output

```
Games found:
1. snake
   Executable: [...]\VirtualController\build\bin\Release\snake.exe
   Icon: [...]\VirtualController\build\bin\Release\snake.ico
```

## Game Library Tab

The GameLibraryTab displays all discovered games in a scrollable list layout.

### UI Layout

```
┌────────────────────────────────┐
│ Game Library         [Refresh] │
├────────────────────────────────┤
│  [Icon] snake                  │
│  [Icon] mygame                 │
│  [Icon] another_game           │
└────────────────────────────────┘
```

### Display Information Per Game

Each game shows:

| Item | Source |
|------|--------|
| Icon | `GameInfo.icon` (48×48 loaded from .ico) |
| Name | `GameInfo.name` (derived from exe filename) |

**Note:** The launcher list does NOT display runtime information. Player counts and game state are only visible within each running game window.

### Refresh Mechanism

The "Refresh Game Library" button rescans the build directory for new/removed games:

```cpp
void GameLibraryTab::onRefreshClicked() {
    loadGames();  // Rescan build directory
}

void GameLibraryTab::loadGames() {
    gamesList->clear();
    games = scanner->scanGames("./games");
    
    for (const GameInfo& game : games) {
        QListWidgetItem* item = new QListWidgetItem(game.icon, game.name);
        item->setData(Qt::UserRole, QVariant::fromValue(game));
        gamesList->addItem(item);
    }
}
```

**Does NOT:**
- Query game server
- Check player counts
- Poll game states
- Auto-refresh (only on manual button click or startup)

This is by design: the game library is static, runtime state is shown within each game window.

## Connection Management

### Launcher and Server Communication

**Important:** The game launcher (GameLibraryTab) does NOT connect to the game server. It only:
1. Scans for game executables
2. Launches them when clicked
3. Passes server host/port as command-line arguments

**Games receive the server info at launch:**

```cpp
// GameLibraryTab::launchGame()
QStringList args;
args << serverHost << QString::number(serverPort);  // Passed to game
QProcess::startDetached(game.executablePath, args, workingDir, &pid);
```

The actual game process (e.g., snake.exe) reads these arguments and connects to the server.

### Communication Ports

| Component | Port | Role |
|-----------|------|------|
| GameServer | 9000 (default) | Accepts game client connections |
| Launcher | N/A | Only launches games, no server connection |
| Games | 9000 (default) | Connect to GameServer |
| debug_proxy | 9001 | Forwards to 9000 (when running) |

## User Interaction Flow

### Launching a Game

```
User clicks game in list
         ↓
→ GameLibraryTab::onGameClicked()
    ↓
→ GameLibraryTab::launchGame()
    ↓
QProcess::startDetached() launch:
   - Executable: build/bin/Release/snake.exe
   - Args: [serverHost, serverPort] (passed to game)
   - Working dir: build/bin/Release/
    ↓
Game process starts independently
    ↓
Launcher UI remains showing game list
(Game connects to server internally)
```

**Note:** Launcher role ends after launching. The game process manages its own network connection and UI.

### After Launch

Once a game is launched:
- The game process runs independently
- The launcher continues showing the game list
- Each game window handles its own UI, networking, and rendering
- Multiple games can be launched simultaneously
- No communication between launcher and running games

## Multiple Simultaneous Games

You can launch multiple games at once:

```
GameLibraryLauncher.exe (Qt main process)
    ↓
    Creates detached child processes:
    • snake.exe (independent)
    • my_game.exe (independent)
    • another_game.exe (independent)
```

Each game:
- Runs in its own process
- Manages its own SFML window
- Connects independently to GameServer
- Does not communicate through launcher

### Implementation

Games are launched with `QProcess::startDetached()` which:
- Creates independent child process
- Does not wait for process to finish
- Launcher continues running, ready to launch more games
- Closing launcher does not close running games

## Controller Tab

Optional tab for input configuration.

### Current Status

The ControllerTab exists in the codebase but is minimally used. It displays:
- Connected input devices
- Configuration options (potentially for future use)

### Implementation

```cpp
class ControllerTab : public QWidget {
private:
    std::vector<InputDevice> devices;
    
public:
    void refreshDevices();
    void showDeviceStatus();
};
```

## Game Launch Process

### How Games Are Started

**GameLibraryTab::launchGame()** (Actual Code):

```cpp
void GameLibraryTab::launchGame(const GameInfo& game) {
    // 1. Validate game executable exists
    QFileInfo exeInfo(game.executablePath);
    if (!exeInfo.exists() || !exeInfo.isFile()) {
        QMessageBox::critical(this, "Launch Error",
            QString("Executable not found: %1")
                .arg(game.executablePath));
        return;
    }
    
    // 2. Prepare working directory and arguments
    QString workingDir = exeInfo.absolutePath();
    QStringList args;
    args << serverHost << QString::number(serverPort);
    
    // 3. Launch detached process (independent of launcher)
    qint64 pid;
    bool success = QProcess::startDetached(
        game.executablePath,
        args,
        workingDir,
        &pid
    );
    
    if (!success) {
        QMessageBox::critical(this, "Launch Error",
            QString("Failed to start '%1'").arg(game.name));
    }
}
```

### What the Game Receives

Each game is passed two command-line arguments:
- `&argv[1]` - server host (e.g., "127.0.0.1" or "192.168.1.5")
- `&argv[2]` - server port (e.g., "9000")

Games use these to connect to GameServer on startup.

### Path Resolution

Games are launched from launcher's directory:

```
GameLibraryLauncher.exe
    ↓ executablePath is relative: ../bin/Release/snake.exe
        ↓
Resolved to: [...]\VirtualController\build\bin\Release\snake.exe
```

Game can be in same directory (build/bin/Release) because:
- All DLLs copied there by CMake
- Icons located there
- Configuration files optional

## Launcher Architecture

The launcher is a **game discovery and launching tool**, not a networked component:

- **No server connection** - Launcher does not connect to GameServer
- **No status display** - Does not show game states or player counts
- **No persistent network** - Simply launches executables and terminates responsibility
- **Arguments only** - Passes server host/port to each game as startup arguments

## Directory Structure

### Launcher-Specific Files

```
launcher/
├── CMakeLists.txt
├── src/
│   ├── core/
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.h
│   │   └── main.cpp
│   ├── input/
│   │   ├── LocalInputSource.cpp
│   │   └── LocalInputSource.h
│   ├── interfaces/
│   │   └── IInputSource.h
│   ├── managers/
│   │   ├── MultiControllerManager.cpp
│   │   └── MultiControllerManager.h
│   ├── scanner/
│   │   ├── GameScanner.cpp
│   │   ├── GameScanner.h
│   │   └── GameInfo.h
│   └── ui/
│       ├── ControllerTab.cpp
│       ├── ControllerTab.h
│       ├── GameLibraryTab.cpp
│       ├── GameLibraryTab.h
│       ├── VirtualControllerWindow.cpp
│       └── VirtualControllerWindow.h
```

### Key Classes

| Class | File | Purpose |
|-------|------|---------|
| MainWindow | core/ | Top-level window container |
| GameLibraryTab | ui/ | Game list display |
| VirtualControllerWindow | ui/ | Single game window |
| ControllerTab | ui/ | Input configuration |
| GameScanner | scanner/ | Game discovery |
| GameInfo | scanner/ | Game metadata |

## Adding a New Game to Discovery

### Step 1: Create Game Executable

Compile your game (see [Adding New Games](07_ADDING_NEW_GAMES.md)):

```bash
cmake --build build --config Release --target my_game
# Output: build/bin/Release/my_game.exe
```

### Step 2: Create Game Icon

Create 48×48 ICO file named to match executable:

```
build/bin/Release/my_game.ico
```

Icon is copied automatically by CMake post-build command.

### Step 3: Restart Launcher

Close and reopen GameLibraryLauncher. Next scan will find it:

```
Games found:
1. snake
2. my_game (NEW!)
```

### No Registration Needed

Games appear automatically. No configuration files, registry entries, or manifest needed.

## Troubleshooting Launcher Issues

### Games Not Appearing

**Problem**: Game list is empty even though games exist in build/bin/Release/

**Causes**:
- Launcher's working directory is not the build directory
- Game executables (.exe) don't exist in `QCoreApplication::applicationDirPath()`
- Game names are "GameLibraryLauncher" or "GameServer" (these are filtered)

**Solution**:
1. Verify launcher runs from build/bin/Release/ directory:
   ```bash
   cd build/bin/Release
   .\GameLibraryLauncher.exe
   ```
2. Check games exist:
   ```bash
   dir *.exe
   ```
3. Verify names are correct (should see snake.exe, etc.)

### Game Won't Connect to Server

**Problem**: Game launches but shows "Connection failed"

**Debugging**:
1. Check GameServer is running:
   ```bash
   .\GameServer.exe
   ```
2. Check what arguments were passed to game (visible in launcher debug output)
3. Verify server host/port are reachable:
   ```bash
   ping <serverHost>
   # Then try connecting in game
   ```

### Icon Not Displaying

**Problem**: Game shows with generic/placeholder icon

**Causes**:
- Icon file name doesn't match exe (e.g., exe is `snake.exe` but icon is `game.ico`)
- Icon file not in launcher working directory
- Icon format not .ico or file is corrupted
- Icon is 0 bytes or invalid

**Solution**:
1. Verify icon name matches exe:
   ```bash
   # Should have both files
   snake.exe
   snake.ico  ← must match exactly
   ```
2. Check icon is valid:
   ```bash
   # Should be non-zero size
   dir snake.ico
   ```
3. Copy manually if CMake post-build failed:
   ```bash
   copy games\snake\snake.ico build\bin\Release\snake.ico
   ```

## Performance Considerations

**Scanning Time**

Scanning happens at launcher startup and when Refresh button clicked. With many .exe files in build directory, this may take a few seconds.

**Launcher Responsiveness**

Launcher UI is not blocked by launched games. Each game runs in separate process, so:
- Launcher remains responsive
- Can launch multiple games without blocking
- No server communication overhead in launcher

## Qt Integration

Launcher uses **Qt 6.10+** for UI framework.

### Main Qt Classes Used

| Class | Purpose |
|-------|---------|
| QMainWindow | Main application window |
| QWidget | Parent for custom widgets |
| QLabel | Text and icon display |
| QPushButton | Interactive buttons |
| QTimer | Refresh timing |
| QProcess | Game launching |

### Key Qt Components in GameLibraryTab

```cpp
// Game list widget (shows icons + names)
QListWidget* gamesList;
gamesList->setIconSize(QSize(48, 48));

// Refresh button (rescans build directory)
QPushButton* refreshButton;
connect(refreshButton, &QPushButton::clicked, this, 
        &GameLibraryTab::onRefreshClicked);

// Item clicked signal (launches game)
connect(gamesList, &QListWidget::itemClicked, this,
        &GameLibraryTab::onGameClicked);
```

## Next Steps

- See [Core Architecture](01_CORE_ARCHITECTURE.md) for system overview
- See [Game Protocol](03_GAME_PROTOCOL.md) for communication details
- See [Adding New Games](07_ADDING_NEW_GAMES.md) for game development
