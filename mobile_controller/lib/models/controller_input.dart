/// Model for controller button states
class ControllerInput {
  final String button;
  final bool isPressed;
  final DateTime timestamp;

  ControllerInput({
    required this.button,
    required this.isPressed,
    required this.timestamp,
  });

  Map<String, dynamic> toJson() {
    return {
      'type': 'button',
      'button': button,
      'pressed': isPressed,
      'timestamp': timestamp.millisecondsSinceEpoch,
    };
  }
}

/// Model for analog stick values
class AnalogInput {
  final String stick; // 'left' or 'right'
  final double x; // -1.0 to 1.0
  final double y; // -1.0 to 1.0
  final DateTime timestamp;

  AnalogInput({
    required this.stick,
    required this.x,
    required this.y,
    required this.timestamp,
  });

  Map<String, dynamic> toJson() {
    return {
      'type': 'analog',
      'stick': stick,
      'x': x,
      'y': y,
      'timestamp': timestamp.millisecondsSinceEpoch,
    };
  }
}

/// Standardized button names matching Xbox controller layout
class ButtonNames {
  // Face buttons
  static const String a = 'A';
  static const String b = 'B';
  static const String x = 'X';
  static const String y = 'Y';
  
  // D-Pad
  static const String dpadUp = 'DPAD_UP';
  static const String dpadDown = 'DPAD_DOWN';
  static const String dpadLeft = 'DPAD_LEFT';
  static const String dpadRight = 'DPAD_RIGHT';
  
  // Shoulder buttons
  static const String lb = 'LB';
  static const String rb = 'RB';
  static const String lt = 'LT';
  static const String rt = 'RT';
  
  // System buttons
  static const String start = 'START';
  static const String select = 'SELECT';
  
  // Stick buttons
  static const String leftStick = 'LEFT_STICK';
  static const String rightStick = 'RIGHT_STICK';
}
