import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../services/controller_service.dart';
import '../widgets/controller_button.dart';
import '../widgets/dpad.dart';
import '../widgets/joystick.dart';
import '../models/controller_input.dart';

class ControllerScreen extends StatelessWidget {
  const ControllerScreen({super.key});

  void _onButtonPressed(BuildContext context, String button, bool isPressed) {
    context.read<ControllerService>().sendButtonInput(button, isPressed);
    
    // Haptic feedback using native Flutter API
    if (isPressed) {
      HapticFeedback.lightImpact();
    }
  }

  void _disconnect(BuildContext context) {
    context.read<ControllerService>().disconnect();
  }

  void _onAnalogChanged(BuildContext context, String stick, double x, double y) {
    context.read<ControllerService>().sendAnalogInput(stick, x, y);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.grey.shade900,
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(12.0),
          child: Column(
            children: [
              // Top bar with status and connection info
              _buildTopBar(context),
              const SizedBox(height: 12),
              
              // Shoulder buttons (all 6 in one row)
              _buildShoulderRow(context),
              const SizedBox(height: 16),
              
              // Main controller area: Left Stick | D-Pad | Right Stick | Action Buttons
              Expanded(
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    // Left Joystick
                    Expanded(
                      child: Center(
                        child: Joystick(
                          label: 'L-STICK',
                          size: 160,
                          onChanged: (x, y) => _onAnalogChanged(
                            context,
                            'left',
                            x,
                            y,
                          ),
                        ),
                      ),
                    ),
                    
                    // D-Pad
                    Expanded(
                      child: Center(
                        child: DPad(
                          onDirectionPressed: (direction, isPressed) {
                            _onButtonPressed(context, direction, isPressed);
                          },
                        ),
                      ),
                    ),
                    
                    // Right Joystick
                    Expanded(
                      child: Center(
                        child: Joystick(
                          label: 'R-STICK',
                          size: 160,
                          onChanged: (x, y) => _onAnalogChanged(
                            context,
                            'right',
                            x,
                            y,
                          ),
                        ),
                      ),
                    ),
                    
                    // Action Buttons (ABXY)
                    Expanded(
                      child: Center(
                        child: _buildActionButtons(context),
                      ),
                    ),
                  ],
                ),
              ),
              
              // Center buttons row (START/SELECT)
              const SizedBox(height: 16),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTopBar(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: Colors.deepPurple.withOpacity(0.3),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        children: [
          const Icon(Icons.gamepad, color: Colors.white70, size: 20),
          const SizedBox(width: 8),
          Expanded(
            child: Consumer<ControllerService>(
              builder: (context, service, child) {
                return Text(
                  'Connected: ${service.serverAddress ?? "Unknown"}',
                  style: const TextStyle(
                    color: Colors.white70,
                    fontSize: 12,
                  ),
                );
              },
            ),
          ),
          IconButton(
            icon: const Icon(Icons.close, color: Colors.white70),
            onPressed: () => _disconnect(context),
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(),
          ),
        ],
      ),
    );
  }

  Widget _buildActionButtons(BuildContext context) {
    const buttonSize = 70.0;
    const spacing = 20.0;

    return SizedBox(
      width: buttonSize * 2 + spacing,
      height: buttonSize * 2 + spacing,
      child: Stack(
        children: [
          // Y - Top
          Positioned(
            left: buttonSize / 2 + spacing / 2,
            top: 0,
            child: ControllerButton(
              label: 'Y',
              size: buttonSize,
              color: Colors.yellow.shade700,
              onPressed: (isPressed) => _onButtonPressed(
                context,
                ButtonNames.y,
                isPressed,
              ),
            ),
          ),
          // B - Right
          Positioned(
            right: 0,
            top: buttonSize / 2 + spacing / 2,
            child: ControllerButton(
              label: 'B',
              size: buttonSize,
              color: Colors.red.shade700,
              onPressed: (isPressed) => _onButtonPressed(
                context,
                ButtonNames.b,
                isPressed,
              ),
            ),
          ),
          // A - Bottom
          Positioned(
            left: buttonSize / 2 + spacing / 2,
            bottom: 0,
            child: ControllerButton(
              label: 'A',
              size: buttonSize,
              color: Colors.green.shade700,
              onPressed: (isPressed) => _onButtonPressed(
                context,
                ButtonNames.a,
                isPressed,
              ),
            ),
          ),
          // X - Left
          Positioned(
            left: 0,
            top: buttonSize / 2 + spacing / 2,
            child: ControllerButton(
              label: 'X',
              size: buttonSize,
              color: Colors.blue.shade700,
              onPressed: (isPressed) => _onButtonPressed(
                context,
                ButtonNames.x,
                isPressed,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildShoulderRow(BuildContext context) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Expanded(
          child: ControllerButton(
            label: 'LT',
            size: 40,
            isWide: true,
            color: Colors.grey.shade600,
            onPressed: (isPressed) => _onButtonPressed(
              context,
              ButtonNames.lt,
              isPressed,
            ),
          ),
        ),
        const SizedBox(width: 6),
        Expanded(
          child: ControllerButton(
            label: 'LB',
            size: 40,
            isWide: true,
            color: Colors.grey.shade700,
            onPressed: (isPressed) => _onButtonPressed(
              context,
              ButtonNames.lb,
              isPressed,
            ),
          ),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: ControllerButton(
            label: 'SELECT',
            size: 40,
            isWide: true,
            color: Colors.deepPurple.shade600,
            onPressed: (isPressed) => _onButtonPressed(
              context,
              ButtonNames.select,
              isPressed,
            ),
          ),
        ),
        const SizedBox(width: 6),
        Expanded(
          child: ControllerButton(
            label: 'START',
            size: 40,
            isWide: true,
            color: Colors.deepPurple.shade600,
            onPressed: (isPressed) => _onButtonPressed(
              context,
              ButtonNames.start,
              isPressed,
            ),
          ),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: ControllerButton(
            label: 'RB',
            size: 40,
            isWide: true,
            color: Colors.grey.shade700,
            onPressed: (isPressed) => _onButtonPressed(
              context,
              ButtonNames.rb,
              isPressed,
            ),
          ),
        ),
        const SizedBox(width: 6),
        Expanded(
          child: ControllerButton(
            label: 'RT',
            size: 40,
            isWide: true,
            color: Colors.grey.shade600,
            onPressed: (isPressed) => _onButtonPressed(
              context,
              ButtonNames.rt,
              isPressed,
            ),
          ),
        ),
      ],
    );
  }
}
