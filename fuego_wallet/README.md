# 🔥 Fuego Flutter Wallet

<p align="center">
  <img src="https://img.shields.io/badge/Flutter-3.22.2-blue?logo=flutter" alt="Flutter Version" />
  <img src="https://img.shields.io/badge/Dart-3.4.3-blue?logo=dart" alt="Dart Version" />
  <img src="https://img.shields.io/badge/Platform-Android%20%7C%20iOS-lightgrey" alt="Platform Support" />
  <img src="https://img.shields.io/badge/License-MIT-green" alt="License" />
</p>

A comprehensive, secure, and feature-rich mobile wallet application for **Fuego (XFG)** - the privacy-focused cryptocurrency built on the CryptoNote protocol.

## ✨ Features

### 🔐 Advanced Security
- **PIN Authentication** with PBKDF2 secure hashing
- **Biometric Authentication** (fingerprint/Face ID)
- **AES-256-CBC Encryption** for wallet data
- **Secure Device Storage** with hardware keychain
- **25-word Mnemonic** backup and recovery

### 💼 Wallet Management
- **Create New Wallets** with secure entropy
- **Restore from Backup** using mnemonic phrases
- **Real-time Balance Display** with privacy toggle
- **Blockchain Synchronization** progress tracking
- **Transaction History** with status monitoring

### 💸 Privacy Transactions
- **Send XFG** with configurable privacy levels
- **Ring Signatures** (0-15 mixins for anonymity)
- **Receive with QR Codes** and integrated addresses
- **Payment ID Support** for transaction identification
- **Fee Estimation** and validation

### ⛏️ Built-in Mining
- **CPU Mining Control** directly from mobile
- **Thread Configuration** (1-8 CPU threads)
- **Real-time Hashrate** monitoring
- **Mining Performance** tracking

### 🔥 Elderfier Staking
- **Node Registration** with 800 XFG minimum stake
- **Consensus Participation** (Fast-Pass/Fall-Back/Full-Quorum)
- **Network Monitoring** of all Elderfier nodes
- **Rewards Tracking** and performance metrics

### 📱 Encrypted Messaging
- **Blockchain Messaging** with end-to-end encryption
- **Self-Destruct Messages** with configurable timers
- **Censorship Resistance** via blockchain storage
- **Complete Privacy** with no metadata tracking

## 🚀 Getting Started

### Prerequisites

- **Flutter SDK**: 3.22.2 or higher
- **Dart SDK**: 3.4.3 or higher
- **Android Studio** / **Xcode** for platform-specific builds
- **Fuego Daemon** running for full functionality

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/usexfg/fuego-flutter.git
   cd fuego-flutter
   ```

2. **Install dependencies**
   ```bash
   flutter pub get
   ```

3. **Run the app**
   ```bash
   flutter run
   ```

### Building for Production

```bash
# Android APK
flutter build apk --release

# Android App Bundle
flutter build appbundle --release

# iOS (requires Xcode)
flutter build ios --release
```

## 🏗️ Architecture

### Project Structure
```
lib/
├── main.dart                    # App entry point
├── models/                      # Data models
├── services/                    # Business logic
│   ├── fuego_rpc_service.dart  # Fuego RPC communication
│   └── security_service.dart   # Security & encryption
├── providers/                   # State management
├── screens/                     # UI screens
│   ├── auth/                   # Authentication
│   ├── wallet_setup/           # Wallet creation/restore
│   ├── home/                   # Main dashboard
│   ├── transactions/           # Send/receive
│   ├── elderfier/              # Staking features
│   ├── messaging/              # Encrypted messaging
│   └── settings/               # Configuration
├── widgets/                     # Reusable components
└── utils/                       # Utilities & theming
```

### State Management
- **Provider Pattern** for reactive state management
- **Clean Architecture** with separation of concerns
- **Comprehensive Error Handling** throughout

### Security Implementation
- **Multi-layered Encryption** with industry standards
- **Hardware Security Integration** when available
- **Memory Protection** with automatic data clearing
- **Secure Key Derivation** and storage

## 🔧 Configuration

### Fuego Node Setup

The app requires a running Fuego daemon for full functionality:

```bash
# Default RPC endpoints
Daemon RPC: http://localhost:28180
Wallet RPC: http://localhost:8070
```

### Network Configuration

You can configure custom node connections in the app settings or modify the default endpoints in:
```dart
// lib/services/fuego_rpc_service.dart
static const int defaultRpcPort = 28180;
static const int defaultWalletPort = 8070;
```

## 📱 Screenshots

| Splash Screen | Wallet Setup | Dashboard | Send Transaction |
|---------------|--------------|-----------|------------------|
| ![Splash](docs/screenshots/splash.png) | ![Setup](docs/screenshots/setup.png) | ![Dashboard](docs/screenshots/dashboard.png) | ![Send](docs/screenshots/send.png) |

| Elderfier Staking | Encrypted Messaging | Mining Control | Settings |
|-------------------|---------------------|----------------|----------|
| ![Elderfier](docs/screenshots/elderfier.png) | ![Messaging](docs/screenshots/messaging.png) | ![Mining](docs/screenshots/mining.png) | ![Settings](docs/screenshots/settings.png) |

## 🔒 Security Features

### Cryptographic Protection
- **AES-256-CBC** encryption for all stored wallet data
- **PBKDF2** key derivation with 100,000 iterations
- **Secure random generation** for keys and entropy
- **Hardware security module** integration when available

### Privacy Features
- **Ring signatures** with configurable mixing levels
- **Payment IDs** for transaction identification
- **Integrated addresses** for enhanced privacy
- **No data tracking** - all operations local

### Authentication Security
- **Multi-factor authentication** with PIN + biometric
- **Failed attempt protection** with progressive lockout
- **Secure session management** with timeout
- **Device binding** with hardware-specific keys

## 🌐 Supported Platforms

- ✅ **Android 7.0+** (API level 24+)
- ✅ **iOS 12.0+**
- 🔄 **Linux Desktop** (ready)
- 🔄 **Windows Desktop** (ready)
- 🔄 **macOS Desktop** (ready)

## 🧪 Testing

```bash
# Run all tests
flutter test

# Run tests with coverage
flutter test --coverage

# Run integration tests
flutter drive --target=test_driver/app.dart
```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes and test thoroughly
4. Commit your changes: `git commit -m 'Add amazing feature'`
5. Push to the branch: `git push origin feature/amazing-feature`
6. Open a Pull Request

### Code Style

- Follow [Flutter's style guide](https://dart.dev/guides/language/effective-dart/style)
- Use `flutter analyze` to check for issues
- Format code with `dart format`
- Write tests for new features

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🔗 Links

- **Fuego Website**: [https://usexfg.org](https://usexfg.org)
- **Main Repository**: [https://github.com/usexfg/fuego](https://github.com/usexfg/fuego)
- **Discord**: [https://discord.gg/5UJcJJg](https://discord.gg/5UJcJJg)
- **Twitter**: [https://twitter.com/useXFG](https://twitter.com/useXFG)

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/usexfg/fuego-flutter/issues)
- **Discord**: [Fuego Community](https://discord.gg/5UJcJJg)
- **Email**: support@usexfg.org

## ⚠️ Disclaimer

This software is provided "as is" without warranty. Cryptocurrency transactions are irreversible. Always:
- **Backup your wallet** securely
- **Verify transactions** before sending
- **Use at your own risk**
- **Keep your backup phrase safe**

---

<p align="center">
  <strong>🔥 Built with ❤️ for the Fuego Community 🔥</strong>
</p>

<p align="center">
  <em>Privacy • Security • Decentralization</em>
</p>