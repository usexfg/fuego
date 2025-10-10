# Fuego Wallet - Flutter Mobile App

A comprehensive mobile wallet application for the Fuego (XFG) privacy-focused cryptocurrency, built with Flutter.

## 🚀 Features Implemented

### 🔐 Security & Authentication
- **PIN Authentication**: 6-digit PIN with secure hashing using PBKDF2
- **Biometric Authentication**: Fingerprint/Face ID support when available
- **Secure Storage**: Encrypted wallet data storage using device keychain
- **Mnemonic Generation**: 25-word backup phrases for wallet recovery
- **Wallet Encryption**: AES-256-CBC encryption for sensitive data

### 💼 Wallet Management
- **Create New Wallet**: Generate new wallets with secure mnemonic phrases
- **Restore Wallet**: Recover wallets from 25-word backup phrases  
- **Balance Display**: Real-time XFG balance with privacy toggle
- **Multi-step Setup**: Guided wallet creation with confirmation steps

### 💸 Transactions
- **Send XFG**: Complete send functionality with privacy controls
- **Receive XFG**: Generate QR codes and integrated addresses
- **Transaction History**: View recent transactions with status tracking
- **Privacy Levels**: Configurable ring signature mixing (0-15 mixins)
- **Payment IDs**: Support for payment identification
- **Fee Estimation**: Automatic fee calculation for transactions

### 🖥️ User Interface
- **Modern Design**: Dark theme optimized for cryptocurrency use
- **Animated UI**: Smooth transitions and loading animations
- **Responsive Layout**: Adaptive design for various screen sizes
- **QR Code Support**: Generate and scan QR codes for addresses
- **Real-time Updates**: Live balance and transaction updates

### ⛏️ Mining Features
- **Start/Stop Mining**: Control CPU mining directly from the app
- **Thread Configuration**: Adjustable CPU thread count (1-8)
- **Mining Status**: Real-time hashrate and thread monitoring
- **Mining Controls**: Easy toggle with status indicators

### 🌐 Network Features
- **RPC Communication**: Full Fuego daemon integration
- **Connection Status**: Real-time network connectivity monitoring
- **Sync Progress**: Blockchain synchronization progress display
- **Multiple Nodes**: Support for connecting to different Fuego nodes

### 🎯 Fuego-Specific Features
- **Ring Signatures**: Enhanced privacy with configurable mixing
- **CryptoNote Protocol**: Built on proven privacy technology  
- **Elderfier Integration**: (Framework ready for Elderfier node features)
- **Encrypted Messaging**: (Framework ready for blockchain messaging)

## 🏗️ Architecture

### Project Structure
```
lib/
├── main.dart                    # App entry point
├── models/                      # Data models
│   ├── wallet.dart             # Wallet and transaction models
│   └── wallet.g.dart           # Generated JSON serialization
├── services/                    # Business logic layer
│   ├── fuego_rpc_service.dart  # Fuego RPC communication
│   └── security_service.dart   # Security and encryption
├── providers/                   # State management
│   └── wallet_provider.dart    # Wallet state provider
├── screens/                     # UI screens
│   ├── splash_screen.dart      # App initialization
│   ├── auth/                   # Authentication screens
│   ├── wallet_setup/           # Wallet setup flow
│   ├── home/                   # Main dashboard
│   └── transactions/           # Send/receive screens
├── widgets/                     # Reusable UI components
│   ├── balance_card.dart       # Balance display
│   ├── pin_input_widget.dart   # PIN entry component
│   ├── quick_actions.dart      # Action buttons
│   └── recent_transactions.dart # Transaction list
└── utils/                       # Utilities
    └── theme.dart              # App theming
```

### State Management
- **Provider Pattern**: Reactive state management using the Provider package
- **Separation of Concerns**: Clear separation between UI, business logic, and data
- **Error Handling**: Comprehensive error handling and user feedback

### Security Architecture  
- **Layered Security**: Multiple security layers from encryption to authentication
- **No Key Storage**: Private keys never stored in plain text
- **Secure Communication**: All RPC calls use secure protocols
- **Local Processing**: All cryptographic operations performed locally

## 🔧 Technical Specifications

### Dependencies
- **Flutter SDK**: 3.22.2+
- **Provider**: State management 
- **Cryptography**: AES encryption and key derivation
- **Local Auth**: Biometric authentication
- **QR Flutter**: QR code generation
- **Dio**: HTTP client for RPC calls
- **Secure Storage**: Encrypted device storage

### Supported Platforms
- ✅ **Android**: Full support with biometric authentication
- ✅ **iOS**: Full support with Face ID/Touch ID
- 🔄 **Linux/Windows/macOS**: Desktop support ready

### Network Requirements
- **Fuego Daemon**: Requires running Fuego daemon for full functionality
- **RPC Ports**: 
  - 28180 (Daemon RPC)
  - 8070 (Wallet Service RPC)

## 🚀 Getting Started

### Prerequisites
```bash
# Install Flutter
curl -fsSL https://storage.googleapis.com/flutter_infra_release/releases/stable/linux/flutter_linux_3.22.2-stable.tar.xz | tar -xJ
export PATH="$PATH:$(pwd)/flutter/bin"

# Verify installation
flutter doctor
```

### Setup & Run
```bash
# Clone and setup
cd fuego_wallet
flutter pub get

# Run the app
flutter run
```

### Build for Production
```bash
# Android APK
flutter build apk --release

# iOS (requires Xcode)
flutter build ios --release

# Desktop
flutter build linux --release
```

## 📱 Usage Flow

1. **First Launch**: Splash screen with initialization
2. **Wallet Setup**: Create new or restore existing wallet
3. **Security Setup**: Configure PIN and biometric authentication  
4. **Main Dashboard**: View balance, transactions, and quick actions
5. **Send XFG**: Select recipient, amount, and privacy level
6. **Receive XFG**: Generate QR codes and share addresses
7. **Mining**: Start/stop CPU mining with thread control

## 🔒 Security Features

- **Private Key Protection**: Keys encrypted with user PIN using PBKDF2
- **Biometric Integration**: Native fingerprint/Face ID support
- **Secure Element**: Utilizes device security features when available
- **Memory Protection**: Sensitive data cleared from memory after use
- **Network Security**: Secure RPC communication with error handling

## 🎨 Design Philosophy

- **Privacy First**: Dark theme and privacy-focused UI elements
- **User Experience**: Intuitive navigation with clear feedback
- **Security Transparency**: Clear security indicators and warnings
- **Accessibility**: Support for screen readers and accessibility features
- **Performance**: Optimized for smooth operation on mobile devices

## 🛣️ Future Enhancements

The app framework is designed to easily accommodate:
- **Elderfier Staking**: Full Elderfier node management interface
- **Encrypted Messaging**: Blockchain-based secure messaging
- **Multi-currency**: Support for additional privacy coins
- **Hardware Wallets**: Integration with hardware wallet devices
- **Advanced Trading**: DEX integration and trading features

## 📄 License

This project is part of the Fuego cryptocurrency ecosystem and follows the same open-source principles as the main Fuego project.

## 🤝 Contributing

The codebase is structured for easy contribution:
- Clear separation of concerns
- Comprehensive error handling
- Well-documented code
- Modular architecture
- Test-ready structure

---

**The Fuego Flutter Wallet provides a complete, secure, and user-friendly mobile interface for the Fuego privacy cryptocurrency, bringing advanced cryptographic features to mobile users with an intuitive and secure interface.**