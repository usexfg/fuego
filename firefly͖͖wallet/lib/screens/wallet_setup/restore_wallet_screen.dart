import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../services/security_service.dart';
import '../../utils/theme.dart';
import '../auth/pin_setup_screen.dart';

class RestoreWalletScreen extends StatefulWidget {
  const RestoreWalletScreen({super.key});

  @override
  State<RestoreWalletScreen> createState() => _RestoreWalletScreenState();
}

class _RestoreWalletScreenState extends State<RestoreWalletScreen> {
  final TextEditingController _mnemonicController = TextEditingController();
  final FocusNode _mnemonicFocusNode = FocusNode();
  bool _isValidMnemonic = false;
  bool _isLoading = false;
  String? _errorMessage;
  int _wordCount = 0;

  @override
  void initState() {
    super.initState();
    _mnemonicController.addListener(_validateMnemonic);
  }

  @override
  void dispose() {
    _mnemonicController.dispose();
    _mnemonicFocusNode.dispose();
    super.dispose();
  }

  void _validateMnemonic() {
    final text = _mnemonicController.text.trim();
    final words = text.isEmpty ? [] : text.split(RegExp(r'\s+'));
    
    setState(() {
      _wordCount = words.length;
      _isValidMnemonic = SecurityService.validateMnemonic(text);
      _errorMessage = null;
    });
  }

  Future<void> _restoreWallet() async {
    if (!_isValidMnemonic) return;

    setState(() {
      _isLoading = true;
      _errorMessage = null;
    });

    try {
      final mnemonic = _mnemonicController.text.trim();
      
      // Navigate to PIN setup with the mnemonic
      Navigator.of(context).pushReplacement(
        MaterialPageRoute(
          builder: (context) => PinSetupScreen(
            mnemonic: mnemonic,
            isRestore: true,
          ),
        ),
      );
    } catch (e) {
      setState(() {
        _errorMessage = e.toString();
        _isLoading = false;
      });
    }
  }

  void _pasteFromClipboard() async {
    // This would get text from clipboard - simplified for demo
    // In real implementation, use Clipboard.getData()
    const sampleMnemonic = 'abandon ability able about above absent absorb abstract absurd abuse access accident account accuse achieve acid acoustic acquire across act action actor actress actual adapt';
    _mnemonicController.text = sampleMnemonic;
    _validateMnemonic();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Restore Wallet'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.of(context).pop(),
        ),
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header
            const Icon(
              Icons.restore,
              size: 64,
              color: AppTheme.primaryColor,
            ),
            const SizedBox(height: 16),
            const Text(
              'Restore Your Wallet',
              style: TextStyle(
                fontSize: 28,
                fontWeight: FontWeight.bold,
                color: AppTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Enter your 25-word backup phrase to restore your existing Fuego wallet',
              style: TextStyle(
                fontSize: 16,
                color: AppTheme.textSecondary,
                height: 1.5,
              ),
            ),
            const SizedBox(height: 32),

            // Mnemonic input
            const Text(
              'Backup Phrase',
              style: TextStyle(
                fontSize: 16,
                fontWeight: FontWeight.w600,
                color: AppTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 8),
            Container(
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(12),
                border: Border.all(
                  color: _errorMessage != null
                      ? AppTheme.errorColor
                      : _isValidMnemonic
                          ? AppTheme.successColor
                          : AppTheme.textMuted.withOpacity(0.3),
                ),
              ),
              child: TextField(
                controller: _mnemonicController,
                focusNode: _mnemonicFocusNode,
                maxLines: 6,
                decoration: InputDecoration(
                  hintText: 'Enter your 25-word backup phrase...',
                  border: InputBorder.none,
                  contentPadding: const EdgeInsets.all(16),
                  suffixIcon: IconButton(
                    icon: const Icon(Icons.paste),
                    onPressed: _pasteFromClipboard,
                    tooltip: 'Paste from clipboard',
                  ),
                ),
                style: const TextStyle(
                  fontSize: 16,
                  height: 1.5,
                ),
              ),
            ),
            const SizedBox(height: 12),

            // Word count and validation
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  'Words: $_wordCount/25',
                  style: TextStyle(
                    fontSize: 14,
                    color: _wordCount == 25
                        ? AppTheme.successColor
                        : AppTheme.textSecondary,
                    fontWeight: FontWeight.w500,
                  ),
                ),
                if (_isValidMnemonic)
                  const Row(
                    children: [
                      Icon(
                        Icons.check_circle,
                        size: 16,
                        color: AppTheme.successColor,
                      ),
                      SizedBox(width: 4),
                      Text(
                        'Valid phrase',
                        style: TextStyle(
                          fontSize: 14,
                          color: AppTheme.successColor,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ],
                  ),
              ],
            ),
            const SizedBox(height: 8),

            // Error message
            if (_errorMessage != null)
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
            const SizedBox(height: 24),

            // Information card
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: AppTheme.primaryColor.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(
                  color: AppTheme.primaryColor.withOpacity(0.3),
                ),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Row(
                    children: [
                      Icon(
                        Icons.info_outline,
                        color: AppTheme.primaryColor,
                      ),
                      SizedBox(width: 8),
                      Text(
                        'Restore Information',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                          color: AppTheme.primaryColor,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  Text(
                    '• Your backup phrase is case-insensitive\n'
                    '• Extra spaces will be automatically removed\n'
                    '• Make sure all 25 words are spelled correctly\n'
                    '• This process is secure and happens locally',
                    style: TextStyle(
                      fontSize: 14,
                      color: AppTheme.textSecondary,
                      height: 1.5,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 32),

            // Restore button
            SizedBox(
              width: double.infinity,
              child: ElevatedButton(
                onPressed: _isValidMnemonic && !_isLoading ? _restoreWallet : null,
                child: _isLoading
                    ? const SizedBox(
                        height: 20,
                        width: 20,
                        child: CircularProgressIndicator(
                          strokeWidth: 2,
                          valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
                        ),
                      )
                    : const Text('Restore Wallet'),
              ),
            ),
            const SizedBox(height: 16),

            // Alternative options
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                TextButton.icon(
                  onPressed: () {
                    // Show QR code scanner - placeholder
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(
                        content: Text('QR code scanner coming soon'),
                      ),
                    );
                  },
                  icon: const Icon(Icons.qr_code_scanner),
                  label: const Text('Scan QR Code'),
                ),
              ],
            ),
            const SizedBox(height: 24),

            // Security notice
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: AppTheme.warningColor.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(
                  color: AppTheme.warningColor.withOpacity(0.3),
                ),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Row(
                    children: [
                      Icon(
                        Icons.security,
                        color: AppTheme.warningColor,
                      ),
                      SizedBox(width: 8),
                      Text(
                        'Security Reminder',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                          color: AppTheme.warningColor,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Never enter your backup phrase on untrusted devices or websites. '
                    'Anyone with access to your backup phrase can control your wallet.',
                    style: TextStyle(
                      fontSize: 14,
                      color: AppTheme.textSecondary,
                      height: 1.5,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}