# Game Configuration Guide

## Overview

The launcher supports per-game server configuration through a `games_config.json` file. This allows each game to connect to different servers or ports, rather than having all games use the same server address.

## How It Works

### With CLI Arguments (Override Mode)
When the launcher is started with explicit IP/port arguments:
```bash
.\GameLibraryLauncher.exe 192.168.1.100 8765
```
**All games** will connect to that server, regardless of the configuration file.

### Without CLI Arguments (Config Mode)  
When the launcher is started **without arguments**:
```bash
.\GameLibraryLauncher.exe
```
The launcher will:
1. Look for `games_config.json` in the same directory as `GameLibraryLauncher.exe`
2. Match each game's name with entries in the config file
3. Use the specified server address for each game
4. Fall back to `127.0.0.1:8765` if a game isn't found in the config

## Configuration File Format

The `games_config.json` file uses JSON format with this structure:

```json
{
  "games": {
    "snake": {
      "host": "127.0.0.1",
      "port": 8765
    },
    "karting": {
      "host": "127.0.0.1",
      "port": 8766
    }
  }
}
```

### Key Points
- **games key**: Container for all game configurations
- **game name**: Must match the executable name (without .exe)
  - `snake.exe` → key is `"snake"`
  - `karting.exe` → key is `"karting"`
- **host**: Server address (IP or hostname)
- **port**: Server port number

## Examples

### Example 1: All Games on Same Local Server
```json
{
  "games": {
    "snake": {
      "host": "127.0.0.1",
      "port": 8765
    },
    "karting": {
      "host": "127.0.0.1",
      "port": 8765
    }
  }
}
```

### Example 2: Different Servers for Each Game
```json
{
  "games": {
    "snake": {
      "host": "127.0.0.1",
      "port": 8765
    },
    "karting": {
      "host": "127.0.0.1",
      "port": 8766
    },
    "pacman": {
      "host": "192.168.1.50",
      "port": 9000
    }
  }
}
```

### Example 3: Network Servers
```json
{
  "games": {
    "snake": {
      "host": "game-server.local",
      "port": 8765
    },
    "karting": {
      "host": "racing-server.local",
      "port": 8766
    }
  }
}
```

## Usage Scenarios

### Scenario 1: Local Development
One game server runs on port 8765, another on 8766:

**Terminal 1:**
```bash
.\SnakeGameServer.exe 8765
```

**Terminal 2:**
```bash
.\KartingGameServer.exe 8766
```

**Terminal 3:**
```bash
.\GameLibraryLauncher.exe
# Uses games_config.json for routing
# Snake → port 8765
# Karting → port 8766
```

### Scenario 2: Override Local Config
Force all games to a specific server:

```bash
.\GameLibraryLauncher.exe 127.0.0.1 8765
# Both games connect to port 8765, ignoring games_config.json
```

### Scenario 3: Network Play
Multiple game servers on different machines:

```json
{
  "games": {
    "snake": {
      "host": "192.168.1.100",
      "port": 8765
    },
    "karting": {
      "host": "192.168.1.101",
      "port": 8766
    }
  }
}
```

Then run on any client machine:
```bash
.\GameLibraryLauncher.exe
```

## File Location

The `games_config.json` file must be placed in the same directory as `GameLibraryLauncher.exe`:

```
build/bin/Release/
├── GameLibraryLauncher.exe
├── games_config.json          ← Place here
├── snake.exe
├── karting.exe
└── [other files...]
```

The file is automatically copied there during the build process.

## Behavior & Defaults

| Situation | Behavior |
|-----------|----------|
| CLI args provided (`GameLibraryLauncher.exe 127.0.0.1 8765`) | Use CLI args for all games, ignore config file |
| No CLI args, config file exists with game entry | Use config for that game |
| No CLI args, config file exists but game not listed | Use defaults (127.0.0.1:8765) |
| No CLI args, config file missing | Use defaults (127.0.0.1:8765) |
| Invalid JSON in config file | Use defaults, warning logged to console |

## Console Output

When launching a game, the console will show which configuration is being used:

```
Launching game: snake at C:\...\snake.exe
Using game-specific config: 127.0.0.1 8765
Starting process with args: ( "127.0.0.1" "8765" )
startDetached returned: true PID: 1234
Game snake started with PID 1234
```

Or with CLI override:
```
Using CLI args: 127.0.0.1 8765
```

Or with defaults:
```
Using defaults: 127.0.0.1 8765
```

##  Adding New Games

When you add a new game `mygame.exe` to the launcher, follow this pattern:

1. **Create the executable** → `build/bin/Release/mygame.exe`
2. **Create the icon** → `build/bin/Release/mygame.ico`
3. **Add to config file:**
   ```json
   {
     "games": {
       "mygame": {
         "host": "127.0.0.1",
         "port": 9000
       }
     }
   }
   ```

The launcher will automatically discover it and use the configured server address.

## Troubleshooting

### Q: Config file isn't being used
**A:** Make sure:
1. File is named exactly `games_config.json` (case-sensitive)
2. File is in `build/bin/Release/` (same directory as launcher)
3. You started launcher with **no CLI arguments**
4. Game name in config matches executable name (without `.exe`)

Check the console output for "Using game-specific config" message.

### Q: Game connects to wrong server
**A:** 
1. Check console output shows which config is being used
2. If using CLI override: remove the arguments to use config file
3. Verify JSON syntax is valid at [jsonlint.com](https://www.jsonlint.com/)
4. Check game name matches exactly (case-sensitive)

### Q: JSON parse error
**A:** 
- Ensure no trailing commas in the JSON
- Verify all quotes are straight quotes (`"`) not curly quotes (`"`)
- Valid JSON should pass validation at [jsonlint.com](https://www.jsonlint.com/)

### Q: Defaults always used
**A:** The launcher will use defaults if:
1. Config file doesn't exist
2. Config file isn't in the right directory
3. Game name not found in config
4. CLI arguments were provided

Check console output to see which configuration source is being used.
