import 'package:flutter/material.dart';

class ControllerButton extends StatefulWidget {
  final String label;
  final double size;
  final Color? color;
  final bool isWide;
  final Function(bool isPressed) onPressed;

  const ControllerButton({
    super.key,
    required this.label,
    required this.size,
    required this.onPressed,
    this.color,
    this.isWide = false,
  });

  @override
  State<ControllerButton> createState() => _ControllerButtonState();
}

class _ControllerButtonState extends State<ControllerButton> {
  bool _isPressed = false;

  void _handlePressStart() {
    setState(() => _isPressed = true);
    widget.onPressed(true);
  }

  void _handlePressEnd() {
    setState(() => _isPressed = false);
    widget.onPressed(false);
  }

  @override
  Widget build(BuildContext context) {
    final color = widget.color ?? Colors.deepPurple;
    
    return GestureDetector(
      onTapDown: (_) => _handlePressStart(),
      onTapUp: (_) => _handlePressEnd(),
      onTapCancel: () => _handlePressEnd(),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 50),
        width: widget.isWide ? double.infinity : widget.size,
        height: widget.size,
        decoration: BoxDecoration(
          color: _isPressed ? color.withOpacity(0.5) : color,
          borderRadius: BorderRadius.circular(widget.isWide ? 8 : widget.size / 2),
          boxShadow: _isPressed
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
        child: Center(
          child: Text(
            widget.label,
            style: TextStyle(
              color: Colors.white,
              fontSize: widget.isWide ? 16 : 18,
              fontWeight: FontWeight.bold,
            ),
          ),
        ),
      ),
    );
  }
}
