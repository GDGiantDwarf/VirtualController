# Test Controller Server

Quick Python script to test the mobile controller connection.

## Requirements

```bash
pip install websockets
```

## Run

```bash
python test_controller_server.py
```

The server will:
1. Display your local IP address
2. Listen on port 8765
3. Print all button presses/releases from your mobile app

## Usage

1. Run this script on your computer
2. Note the IP address shown
3. Open the mobile app and enter that IP + port 8765
4. Tap CONNECT
5. Press buttons on your phone - you'll see them printed here!

Example output:
```
🎮 Button: A               PRESSED
🎮 Button: A               RELEASED
🎮 Button: DPAD_UP         PRESSED
🎮 Button: DPAD_UP         RELEASED
```
