# Test Controller Server

Quick Python script to test the mobile controller connection.

## Requirements

```bash
pip install websockets qrcode[pil]
```

## Run

```bash
python test_controller_server.py
```

The server will:
1. Display your local IP address
2. Generate and display an ASCII QR code for easy pairing
3. Listen on port 8765
4. Print all button presses/releases and joystick input from your mobile app

## Usage

### QR Code Pairing (Recommended)
1. Run this script on your computer
2. An ASCII QR code will be displayed in the terminal
3. Open the mobile app and tap "SCAN QR"
4. Point your phone camera at the QR code
5. The app automatically connects
6. Press buttons on your phone - you'll see them printed here!

### Manual Connection
1. Run this script on your computer
2. Note the IP address shown (e.g., `192.168.1.144`)
3. Open the mobile app and tap the input fields
4. Enter the IP address and port (8765)
5. Tap "CONNECT"
6. Press buttons on your phone - you'll see them printed here!

Example output:
```
============================================================
🎮 Virtual Controller Test Server
============================================================

📡 Server starting on 192.168.1.144:8765

🔗 Pairing URL: ws://192.168.1.144:8765/controller

🟦 Scan this QR with the mobile app:

█▀▀▀▀▀▀▀███▀▀▀▀▀▀███▀██▀▀▀▀▀▀▀█
█ █▀▀▀█ █  █ ██▀▄▀ ▄ ▄█ █▀▀▀█ █
█ █   █ █▄▀█▄ ▀  █▀▄▄▀█ █   █ █
(QR code continues...)

⏳ Waiting for controller connection...

✅ Controller connected from 192.168.1.100:54321
🎮 Button: A               PRESSED
🎮 Button: A               RELEASED
🕹️  Analog left : X=+0.34, Y=-0.67
🕹️  Analog right: X=+0.12, Y=-0.89
```
