import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/controller_service.dart';
import 'qr_scan_screen.dart';
import 'dart:convert';

class ConnectionScreen extends StatefulWidget {
  const ConnectionScreen({super.key});

  @override
  State<ConnectionScreen> createState() => _ConnectionScreenState();
}

class _ConnectionScreenState extends State<ConnectionScreen> {
  final _hostController = TextEditingController(text: '192.168.1.100');
  final _portController = TextEditingController(text: '8765');
  bool _isConnecting = false;

  @override
  void dispose() {
    _hostController.dispose();
    _portController.dispose();
    super.dispose();
  }

  Future<void> _connect() async {
    if (_isConnecting) return;

    final host = _hostController.text.trim();
    final port = int.tryParse(_portController.text.trim());

    if (host.isEmpty || port == null) {
      _showError('Please enter valid host and port');
      return;
    }

    setState(() => _isConnecting = true);

    final service = context.read<ControllerService>();
    final success = await service.connect(host, port);

    if (mounted) {
      setState(() => _isConnecting = false);
      
      if (!success) {
        _showError('Failed to connect. Check host/port and try again.');
      }
    }
  }

  void _showError(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: Colors.red,
      ),
    );
  }

  Map<String, dynamic>? _parsePairingPayload(String raw) {
    try {
      // Try JSON first: {"host":"ip","port":8765}
      final data = json.decode(raw);
      if (data is Map && data['host'] is String) {
        final host = (data['host'] as String).trim();
        final port = (data['port'] is int)
            ? data['port'] as int
            : int.tryParse('${data['port']}');
        if (host.isNotEmpty && port != null) {
          return {'host': host, 'port': port};
        }
      }
    } catch (_) {
      // Not JSON, fall through
    }

    try {
      // Accept ws://host:port[/path]
      if (raw.startsWith('ws://') || raw.startsWith('wss://')) {
        final uri = Uri.parse(raw);
        if (uri.host.isNotEmpty) {
          final port = uri.port == 0 ? 8765 : uri.port;
          return {'host': uri.host, 'port': port};
        }
      }
    } catch (_) {}

    final s = raw.trim();
    // host:port or ip:port
    if (s.contains(':')) {
      final parts = s.split(':');
      if (parts.length >= 2) {
        final host = parts[0].trim();
        final port = int.tryParse(parts[1].trim());
        if (host.isNotEmpty && port != null) {
          return {'host': host, 'port': port};
        }
      }
    }

    // plain host/IP => default port 8765
    if (s.isNotEmpty) {
      return {'host': s, 'port': 8765};
    }

    return null;
  }

  Future<void> _scanAndConnect() async {
    if (_isConnecting) return;

    final result = await Navigator.of(context).push<String>(
      MaterialPageRoute(builder: (_) => const QrScanScreen()),
    );

    if (result == null) return;

    final parsed = _parsePairingPayload(result);
    if (parsed == null) {
      _showError('Invalid QR content. Expected ws://host:port or JSON.');
      return;
    }

    _hostController.text = parsed['host'] as String;
    _portController.text = (parsed['port'] as int).toString();

    await _connect();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [
              Colors.deepPurple.shade900,
              Colors.deepPurple.shade700,
            ],
          ),
        ),
        child: SafeArea(
          child: Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(24.0),
              child: Card(
                elevation: 8,
                child: Padding(
                  padding: const EdgeInsets.all(24.0),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      const Icon(
                        Icons.gamepad,
                        size: 64,
                        color: Colors.deepPurple,
                      ),
                      const SizedBox(height: 16),
                      const Text(
                        'Virtual Controller',
                        style: TextStyle(
                          fontSize: 28,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        'Connect to your desktop app',
                        style: TextStyle(
                          fontSize: 14,
                          color: Colors.grey.shade600,
                        ),
                      ),
                      const SizedBox(height: 32),
                      TextField(
                        controller: _hostController,
                        decoration: const InputDecoration(
                          labelText: 'Desktop IP Address',
                          hintText: '192.168.1.100',
                          prefixIcon: Icon(Icons.computer),
                          border: OutlineInputBorder(),
                        ),
                        keyboardType: TextInputType.number,
                      ),
                      const SizedBox(height: 16),
                      TextField(
                        controller: _portController,
                        decoration: const InputDecoration(
                          labelText: 'Port',
                          hintText: '8765',
                          prefixIcon: Icon(Icons.settings_ethernet),
                          border: OutlineInputBorder(),
                        ),
                        keyboardType: TextInputType.number,
                      ),
                      const SizedBox(height: 24),
                      Row(
                        children: [
                          Expanded(
                            child: SizedBox(
                              height: 50,
                              child: ElevatedButton(
                                onPressed: _isConnecting ? null : _connect,
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: Colors.deepPurple,
                                  foregroundColor: Colors.white,
                                  shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                ),
                                child: _isConnecting
                                    ? const SizedBox(
                                        width: 24,
                                        height: 24,
                                        child: CircularProgressIndicator(
                                          strokeWidth: 2,
                                          color: Colors.white,
                                        ),
                                      )
                                    : const Text(
                                        'CONNECT',
                                        style: TextStyle(
                                          fontSize: 16,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                              ),
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: SizedBox(
                              height: 50,
                              child: OutlinedButton.icon(
                                onPressed: _isConnecting ? null : _scanAndConnect,
                                style: OutlinedButton.styleFrom(
                                  side: const BorderSide(color: Colors.deepPurple),
                                  shape: RoundedRectangleBorder(
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                ),
                                icon: const Icon(Icons.qr_code, color: Colors.deepPurple),
                                label: const Text(
                                  'SCAN QR',
                                  style: TextStyle(
                                    fontSize: 16,
                                    fontWeight: FontWeight.bold,
                                    color: Colors.deepPurple,
                                  ),
                                ),
                              ),
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                      Consumer<ControllerService>(
                        builder: (context, service, child) {
                          return Text(
                            service.statusMessage,
                            style: TextStyle(
                              fontSize: 12,
                              color: service.isConnected 
                                  ? Colors.green 
                                  : Colors.grey.shade600,
                            ),
                          );
                        },
                      ),
                      const SizedBox(height: 24),
                      Divider(color: Colors.grey.shade300),
                      const SizedBox(height: 16),
                      Text(
                        'How to connect:',
                        style: TextStyle(
                          fontSize: 14,
                          fontWeight: FontWeight.bold,
                          color: Colors.grey.shade700,
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        'Option A: Enter IP/Port and tap Connect.\n'
                        'Option B: Tap Scan QR and point your camera at the code.',
                        textAlign: TextAlign.center,
                        style: TextStyle(
                          fontSize: 12,
                          color: Colors.grey.shade600,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
