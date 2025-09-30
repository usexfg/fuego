import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../utils/theme.dart';

class SendScreen extends StatefulWidget {
  const SendScreen({super.key});

  @override
  State<SendScreen> createState() => _SendScreenState();
}

class _SendScreenState extends State<SendScreen> {
  final _formKey = GlobalKey<FormState>();
  final _addressController = TextEditingController();
  final _amountController = TextEditingController();
  final _paymentIdController = TextEditingController();
  final _addressFocusNode = FocusNode();
  final _amountFocusNode = FocusNode();
  final _paymentIdFocusNode = FocusNode();

  bool _isLoading = false;
  int _mixins = 7; // Default privacy level
  String? _errorMessage;
  bool _showAdvanced = false;

  @override
  void dispose() {
    _addressController.dispose();
    _amountController.dispose();
    _paymentIdController.dispose();
    _addressFocusNode.dispose();
    _amountFocusNode.dispose();
    _paymentIdFocusNode.dispose();
    super.dispose();
  }

  Future<void> _sendTransaction() async {
    if (!_formKey.currentState!.validate()) return;

    setState(() {
      _isLoading = true;
      _errorMessage = null;
    });

    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      
      final txHash = await walletProvider.sendTransaction(
        address: _addressController.text.trim(),
        amount: double.parse(_amountController.text),
        paymentId: _paymentIdController.text.trim().isEmpty 
            ? null 
            : _paymentIdController.text.trim(),
        mixins: _mixins,
      );

      if (txHash != null && mounted) {
        _showSuccessDialog(txHash);
      } else {
        setState(() {
          _errorMessage = walletProvider.error ?? 'Transaction failed';
        });
      }
    } catch (e) {
      setState(() {
        _errorMessage = e.toString();
      });
    } finally {
      setState(() {
        _isLoading = false;
      });
    }
  }

  void _showSuccessDialog(String txHash) {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return AlertDialog(
          backgroundColor: AppTheme.cardColor,
          title: const Row(
            children: [
              Icon(
                Icons.check_circle,
                color: AppTheme.successColor,
              ),
              SizedBox(width: 8),
              Text(
                'Transaction Sent',
                style: TextStyle(color: AppTheme.textPrimary),
              ),
            ],
          ),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'Your transaction has been broadcast to the network.',
                style: TextStyle(color: AppTheme.textSecondary),
              ),
              const SizedBox(height: 16),
              Text(
                'Transaction ID:',
                style: TextStyle(
                  color: AppTheme.textPrimary,
                  fontWeight: FontWeight.w600,
                ),
              ),
              const SizedBox(height: 8),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: AppTheme.surfaceColor,
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(
                    color: AppTheme.textMuted.withOpacity(0.3),
                  ),
                ),
                child: Row(
                  children: [
                    Expanded(
                      child: Text(
                        txHash,
                        style: const TextStyle(
                          color: AppTheme.textPrimary,
                          fontSize: 12,
                          fontFamily: 'monospace',
                        ),
                        maxLines: 2,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                    IconButton(
                      onPressed: () {
                        Clipboard.setData(ClipboardData(text: txHash));
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Transaction ID copied'),
                            backgroundColor: AppTheme.successColor,
                          ),
                        );
                      },
                      icon: const Icon(
                        Icons.copy,
                        size: 16,
                        color: AppTheme.primaryColor,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
          actions: [
            ElevatedButton(
              onPressed: () {
                Navigator.of(context).pop(); // Close dialog
                Navigator.of(context).pop(); // Return to home
              },
              child: const Text('Done'),
            ),
          ],
        );
      },
    );
  }

  void _pasteFromClipboard() async {
    final clipboardData = await Clipboard.getData('text/plain');
    if (clipboardData?.text != null) {
      _addressController.text = clipboardData!.text!;
    }
  }

  void _generatePaymentId() async {
    final walletProvider = Provider.of<WalletProvider>(context, listen: false);
    final paymentId = await walletProvider.generatePaymentId();
    _paymentIdController.text = paymentId;
  }

  void _setMaxAmount() {
    final walletProvider = Provider.of<WalletProvider>(context, listen: false);
    final availableBalance = walletProvider.wallet?.unlockedBalanceXFG ?? 0.0;
    
    // Reserve some amount for fees (approximately 0.01 XFG)
    final maxAmount = (availableBalance - 0.01).clamp(0.0, availableBalance);
    _amountController.text = maxAmount.toStringAsFixed(8);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Send XFG'),
        elevation: 0,
        backgroundColor: Colors.transparent,
      ),
      body: Consumer<WalletProvider>(
        builder: (context, walletProvider, child) {
          final availableBalance = walletProvider.wallet?.unlockedBalanceXFG ?? 0.0;
          
          return SingleChildScrollView(
            padding: const EdgeInsets.all(24),
            child: Form(
              key: _formKey,
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // Available balance
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
                        const Text(
                          'Available Balance',
                          style: TextStyle(
                            fontSize: 14,
                            color: AppTheme.textSecondary,
                          ),
                        ),
                        const SizedBox(height: 4),
                        Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Text(
                              '${availableBalance.toStringAsFixed(8)} XFG',
                              style: const TextStyle(
                                fontSize: 20,
                                fontWeight: FontWeight.bold,
                                color: AppTheme.textPrimary,
                              ),
                            ),
                            TextButton(
                              onPressed: _setMaxAmount,
                              child: const Text('MAX'),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: 24),

                  // Recipient address
                  const Text(
                    'Recipient Address',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: AppTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _addressController,
                    focusNode: _addressFocusNode,
                    decoration: InputDecoration(
                      hintText: 'fire... or integrated address',
                      suffixIcon: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          IconButton(
                            onPressed: _pasteFromClipboard,
                            icon: const Icon(Icons.paste),
                            tooltip: 'Paste',
                          ),
                          IconButton(
                            onPressed: () {
                              // TODO: Implement QR scanner
                              ScaffoldMessenger.of(context).showSnackBar(
                                const SnackBar(
                                  content: Text('QR scanner coming soon'),
                                ),
                              );
                            },
                            icon: const Icon(Icons.qr_code_scanner),
                            tooltip: 'Scan QR',
                          ),
                        ],
                      ),
                    ),
                    validator: (value) {
                      if (value == null || value.trim().isEmpty) {
                        return 'Please enter recipient address';
                      }
                      // Basic validation for Fuego address format
                      if (!value.startsWith('fire') && value.length < 50) {
                        return 'Invalid address format';
                      }
                      return null;
                    },
                    maxLines: 3,
                    minLines: 1,
                  ),
                  const SizedBox(height: 24),

                  // Amount
                  const Text(
                    'Amount',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: AppTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _amountController,
                    focusNode: _amountFocusNode,
                    keyboardType: TextInputType.number,
                    decoration: const InputDecoration(
                      hintText: '0.00000000',
                      suffixText: 'XFG',
                    ),
                    validator: (value) {
                      if (value == null || value.trim().isEmpty) {
                        return 'Please enter amount';
                      }
                      final amount = double.tryParse(value);
                      if (amount == null || amount <= 0) {
                        return 'Please enter a valid amount';
                      }
                      if (amount > availableBalance) {
                        return 'Insufficient balance';
                      }
                      return null;
                    },
                    inputFormatters: [
                      FilteringTextInputFormatter.allow(RegExp(r'^\d*\.?\d{0,8}')),
                    ],
                  ),
                  const SizedBox(height: 24),

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

                  // Advanced options
                  if (_showAdvanced) ...[
                    const SizedBox(height: 16),
                    
                    // Payment ID
                    const Text(
                      'Payment ID (Optional)',
                      style: TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.w600,
                        color: AppTheme.textPrimary,
                      ),
                    ),
                    const SizedBox(height: 8),
                    TextFormField(
                      controller: _paymentIdController,
                      focusNode: _paymentIdFocusNode,
                      decoration: InputDecoration(
                        hintText: 'Leave empty for automatic generation',
                        suffixIcon: IconButton(
                          onPressed: _generatePaymentId,
                          icon: const Icon(Icons.auto_fix_high),
                          tooltip: 'Generate',
                        ),
                      ),
                      maxLength: 64,
                    ),
                    const SizedBox(height: 16),

                    // Privacy level (mixins)
                    Text(
                      'Privacy Level: $_mixins mixins',
                      style: const TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.w600,
                        color: AppTheme.textPrimary,
                      ),
                    ),
                    const SizedBox(height: 8),
                    Slider(
                      value: _mixins.toDouble(),
                      min: 0,
                      max: 15,
                      divisions: 15,
                      label: '$_mixins mixins',
                      onChanged: (value) {
                        setState(() {
                          _mixins = value.round();
                        });
                      },
                    ),
                    Text(
                      _mixins == 0 
                          ? 'No privacy (not recommended)'
                          : _mixins <= 3
                              ? 'Low privacy'
                              : _mixins <= 7
                                  ? 'Normal privacy'
                                  : 'High privacy',
                      style: TextStyle(
                        fontSize: 12,
                        color: _mixins == 0 
                            ? AppTheme.errorColor
                            : AppTheme.textSecondary,
                      ),
                    ),
                  ],
                  const SizedBox(height: 32),

                  // Error message
                  if (_errorMessage != null) ...[
                    Container(
                      width: double.infinity,
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: AppTheme.errorColor.withOpacity(0.1),
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(
                          color: AppTheme.errorColor.withOpacity(0.3),
                        ),
                      ),
                      child: Row(
                        children: [
                          const Icon(
                            Icons.error_outline,
                            color: AppTheme.errorColor,
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: Text(
                              _errorMessage!,
                              style: const TextStyle(
                                color: AppTheme.errorColor,
                                fontSize: 14,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                    const SizedBox(height: 16),
                  ],

                  // Send button
                  SizedBox(
                    width: double.infinity,
                    height: 56,
                    child: ElevatedButton(
                      onPressed: _isLoading || availableBalance <= 0 
                          ? null 
                          : _sendTransaction,
                      child: _isLoading
                          ? const SizedBox(
                              height: 20,
                              width: 20,
                              child: CircularProgressIndicator(
                                strokeWidth: 2,
                                valueColor: AlwaysStoppedAnimation<Color>(
                                  Colors.white,
                                ),
                              ),
                            )
                          : const Text(
                              'Send Transaction',
                              style: TextStyle(
                                fontSize: 18,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                    ),
                  ),
                  const SizedBox(height: 16),

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
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Icon(
                          Icons.info_outline,
                          color: AppTheme.warningColor,
                          size: 20,
                        ),
                        const SizedBox(width: 12),
                        Expanded(
                          child: Text(
                            'Transactions cannot be reversed. Please double-check the recipient address before sending.',
                            style: TextStyle(
                              color: AppTheme.textSecondary,
                              fontSize: 14,
                              height: 1.4,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }
}