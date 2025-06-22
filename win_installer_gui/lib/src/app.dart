import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'screens/home_screen.dart';
import 'providers/mouse_provider.dart';

class WinInstallerApp extends StatelessWidget {
  const WinInstallerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => MouseProvider()),
      ],
      child: MaterialApp(
        title: 'Windows 系统重装工具',
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: Colors.blue,
            brightness: Brightness.light,
          ),
          useMaterial3: true,
        ),
        builder: (context, child) {
          return MouseRegion(
            onHover: (_) => context.read<MouseProvider>().notifyMouseMove(),
            child: child!,
          );
        },
        home: const HomeScreen(),
        debugShowCheckedModeBanner: false,
      ),
    );
  }
} 