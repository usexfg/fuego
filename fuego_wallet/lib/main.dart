import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:flutter/services.dart';
import 'providers/wallet_provider.dart';
import 'screens/splash_screen.dart';
import 'utils/theme.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  
  // Set preferred orientations
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);
  
  // Set system UI overlay style
  SystemChrome.setSystemUIOverlayStyle(
    const SystemUiOverlayStyle(
      statusBarColor: Colors.transparent,
      statusBarIconBrightness: Brightness.light,
      systemNavigationBarColor: Colors.black,
      systemNavigationBarIconBrightness: Brightness.light,
    ),
  );

  runApp(const FuegoWalletApp());
}

class FuegoWalletApp extends StatelessWidget {
  const FuegoWalletApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => WalletProvider()),
      ],
      child: MaterialApp(
        title: 'Fuego Wallet',
        theme: AppTheme.lightTheme,
        darkTheme: AppTheme.darkTheme,
        themeMode: ThemeMode.dark, // Default to dark theme for crypto aesthetic
        home: const SplashScreen(),
        debugShowCheckedModeBanner: false,
      ),
    );
  }
}