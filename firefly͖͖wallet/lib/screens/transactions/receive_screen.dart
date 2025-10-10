import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:qr_flutter/qr_flutter.dart';
import '../../providers/wallet_provider.dart';
import '../../utils/theme.dart';

class ReceiveScreen extends StatefulWidget {
  const ReceiveScreen({super.key});

  @override
  State<ReceiveScreen> createState() => _ReceiveScreenState();
}

class _ReceiveScreenState extends State<ReceiveScreen>
    with TickerProviderStateMixin {
  final _amountController = TextEditingController();
  final _noteController = TextEditingController();
  
  late AnimationController _fadeController;
  late AnimationController _pulseController;
  late Animation<double> _fadeAnimation;
  late Animation<double> _pulseAnimation;
  
  String? _walletAddress;
  String? _paymentId;
  String? _integratedAddress;
  bool _isLoading = true;
  bool _showAdvanced = false;
  bool _useIntegratedAddress = false;

  @override
  void initState() {
    super.initState();
    _setupAnimations();
    _loadWalletAddress();
  }

  void _setupAnimations() {
    _fadeController = AnimationController(
      duration: const Duration(milliseconds: 800),
      vsync: this,
    );

    _pulseController = AnimationController(
      duration: const Duration(milliseconds: 1500),
      vsync: this,
    );

    _fadeAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _fadeController,
      curve: Curves.easeInOut,
    ));

    _pulseAnimation = Tween<double>(
      begin: 0.95,
      end: 1.05,
    ).animate(CurvedAnimation(
      parent: _pulseController,
      curve: Curves.easeInOut,
    ));

    _fadeController.forward();
    _pulseController.repeat(reverse: true);
  }

  Future<void> _loadWalletAddress() async {
    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      final address = walletProvider.wallet?.address ?? 
          await _getAddressFromProvider();
      
      setState(() {
        _walletAddress = address;
        _isLoading = false;
      });
    } catch (e) {
      setState(() {
        _isLoading = false;
      });
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Failed to load address: $e'),
            backgroundColor: AppTheme.errorColor,
          ),
        );
      }
    }
  }

  Future<String> _getAddressFromProvider() async {
    // Placeholder - in real implementation, get address from wallet service
    return 'fire7rp9y1XyaHBPNmBTSb2VyzuGhNPRrJT5HhTCzBzj39ztFSzTu2qQdeCQpPqr3VxWQK8kj5zk3BHPgCdEz5H8WZZD9ZyRZT2gvGcwHzrVhRJFQ8k';
  }

  Future<void> _generatePaymentId() async {
    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      final paymentId = await walletProvider.generatePaymentId();
      
      setState(() {
        _paymentId = paymentId;
      });
      
      if (_useIntegratedAddress) {
        _generateIntegratedAddress();
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to generate payment ID: $e'),
          backgroundColor: AppTheme.errorColor,
        ),
      );
    }
  }

  Future<void> _generateIntegratedAddress() async {
    if (_paymentId == null || _walletAddress == null) return;

    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      final integrated = await walletProvider.createIntegratedAddress(_paymentId!);
      
      setState(() {
        _integratedAddress = integrated;
      });
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to create integrated address: $e'),
          backgroundColor: AppTheme.errorColor,
        ),
      );
    }
  }

  void _copyToClipboard(String text, String label) {
    Clipboard.setData(ClipboardData(text: text));
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text('$label copied to clipboard'),
        backgroundColor: AppTheme.successColor,
        duration: const Duration(seconds: 2),
      ),
    );
  }

  void _shareAddress() {
    final address = _useIntegratedAddress && _integratedAddress != null
        ? _integratedAddress!
        : _walletAddress ?? '';
    
    if (address.isNotEmpty) {
      // TODO: Implement native sharing
      _copyToClipboard(address, 'Address');
    }
  }

  String _generateQRData() {
    final address = _useIntegratedAddress && _integratedAddress != null
        ? _integratedAddress!
        : _walletAddress ?? '';

    if (address.isEmpty) return '';

    // Create Fuego URI format
    final uri = StringBuffer('fuego:$address');
    
    final amount = _amountController.text.trim();
    final note = _noteController.text.trim();
    
    final params = <String>[];
    
    if (amount.isNotEmpty) {
      params.add('amount=$amount');
    }
    
    if (_paymentId != null && _paymentId!.isNotEmpty && !_useIntegratedAddress) {
      params.add('payment_id=$_paymentId');
    }
    
    if (note.isNotEmpty) {
      params.add('label=${Uri.encodeComponent(note)}');
    }
    
    if (params.isNotEmpty) {
      uri.write('?${params.join('&')}');
    }
    
    return uri.toString();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Receive XFG'),
        elevation: 0,
        backgroundColor: Colors.transparent,
        actions: [
          IconButton(
            onPressed: _shareAddress,
            icon: const Icon(Icons.share),
            tooltip: 'Share Address',
          ),
        ],
      ),
      body: _isLoading
          ? const Center(
              child: CircularProgressIndicator(
                valueColor: AlwaysStoppedAnimation<Color>(AppTheme.primaryColor),
              ),
            )
          : FadeTransition(
              opacity: _fadeAnimation,
              child: SingleChildScrollView(
                padding: const EdgeInsets.all(24),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    // QR Code
                    AnimatedBuilder(
                      animation: _pulseAnimation,
                      builder: (context, child) {
                        return Transform.scale(
                          scale: _pulseAnimation.value,
                          child: Container(
                            padding: const EdgeInsets.all(16),
                            decoration: BoxDecoration(
                              color: Colors.white,
                              borderRadius: BorderRadius.circular(20),
                              boxShadow: [
                                BoxShadow(
                                  color: AppTheme.primaryColor.withOpacity(0.2),
                                  blurRadius: 15,
                                  spreadRadius: 2,
                                ),
                              ],
                            ),
                            child: _walletAddress != null
                                ? QrImageView(
                                    data: _generateQRData(),
                                    version: QrVersions.auto,
                                    size: 200,
                                    backgroundColor: Colors.white,
                                    errorCorrectionLevel: QrErrorCorrectLevel.M,
                                  )
                                : Container(
                                    width: 200,
                                    height: 200,
                                    decoration: BoxDecoration(
                                      color: AppTheme.surfaceColor,
                                      borderRadius: BorderRadius.circular(8),
                                    ),
                                    child: const Center(
                                      child: Text(
                                        'Unable to load QR code',
                                        style: TextStyle(
                                          color: AppTheme.textSecondary,
                                        ),
                                        textAlign: TextAlign.center,
                                      ),
                                    ),
                                  ),
                          ),
                        );
                      },
                    ),
                    const SizedBox(height: 24),

                    // Address display
                    Container(
                      width: double.infinity,
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: AppTheme.cardColor,
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(
                          color: AppTheme.primaryColor.withOpacity(0.3),
                        ),
                      ),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Text(
                                _useIntegratedAddress && _integratedAddress != null
                                    ? 'Integrated Address'
                                    : 'Your Address',
                                style: const TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.w600,
                                  color: AppTheme.textPrimary,
                                ),
                              ),
                              Row(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  IconButton(
                                    onPressed: () {
                                      final address = _useIntegratedAddress && 
                                              _integratedAddress != null
                                          ? _integratedAddress!
                                          : _walletAddress ?? '';
                                      _copyToClipboard(address, 'Address');
                                    },
                                    icon: const Icon(
                                      Icons.copy,
                                      color: AppTheme.primaryColor,
                                      size: 20,
                                    ),
                                    tooltip: 'Copy',
                                  ),
                                ],
                              ),
                            ],
                          ),
                          const SizedBox(height: 8),
                          Container(
                            padding: const EdgeInsets.all(12),
                            decoration: BoxDecoration(
                              color: AppTheme.surfaceColor,
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: SelectableText(
                              _useIntegratedAddress && _integratedAddress != null
                                  ? _integratedAddress!
                                  : _walletAddress ?? 'Loading...',
                              style: const TextStyle(
                                color: AppTheme.textPrimary,
                                fontSize: 14,
                                fontFamily: 'monospace',
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                    const SizedBox(height: 24),

                    // Request specific amount
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: AppTheme.cardColor,
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(
                          color: AppTheme.primaryColor.withOpacity(0.1),
                        ),
                      ),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            'Request Specific Amount (Optional)',
                            style: TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.w600,
                              color: AppTheme.textPrimary,
                            ),
                          ),
                          const SizedBox(height: 12),
                          Row(
                            children: [
                              Expanded(
                                child: TextField(
                                  controller: _amountController,
                                  keyboardType: TextInputType.number,
                                  decoration: const InputDecoration(
                                    hintText: '0.00000000',
                                    suffixText: 'XFG',
                                    isDense: true,
                                  ),
                                  onChanged: (_) => setState(() {}),
                                  inputFormatters: [
                                    FilteringTextInputFormatter.allow(
                                      RegExp(r'^\d*\.?\d{0,8}'),
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                    const SizedBox(height: 16),

                    // Advanced options toggle
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        const Text(
                          'Advanced Options',
                          style: TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                            color: AppTheme.textPrimary,
                          ),
                        ),
                        Switch(
                          value: _showAdvanced,
                          onChanged: (value) {
                            setState(() {
                              _showAdvanced = value;
                            });
                          },
                        ),
                      ],
                    ),

                    if (_showAdvanced) ...[
                      const SizedBox(height: 16),
                      
                      // Payment ID section
                      Container(
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: AppTheme.cardColor,
                          borderRadius: BorderRadius.circular(12),
                          border: Border.all(
                            color: AppTheme.primaryColor.withOpacity(0.1),
                          ),
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                const Text(
                                  'Payment ID',
                                  style: TextStyle(
                                    fontSize: 14,
                                    fontWeight: FontWeight.w600,
                                    color: AppTheme.textPrimary,
                                  ),
                                ),
                                TextButton(
                                  onPressed: _generatePaymentId,
                                  child: const Text('Generate'),
                                ),
                              ],
                            ),
                            if (_paymentId != null) ...[
                              Container(
                                padding: const EdgeInsets.all(12),
                                decoration: BoxDecoration(
                                  color: AppTheme.surfaceColor,
                                  borderRadius: BorderRadius.circular(8),
                                ),
                                child: Row(
                                  children: [
                                    Expanded(
                                      child: SelectableText(
                                        _paymentId!,
                                        style: const TextStyle(
                                          color: AppTheme.textPrimary,
                                          fontSize: 12,
                                          fontFamily: 'monospace',
                                        ),
                                      ),
                                    ),
                                    IconButton(
                                      onPressed: () => _copyToClipboard(
                                        _paymentId!,
                                        'Payment ID',
                                      ),
                                      icon: const Icon(
                                        Icons.copy,
                                        size: 16,
                                        color: AppTheme.primaryColor,
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                              const SizedBox(height: 12),
                              Row(
                                children: [
                                  Checkbox(
                                    value: _useIntegratedAddress,
                                    onChanged: (value) {
                                      setState(() {
                                        _useIntegratedAddress = value ?? false;
                                      });
                                      
                                      if (_useIntegratedAddress && _paymentId != null) {
                                        _generateIntegratedAddress();
                                      }
                                    },
                                  ),
                                  const Expanded(
                                    child: Text(
                                      'Use integrated address',
                                      style: TextStyle(
                                        fontSize: 14,
                                        color: AppTheme.textSecondary,
                                      ),
                                    ),
                                  ),
                                ],
                              ),
                            ],
                          ],
                        ),
                      ),
                      const SizedBox(height: 16),

                      // Note/Label
                      Container(
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: AppTheme.cardColor,
                          borderRadius: BorderRadius.circular(12),
                          border: Border.all(
                            color: AppTheme.primaryColor.withOpacity(0.1),
                          ),
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            const Text(
                              'Note/Label (Optional)',
                              style: TextStyle(
                                fontSize: 14,
                                fontWeight: FontWeight.w600,
                                color: AppTheme.textPrimary,
                              ),
                            ),
                            const SizedBox(height: 8),
                            TextField(
                              controller: _noteController,
                              decoration: const InputDecoration(
                                hintText: 'Payment for...',
                                isDense: true,
                              ),
                              onChanged: (_) => setState(() {}),
                              maxLength: 100,
                            ),
                          ],
                        ),
                      ),
                    ],
                    const SizedBox(height: 24),

                    // Information
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
                                size: 20,
                              ),
                              SizedBox(width: 8),
                              Text(
                                'How to Receive',
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
                            '• Share your address or QR code with the sender\n'
                            '• Use payment IDs to identify specific payments\n'
                            '• Transactions are private and untraceable\n'
                            '• New transactions will appear automatically',
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
            ),
    );
  }

  @override
  void dispose() {
    _amountController.dispose();
    _noteController.dispose();
    _fadeController.dispose();
    _pulseController.dispose();
    super.dispose();
  }
}