import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../services/security_service.dart';
import '../../utils/theme.dart';
import '../main/main_screen.dart';
import '../../widgets/pin_input_widget.dart';

class PinSetupScreen extends StatefulWidget {
  final String mnemonic;
  final bool isRestore;

  const PinSetupScreen({
    super.key,
    required this.mnemonic,
    this.isRestore = false,
  });

  @override
  State<PinSetupScreen> createState() => _PinSetupScreenState();
}

class _PinSetupScreenState extends State<PinSetupScreen> {
  final PageController _pageController = PageController();
  int _currentPage = 0;
  String _firstPin = '';
  String _confirmPin = '';
  bool _isLoading = false;
  bool _biometricEnabled = false;
  String? _errorMessage;

  @override
  void initState() {
    super.initState();
    _checkBiometricAvailability();
  }

  Future<void> _checkBiometricAvailability() async {
    final securityService = SecurityService();
    final available = await securityService.isBiometricAvailable();
    if (mounted) {
      setState(() {
        _biometricEnabled = available;
      });
    }
  }

  void _nextPage() {
    if (_currentPage < 2) {
      _pageController.nextPage(
        duration: const Duration(milliseconds: 300),
        curve: Curves.easeInOut,
      );
    } else {
      _createWallet();
    }
  }

  void _previousPage() {
    if (_currentPage > 0) {
      _pageController.previousPage(
        duration: const Duration(milliseconds: 300),
        curve: Curves.easeInOut,
      );
    }
  }

  void _onFirstPinComplete(String pin) {
    setState(() {
      _firstPin = pin;
      _errorMessage = null;
    });
    _nextPage();
  }

  void _onConfirmPinComplete(String pin) {
    setState(() {
      _confirmPin = pin;
      _errorMessage = null;
    });

    if (_firstPin == _confirmPin) {
      _nextPage();
    } else {
      setState(() {
        _errorMessage = 'PINs do not match. Please try again.';
        _confirmPin = '';
      });
    }
  }

