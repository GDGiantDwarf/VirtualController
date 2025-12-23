import 'dart:math';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Simple on-screen joystick for analog input
class Joystick extends StatefulWidget {
  final double size;
  final String label;
  final void Function(double x, double y) onChanged;
  final VoidCallback? onRelease;

  const Joystick({
    super.key,
    required this.size,
    required this.label,
    required this.onChanged,
    this.onRelease,
  });

  @override
  State<Joystick> createState() => _JoystickState();
}

class _JoystickState extends State<Joystick> {
  late Offset _center;
  Offset _knobOffset = Offset.zero;

  double get _radius => widget.size / 2;
  double get _knobRadius => widget.size * 0.18;

  void _handlePanStart(DragStartDetails details) {
    HapticFeedback.selectionClick();
    _updateOffset(details.localPosition);
  }

  void _handlePanUpdate(DragUpdateDetails details) {
    _updateOffset(details.localPosition);
  }

  void _handlePanEnd([_]) {
    setState(() {
      _knobOffset = Offset.zero;
    });
    widget.onChanged(0, 0);
    widget.onRelease?.call();
  }

  void _updateOffset(Offset localPos) {
    final relative = localPos - _center;
    final distance = relative.distance;
    final clampedDistance = min(distance, _radius - _knobRadius);

    // Avoid division by zero
    final direction = distance == 0 ? Offset.zero : relative / distance;
    final clamped = direction * clampedDistance;

    final x = (clamped.dx / (_radius - _knobRadius)).clamp(-1.0, 1.0);
    final y = (-clamped.dy / (_radius - _knobRadius)).clamp(-1.0, 1.0); // invert Y so up is positive

    setState(() {
      _knobOffset = clamped;
    });

    widget.onChanged(x, y);
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: widget.size,
      height: widget.size,
      child: LayoutBuilder(
        builder: (context, constraints) {
          _center = Offset(constraints.maxWidth / 2, constraints.maxHeight / 2);

          return GestureDetector(
            onPanStart: _handlePanStart,
            onPanUpdate: _handlePanUpdate,
            onPanEnd: _handlePanEnd,
            onPanCancel: _handlePanEnd,
            child: Stack(
              children: [
                // Base circle
                Container(
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: const RadialGradient(
                      colors: [Color(0xFF2C2C2C), Color(0xFF1A1A1A)],
                    ),
                    border: Border.all(color: Colors.white24, width: 2),
                    boxShadow: [
                      BoxShadow(
                        color: Colors.black.withOpacity(0.5),
                        blurRadius: 12,
                        offset: const Offset(0, 6),
                      ),
                    ],
                  ),
                ),
                // Label
                Positioned(
                  bottom: 8,
                  left: 0,
                  right: 0,
                  child: Text(
                    widget.label,
                    textAlign: TextAlign.center,
                    style: const TextStyle(
                      color: Colors.white70,
                      fontSize: 12,
                      fontWeight: FontWeight.w600,
                    ),
                  ),
                ),
                // Knob
                Positioned(
                  left: _center.dx - _knobRadius + _knobOffset.dx,
                  top: _center.dy - _knobRadius + _knobOffset.dy,
                  child: Container(
                    width: _knobRadius * 2,
                    height: _knobRadius * 2,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: Colors.deepPurpleAccent.shade200,
                      boxShadow: [
                        BoxShadow(
                          color: Colors.deepPurpleAccent.withOpacity(0.5),
                          blurRadius: 8,
                          offset: const Offset(0, 3),
                        ),
                      ],
                      border: Border.all(color: Colors.white30, width: 2),
                    ),
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}
