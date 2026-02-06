import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'services/controller_service.dart';
import 'screens/connection_screen.dart';
import 'screens/controller_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  
  // Lock to landscape orientation for better controller layout
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft,
    DeviceOrientation.landscapeRight,
  ]);
  
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => ControllerService(),
      child: MaterialApp(
        title: 'Virtual Controller',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: Colors.deepPurple,
            brightness: Brightness.dark,
          ),
          useMaterial3: true,
        ),
        home: const MainScreen(),
      ),
    );
  }
}

class MainScreen extends StatelessWidget {
  const MainScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<ControllerService>(
      builder: (context, service, child) {
        if (service.isConnected) {
          return const ControllerScreen();
        } else {
          return const ConnectionScreen();
        }
      },
    );
  }
}
