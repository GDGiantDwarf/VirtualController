import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:web_socket_channel/web_socket_channel.dart';
import '../models/controller_input.dart';

class ControllerService extends ChangeNotifier {
  WebSocketChannel? _channel;
  String? _serverAddress;
  bool _isConnected = false;
  String _statusMessage = 'Disconnected';

  bool get isConnected => _isConnected;
  String get statusMessage => _statusMessage;
  String? get serverAddress => _serverAddress;

  /// Connect to desktop app via WebSocket
  Future<bool> connect(String host, int port) async {
    try {
      _statusMessage = 'Connecting...';
      notifyListeners();

      final uri = Uri.parse('ws://$host:$port/controller');
      _channel = WebSocketChannel.connect(uri);
      
      // Wait for connection with timeout
      await _channel!.ready.timeout(
        const Duration(seconds: 5),
        onTimeout: () => throw TimeoutException('Connection timeout'),
      );

      _serverAddress = '$host:$port';
      _isConnected = true;
      _statusMessage = 'Connected to $host:$port';
      
      // Listen for messages from server
      _channel!.stream.listen(
        _handleServerMessage,
        onError: _handleError,
        onDone: _handleDisconnect,
      );

      notifyListeners();
      return true;
    } catch (e) {
      _statusMessage = 'Connection failed: $e';
      _isConnected = false;
      notifyListeners();
      return false;
    }
  }

  /// Disconnect from server
  void disconnect() {
    _channel?.sink.close();
    _channel = null;
    _isConnected = false;
    _serverAddress = null;
    _statusMessage = 'Disconnected';
    notifyListeners();
  }

  /// Send button press/release event
  void sendButtonInput(String button, bool isPressed) {
    if (!_isConnected) return;

    final input = ControllerInput(
      button: button,
      isPressed: isPressed,
      timestamp: DateTime.now(),
    );

    _sendMessage(input.toJson());
  }

  /// Send analog stick position
  void sendAnalogInput(String stick, double x, double y) {
    if (!_isConnected) return;

    final input = AnalogInput(
      stick: stick,
      x: x,
      y: y,
      timestamp: DateTime.now(),
    );

    _sendMessage(input.toJson());
  }

  void _sendMessage(Map<String, dynamic> data) {
    try {
      final json = jsonEncode(data);
      _channel?.sink.add(json);
    } catch (e) {
      if (kDebugMode) {
        print('Error sending message: $e');
      }
    }
  }

  void _handleServerMessage(dynamic message) {
    // Handle any messages from server (e.g., vibration commands)
    if (kDebugMode) {
      print('Received from server: $message');
    }
  }

  void _handleError(error) {
    _statusMessage = 'Error: $error';
    _isConnected = false;
    notifyListeners();
  }

  void _handleDisconnect() {
    _statusMessage = 'Disconnected from server';
    _isConnected = false;
    _channel = null;
    notifyListeners();
  }

  @override
  void dispose() {
    disconnect();
    super.dispose();
  }
}
