# Mobile Controller

Flutter-based mobile application that acts as a virtual game controller for the VirtualController system.

## Features

- **WebSocket Connection**: Lightweight real-time communication with desktop app
- **Standard Controller Layout**: Xbox-style button layout with dual joysticks
  - Face buttons: A, B, X, Y
  - Shoulder buttons: LB, RB, LT, RT (4 triggers)
  - Dual analog joysticks with analog stick buttons
  - D-Pad and Start/Select
- **QR Code Pairing**: Scan QR from desktop app for automatic IP/port configuration
- **Manual Pairing**: Or manually enter IP address and port
- **Haptic Feedback**: Vibration feedback for button presses
- **Landscape Mode**: Optimized for controller usage

## Requirements

- Flutter SDK 3.0 or higher
- Android 5.0+ or iOS 12.0+

## Installation

1. Install Flutter dependencies:
```bash
cd mobile_controller
flutter pub get
```

2. Run on device/emulator:
```bash
# Android
flutter run

# iOS
flutter run

# Specific device
flutter run -d <device-id>
```

## Building Release

### Android APK
```bash
flutter build apk --release
# Output: build/app/outputs/flutter-apk/app-release.apk
```

### iOS
```bash
flutter build ios --release
# Then archive in Xcode
```

## Usage

### Quick Start (QR Code)
1. Run desktop app with test server
2. Open mobile controller app
3. Tap "SCAN QR" and point camera at the QR code shown on desktop
4. App auto-fills IP/port and connects automatically
5. Use controller buttons to play!

### Manual Connection
1. Ensure desktop app is running and showing its IP address
2. Open mobile controller app
3. Enter desktop IP address and port (default: 8765)
4. Tap "CONNECT"
5. Use controller buttons to play!

## Architecture

### Project Structure
```
lib/
├── main.dart                   # App entry point
├── models/
│   └── controller_input.dart  # Input data models
├── screens/
│   ├── connection_screen.dart # Connection UI
│   └── controller_screen.dart # Controller UI
├── services/
│   └── controller_service.dart # WebSocket service
└── widgets/
    ├── controller_button.dart  # Button widget
    └── dpad.dart              # D-Pad widget
```

### Communication Protocol

Messages are sent as JSON over WebSocket:

**Button Event:**
```json
{
  "type": "button",
  "button": "A",
  "pressed": true,
  "timestamp": 1234567890
}
```

**Analog Input (future):**
```json
{
  "type": "analog",
  "stick": "left",
  "x": 0.5,
  "y": -0.3,
  "timestamp": 1234567890
}
```

### Button Names
Following Xbox controller standard:
- Face buttons: `A`, `B`, `X`, `Y`
- D-Pad: `DPAD_UP`, `DPAD_DOWN`, `DPAD_LEFT`, `DPAD_RIGHT`
- Shoulder: `LB`, `RB`
- System: `START`, `SELECT`

## Dependencies

Lightweight packages chosen for minimal app size:

- **web_socket_channel**: WebSocket communication
- **provider**: State management (lightweight)
- **mobile_scanner**: QR code scanning for pairing

## Configuration

Default WebSocket endpoint: `ws://<host>:8765/controller`

To change the default host/port, edit [connection_screen.dart](lib/screens/connection_screen.dart):
```dart
final _hostController = TextEditingController(text: 'YOUR_IP');
final _portController = TextEditingController(text: 'YOUR_PORT');
```

## Next Steps

- Custom button mapping
- Connection history
- Rumble/force feedback from desktop
- Battery indicator passthrough
