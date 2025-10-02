# Contributing to Fuego Flutter Wallet

Thank you for your interest in contributing to the Fuego Flutter Wallet! This document provides guidelines for contributing to this project.

## 🤝 How to Contribute

### Reporting Issues

Before creating an issue, please:
1. Check if the issue already exists
2. Use a clear and descriptive title
3. Provide detailed steps to reproduce the issue
4. Include relevant logs, screenshots, or error messages
5. Specify your environment (OS, Flutter version, device, etc.)

### Suggesting Features

We welcome feature suggestions! Please:
1. Check if the feature has already been requested
2. Clearly describe the feature and its benefits
3. Explain how it fits with Fuego's privacy-focused mission
4. Provide mockups or examples if applicable

### Submitting Code Changes

1. **Fork the repository**
   ```bash
   git clone https://github.com/usexfg/fuego-flutter.git
   cd fuego-flutter
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the coding standards below
   - Write tests for new functionality
   - Update documentation if needed

4. **Test your changes**
   ```bash
   flutter test
   flutter analyze
   dart format lib/ test/
   ```

5. **Commit your changes**
   ```bash
   git add .
   git commit -m "Add feature: your feature description"
   ```

6. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

7. **Create a Pull Request**
   - Use a clear and descriptive title
   - Reference any related issues
   - Describe what changes you made and why
   - Include screenshots for UI changes

## 📋 Coding Standards

### Dart/Flutter Guidelines

- Follow [Effective Dart](https://dart.dev/guides/language/effective-dart) guidelines
- Use `dart format` to format your code
- Run `flutter analyze` and fix all warnings
- Follow Flutter's [style guide](https://flutter.dev/docs/development/tools/formatting)

### Code Organization

```dart
// Order imports alphabetically within groups
import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../models/wallet.dart';
import '../services/fuego_rpc_service.dart';
import '../utils/theme.dart';
```

### Naming Conventions

- **Classes**: `PascalCase` (e.g., `WalletProvider`)
- **Functions/Variables**: `camelCase` (e.g., `sendTransaction`)
- **Constants**: `lowerCamelCase` (e.g., `defaultRpcPort`)
- **Files**: `snake_case` (e.g., `wallet_provider.dart`)

### Documentation

- Add dartdoc comments to public APIs
- Include code examples for complex functions
- Update README.md for new features
- Add inline comments for complex logic

```dart
/// Sends a transaction to the specified address with privacy controls.
/// 
/// [address] The recipient's Fuego address
/// [amount] The amount to send in XFG
/// [mixins] Number of mixins for privacy (0-15)
/// 
/// Returns the transaction hash if successful, null otherwise.
/// 
/// Example:
/// ```dart
/// final txHash = await sendTransaction(
///   address: 'fire7rp9y1XyaHBPNmBT...',
///   amount: 10.0,
///   mixins: 7,
/// );
/// ```
Future<String?> sendTransaction({
  required String address,
  required double amount,
  int mixins = 7,
}) async {
  // Implementation...
}
```

## 🧪 Testing

### Writing Tests

- Write unit tests for all business logic
- Write widget tests for UI components
- Write integration tests for user flows
- Aim for >80% code coverage

### Test Structure

```dart
group('WalletProvider', () {
  late WalletProvider walletProvider;
  
  setUp(() {
    walletProvider = WalletProvider();
  });
  
  tearDown(() {
    walletProvider.dispose();
  });
  
  test('should create wallet successfully', () async {
    // Arrange
    const pin = '123456';
    
    // Act
    final result = await walletProvider.createWallet(pin: pin);
    
    // Assert
    expect(result, isTrue);
    expect(walletProvider.hasWallet, isTrue);
  });
});
```

### Running Tests

```bash
# Run all tests
flutter test

# Run tests with coverage
flutter test --coverage

# Run specific test file
flutter test test/providers/wallet_provider_test.dart

# Run integration tests
flutter drive --target=test_driver/app.dart
```

## 🔒 Security Guidelines

### Security Considerations

- Never commit private keys, mnemonics, or sensitive data
- Use secure coding practices for cryptographic operations
- Validate all user inputs
- Handle errors gracefully without exposing sensitive information
- Follow OWASP mobile security guidelines

### Cryptographic Code

- Use established libraries (cryptography, crypto packages)
- Don't implement custom cryptographic algorithms
- Use proper key derivation functions (PBKDF2, Argon2)
- Ensure secure random number generation

### Code Review Checklist

- [ ] No hardcoded secrets or sensitive data
- [ ] Proper error handling
- [ ] Input validation and sanitization
- [ ] Memory management (dispose controllers, listeners)
- [ ] Performance considerations
- [ ] Security best practices followed

## 🎨 UI/UX Guidelines

### Design Principles

- **Privacy First**: Dark theme, minimal data collection
- **Security Transparency**: Clear security indicators
- **User-Friendly**: Intuitive navigation and feedback
- **Accessibility**: Support for screen readers and accessibility features

### UI Components

- Use consistent spacing (8px grid system)
- Follow Material Design principles
- Maintain consistent color scheme (see `theme.dart`)
- Provide loading states and error handling
- Include haptic feedback for important actions

### Animations

- Keep animations smooth (60fps)
- Use appropriate duration (200-500ms for most transitions)
- Provide meaningful motion that guides user attention
- Allow users to disable animations if needed

## 📱 Platform Considerations

### Android

- Support API level 24+ (Android 7.0+)
- Test on various screen sizes and densities
- Handle Android-specific permissions
- Support Android Auto-fill for forms

### iOS

- Support iOS 12.0+
- Test on various device sizes (iPhone/iPad)
- Handle iOS-specific permissions
- Support iOS accessibility features

### Desktop (Future)

- Responsive design for larger screens
- Keyboard navigation support
- Platform-specific menu integration

## 🚀 Release Process

### Version Management

- Follow [Semantic Versioning](https://semver.org/)
- Update version in `pubspec.yaml`
- Update `CHANGELOG.md` with release notes
- Tag releases appropriately

### Pre-release Checklist

- [ ] All tests passing
- [ ] Code analysis clean (`flutter analyze`)
- [ ] Performance testing completed
- [ ] Security review completed
- [ ] Documentation updated
- [ ] Translation strings updated

## 📞 Getting Help

- **Discord**: [Fuego Community](https://discord.gg/5UJcJJg)
- **GitHub Issues**: For bug reports and feature requests
- **Discussions**: For questions and community discussion

## 🙏 Recognition

Contributors will be:
- Listed in the CONTRIBUTORS.md file
- Mentioned in release notes for significant contributions
- Invited to the contributors' Discord channel

## 📄 License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

Thank you for helping make Fuego Flutter Wallet better for everyone! 🔥
