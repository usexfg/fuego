// Basic Flutter widget test for Fuego Wallet

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:fuego_wallet/main.dart';

void main() {
  testWidgets('Fuego Wallet app smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const FuegoWalletApp());

    // Verify that our app starts with the splash screen
    expect(find.text('FUEGO WALLET'), findsOneWidget);
    expect(find.text('Privacy-Focused Cryptocurrency'), findsOneWidget);
  });
}
