import 'package:flutter/material.dart';
import '../models/controller_input.dart';

class DPad extends StatefulWidget {
  final Function(String direction, bool isPressed) onDirectionPressed;

  const DPad({
    super.key,
    required this.onDirectionPressed,
  });

  @override
  State<DPad> createState() => _DPadState();
}

class _DPadState extends State<DPad> {
  String? _pressedDirection;

  void _handlePressStart(String direction) {
    setState(() => _pressedDirection = direction);
    widget.onDirectionPressed(direction, true);
  }

  void _handlePressEnd() {
    if (_pressedDirection != null) {
      widget.onDirectionPressed(_pressedDirection!, false);
      setState(() => _pressedDirection = null);
    }
  }

  bool _isPressed(String direction) => _pressedDirection == direction;

  @override
  Widget build(BuildContext context) {
    const size = 60.0;
    const spacing = 4.0;

    return SizedBox(
      width: size * 3 + spacing * 2,
      height: size * 3 + spacing * 2,
      child: Stack(
        children: [
          // Center piece
          Positioned(
            left: size + spacing,
            top: size + spacing,
            child: Container(
              width: size,
              height: size,
              decoration: BoxDecoration(
                color: Colors.grey.shade800,
                borderRadius: BorderRadius.circular(8),
              ),
            ),
          ),
          
          // Up
          Positioned(
            left: size + spacing,
            top: 0,
            child: _buildDPadButton(
              direction: ButtonNames.dpadUp,
              icon: Icons.arrow_upward,
            ),
          ),
          
          // Down
          Positioned(
            left: size + spacing,
            bottom: 0,
            child: _buildDPadButton(
              direction: ButtonNames.dpadDown,
              icon: Icons.arrow_downward,
            ),
          ),
          
          // Left
          Positioned(
            left: 0,
            top: size + spacing,
            child: _buildDPadButton(
              direction: ButtonNames.dpadLeft,
              icon: Icons.arrow_back,
            ),
          ),
          
          // Right
          Positioned(
            right: 0,
            top: size + spacing,
            child: _buildDPadButton(
              direction: ButtonNames.dpadRight,
              icon: Icons.arrow_forward,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDPadButton({
    required String direction,
    required IconData icon,
  }) {
    const size = 60.0;
    final isPressed = _isPressed(direction);

    return GestureDetector(
      onTapDown: (_) => _handlePressStart(direction),
      onTapUp: (_) => _handlePressEnd(),
      onTapCancel: () => _handlePressEnd(),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 50),
        width: size,
        height: size,
        decoration: BoxDecoration(
          color: isPressed 
              ? Colors.grey.shade700 
              : Colors.grey.shade800,
          borderRadius: BorderRadius.circular(8),
          boxShadow: isPressed
              ? []
              : [
                  BoxShadow(
                    color: Colors.black.withOpacity(0.3),
                    blurRadius: 8,
                    offset: const Offset(0, 4),
                  ),
                ],
          border: Border.all(
            color: Colors.white.withOpacity(0.2),
            width: 2,
          ),
        ),
        child: Icon(
          icon,
          color: Colors.white70,
          size: 28,
        ),
      ),
    );
  }
}