  Future<void> _createWallet() async {
    setState(() {
      _isLoading = true;
      _errorMessage = null;
    });

    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      final securityService = SecurityService();

      bool success;
      if (widget.isRestore) {
        success = await walletProvider.restoreWallet(
          mnemonic: widget.mnemonic,
          pin: _firstPin,
        );
      } else {
        success = await walletProvider.createWallet(
          pin: _firstPin,
          mnemonic: widget.mnemonic,
        );
      }

      if (success) {
        // Set up biometric authentication if enabled
        if (_biometricEnabled) {
          await securityService.setBiometricEnabled(true);
        }

        // Navigate to main screen
        if (mounted) {
          Navigator.of(context).pushReplacement(
            MaterialPageRoute(builder: (context) => const MainScreen()),
          );
        }
      } else {
        setState(() {
          _errorMessage = walletProvider.error ?? 'Failed to create wallet';
          _isLoading = false;
        });
      }
    } catch (e) {
      setState(() {
        _errorMessage = e.toString();
        _isLoading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.isRestore ? 'Restore Wallet' : 'Create Wallet'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () {
            if (_currentPage > 0) {
              _previousPage();
            } else {
              Navigator.of(context).pop();
            }
          },
        ),
      ),
      body: Column(
        children: [
          // Step indicator
          Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: List.generate(3, (index) {
                return Container(
                  margin: const EdgeInsets.symmetric(horizontal: 4),
                  width: 8,
                  height: 8,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: index <= _currentPage
                        ? AppTheme.primaryColor
                        : AppTheme.textMuted,
                  ),
                );
              }),
            ),
          ),
          Expanded(
            child: PageView(
              controller: _pageController,
              physics: const NeverScrollableScrollPhysics(),
              onPageChanged: (page) {
                setState(() {
                  _currentPage = page;
                });
              },
              children: [
                _buildSetupPinPage(),
                _buildConfirmPinPage(),
                _buildSecurityOptionsPage(),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSetupPinPage() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        children: [
          const SizedBox(height: 32),
          const Icon(
            Icons.lock_outline,
            size: 64,
            color: AppTheme.primaryColor,
          ),
          const SizedBox(height: 24),
          const Text(
            'Create Your PIN',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
              color: AppTheme.textPrimary,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Create a 6-digit PIN to secure your wallet',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 16,
              color: AppTheme.textSecondary,
            ),
          ),
          const SizedBox(height: 48),
          PinInputWidget(
            onComplete: _onFirstPinComplete,
            errorMessage: _errorMessage,
          ),
          const SizedBox(height: 32),
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: AppTheme.primaryColor.withOpacity(0.1),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: AppTheme.primaryColor.withOpacity(0.3),
              ),
            ),
            child: Row(
              children: [
                const Icon(
                  Icons.info_outline,
                  color: AppTheme.primaryColor,
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    'Your PIN is stored securely on this device and cannot be recovered if forgotten.',
                    style: TextStyle(
                      fontSize: 14,
                      color: AppTheme.textSecondary,
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildConfirmPinPage() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        children: [
          const SizedBox(height: 32),
          const Icon(
            Icons.verified_outlined,
            size: 64,
            color: AppTheme.primaryColor,
          ),
          const SizedBox(height: 24),
          const Text(
            'Confirm Your PIN',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
              color: AppTheme.textPrimary,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Enter your PIN again to confirm',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 16,
              color: AppTheme.textSecondary,
            ),
          ),
          const SizedBox(height: 48),
          PinInputWidget(
            onComplete: _onConfirmPinComplete,
            errorMessage: _errorMessage,
          ),
          if (_errorMessage != null) ...[
            const SizedBox(height: 16),
            TextButton(
              onPressed: () {
                setState(() {
                  _firstPin = '';
                  _confirmPin = '';
                  _errorMessage = null;
                });
                _previousPage();
              },
              child: const Text('Change PIN'),
            ),
          ],
        ],
      ),
    );
  }

  Widget _buildSecurityOptionsPage() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        children: [
          const SizedBox(height: 32),
          const Icon(
            Icons.fingerprint,
            size: 64,
            color: AppTheme.primaryColor,
          ),
          const SizedBox(height: 24),
          const Text(
            'Security Options',
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.bold,
              color: AppTheme.textPrimary,
            ),
          ),
          const SizedBox(height: 8),
          Text(
            'Choose additional security features for your wallet',
            textAlign: TextAlign.center,
            style: TextStyle(
              fontSize: 16,
              color: AppTheme.textSecondary,
            ),
          ),
          const SizedBox(height: 48),
          
          // Biometric option
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: AppTheme.cardColor,
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: AppTheme.textMuted.withOpacity(0.3),
              ),
            ),
            child: Row(
              children: [
                const Icon(
                  Icons.fingerprint,
                  color: AppTheme.primaryColor,
                  size: 32,
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        'Biometric Authentication',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.w600,
                          color: AppTheme.textPrimary,
                        ),
                      ),
                      Text(
                        'Use fingerprint or face recognition',
                        style: TextStyle(
                          fontSize: 14,
                          color: AppTheme.textSecondary,
                        ),
                      ),
                    ],
                  ),
                ),
                Switch(
                  value: _biometricEnabled,
                  onChanged: (value) {
                    setState(() {
                      _biometricEnabled = value;
                    });
                  },
                ),
              ],
            ),
          ),
          const SizedBox(height: 48),
          
          // Create wallet button
          SizedBox(
            width: double.infinity,
            child: ElevatedButton(
              onPressed: _isLoading ? null : _createWallet,
              child: _isLoading
                  ? const SizedBox(
                      height: 20,
                      width: 20,
                      child: CircularProgressIndicator(
                        strokeWidth: 2,
                        valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
                      ),
                    )
                  : Text(widget.isRestore ? 'Restore Wallet' : 'Create Wallet'),
            ),
          ),
          
          if (_errorMessage != null) ...[
            const SizedBox(height: 16),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: AppTheme.errorColor.withOpacity(0.1),
                borderRadius: BorderRadius.circular(8),
                border: Border.all(
                  color: AppTheme.errorColor.withOpacity(0.3),
                ),
              ),
              child: Row(
                children: [
                  const Icon(
                    Icons.error_outline,
                    color: AppTheme.errorColor,
                    size: 20,
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      _errorMessage!,
                      style: const TextStyle(
                        fontSize: 14,
                        color: AppTheme.errorColor,
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ],
      ),
    );
  }

  @override
  void dispose() {
    _pageController.dispose();
    super.dispose();
  }
}