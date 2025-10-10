import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../utils/theme.dart';

class RegisterElderfierScreen extends StatefulWidget {
  const RegisterElderfierScreen({super.key});

  @override
  State<RegisterElderfierScreen> createState() => _RegisterElderfierScreenState();
}

class _RegisterElderfierScreenState extends State<RegisterElderfierScreen> {
  final _formKey = GlobalKey<FormState>();
  final _nameController = TextEditingController();
  final _stakeController = TextEditingController();
  
  bool _isLoading = false;
  String? _errorMessage;
  double _stakeAmount = 800.0; // Minimum stake

  static const double minStake = 800.0;
  static const double maxStake = 10000.0;

  @override
  void initState() {
    super.initState();
    _stakeController.text = minStake.toStringAsFixed(0);
  }

  @override
  void dispose() {
    _nameController.dispose();
    _stakeController.dispose();
    super.dispose();
  }

  Future<void> _registerElderfier() async {
    if (!_formKey.currentState!.validate()) return;

    setState(() {
      _isLoading = true;
      _errorMessage = null;
    });

    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      final walletAddress = walletProvider.wallet?.address ?? '';
      
      if (walletAddress.isEmpty) {
        throw Exception('Wallet address not available');
      }

      final success = await walletProvider.registerElderfierNode(
        customName: _nameController.text.trim(),
        address: walletAddress,
        stakeAmount: _stakeAmount,
      );

      if (success && mounted) {
        _showSuccessDialog();
      } else {
        setState(() {
          _errorMessage = walletProvider.error ?? 'Failed to register Elderfier node';
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

  void _showSuccessDialog() {
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
                'Registration Successful',
                style: TextStyle(color: AppTheme.textPrimary),
              ),
            ],
          ),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'Your Elderfier node has been successfully registered on the network.',
                style: TextStyle(color: AppTheme.textSecondary),
              ),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: AppTheme.successColor.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(
                    color: AppTheme.successColor.withOpacity(0.3),
                  ),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Node Details:',
                      style: TextStyle(
                        color: AppTheme.textPrimary,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      'Name: ${_nameController.text.trim()}',
                      style: TextStyle(color: AppTheme.textSecondary),
                    ),
                    Text(
                      'Stake: ${_stakeAmount.toStringAsFixed(0)} XFG',
                      style: TextStyle(color: AppTheme.textSecondary),
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
                Navigator.of(context).pop(); // Return to previous screen
              },
              child: const Text('Done'),
            ),
          ],
        );
      },
    );
  }

  void _onStakeChanged(String value) {
    final stake = double.tryParse(value) ?? minStake;
    setState(() {
      _stakeAmount = stake.clamp(minStake, maxStake);
    });
  }

  void _setMinimumStake() {
    setState(() {
      _stakeAmount = minStake;
      _stakeController.text = minStake.toStringAsFixed(0);
    });
  }

  void _setMaximumStake() {
    final walletProvider = Provider.of<WalletProvider>(context, listen: false);
    final availableBalance = walletProvider.wallet?.unlockedBalanceXFG ?? 0.0;
    final maxAffordable = (availableBalance - 1.0).clamp(minStake, maxStake); // Reserve 1 XFG for fees
    
    setState(() {
      _stakeAmount = maxAffordable;
      _stakeController.text = maxAffordable.toStringAsFixed(0);
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Register Elderfier Node'),
        elevation: 0,
        backgroundColor: Colors.transparent,
      ),
      body: Consumer<WalletProvider>(
        builder: (context, walletProvider, child) {
          final availableBalance = walletProvider.wallet?.unlockedBalanceXFG ?? 0.0;
          final canAffordMinStake = availableBalance >= minStake;

          return SingleChildScrollView(
            padding: const EdgeInsets.all(24),
            child: Form(
              key: _formKey,
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // Header
                  Container(
                    padding: const EdgeInsets.all(20),
                    decoration: BoxDecoration(
                      gradient: AppTheme.primaryGradient,
                      borderRadius: BorderRadius.circular(16),
                    ),
                    child: Row(
                      children: [
                        const Icon(
                          Icons.account_tree,
                          color: Colors.white,
                          size: 32,
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              const Text(
                                'Become an Elderfier',
                                style: TextStyle(
                                  color: Colors.white,
                                  fontSize: 20,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              Text(
                                'Earn rewards by securing the network',
                                style: TextStyle(
                                  color: Colors.white.withOpacity(0.9),
                                  fontSize: 14,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: 24),

                  // Balance check
                  Container(
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: canAffordMinStake 
                          ? AppTheme.successColor.withOpacity(0.1)
                          : AppTheme.errorColor.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(
                        color: canAffordMinStake 
                            ? AppTheme.successColor.withOpacity(0.3)
                            : AppTheme.errorColor.withOpacity(0.3),
                      ),
                    ),
                    child: Row(
                      children: [
                        Icon(
                          canAffordMinStake ? Icons.check_circle : Icons.warning,
                          color: canAffordMinStake 
                              ? AppTheme.successColor 
                              : AppTheme.errorColor,
                        ),
                        const SizedBox(width: 12),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                'Available Balance',
                                style: TextStyle(
                                  fontSize: 14,
                                  color: AppTheme.textSecondary,
                                ),
                              ),
                              Text(
                                '${availableBalance.toStringAsFixed(2)} XFG',
                                style: const TextStyle(
                                  fontSize: 18,
                                  fontWeight: FontWeight.bold,
                                  color: AppTheme.textPrimary,
                                ),
                              ),
                              if (!canAffordMinStake)
                                Text(
                                  'Minimum ${minStake.toStringAsFixed(0)} XFG required',
                                  style: const TextStyle(
                                    fontSize: 12,
                                    color: AppTheme.errorColor,
                                  ),
                                ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: 24),

                  // Node name
                  const Text(
                    'Node Name',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: AppTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _nameController,
                    decoration: const InputDecoration(
                      hintText: 'Enter a unique name (8 characters max)',
                      prefixIcon: Icon(Icons.badge),
                    ),
                    maxLength: 8,
                    inputFormatters: [
                      FilteringTextInputFormatter.allow(RegExp(r'[A-Z0-9]')),
                    ],
                    textCapitalization: TextCapitalization.characters,
                    validator: (value) {
                      if (value == null || value.trim().isEmpty) {
                        return 'Please enter a node name';
                      }
                      if (value.trim().length < 3) {
                        return 'Name must be at least 3 characters';
                      }
                      return null;
                    },
                  ),
                  const SizedBox(height: 24),

                  // Stake amount
                  const Text(
                    'Stake Amount',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: AppTheme.textPrimary,
                    ),
                  ),
                  const SizedBox(height: 8),
                  TextFormField(
                    controller: _stakeController,
                    keyboardType: TextInputType.number,
                    decoration: InputDecoration(
                      hintText: 'Minimum ${minStake.toStringAsFixed(0)} XFG',
                      suffixText: 'XFG',
                      prefixIcon: const Icon(Icons.lock),
                    ),
                    onChanged: _onStakeChanged,
                    inputFormatters: [
                      FilteringTextInputFormatter.allow(RegExp(r'^\d*\.?\d{0,2}')),
                    ],
                    validator: (value) {
                      if (value == null || value.trim().isEmpty) {
                        return 'Please enter stake amount';
                      }
                      final amount = double.tryParse(value);
                      if (amount == null) {
                        return 'Please enter a valid amount';
                      }
                      if (amount < minStake) {
                        return 'Minimum stake is ${minStake.toStringAsFixed(0)} XFG';
                      }
                      if (amount > availableBalance) {
                        return 'Insufficient balance';
                      }
                      return null;
                    },
                  ),
                  const SizedBox(height: 12),

                  // Stake amount buttons
                  Row(
                    children: [
                      Expanded(
                        child: OutlinedButton(
                          onPressed: _setMinimumStake,
                          child: Text('Min (${minStake.toStringAsFixed(0)})'),
                        ),
                      ),
                      const SizedBox(width: 12),
                      Expanded(
                        child: OutlinedButton(
                          onPressed: canAffordMinStake ? _setMaximumStake : null,
                          child: const Text('Max Available'),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),

                  // Requirements
                  Container(
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
                        const Row(
                          children: [
                            Icon(
                              Icons.info_outline,
                              color: AppTheme.primaryColor,
                            ),
                            SizedBox(width: 8),
                            Text(
                              'Elderfier Requirements',
                              style: TextStyle(
                                fontSize: 16,
                                fontWeight: FontWeight.bold,
                                color: AppTheme.primaryColor,
                              ),
                            ),
                          ],
                        ),
                        const SizedBox(height: 12),
                        _buildRequirement(
                          'Minimum ${minStake.toStringAsFixed(0)} XFG stake',
                          _stakeAmount >= minStake,
                        ),
                        _buildRequirement(
                          'Unique node name (3-8 characters)',
                          _nameController.text.trim().length >= 3,
                        ),
                        _buildRequirement(
                          'Sufficient balance for fees',
                          availableBalance > _stakeAmount,
                        ),
                        _buildRequirement(
                          'Network connectivity',
                          walletProvider.isConnected,
                        ),
                      ],
                    ),
                  ),
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

                  // Register button
                  SizedBox(
                    width: double.infinity,
                    height: 56,
                    child: ElevatedButton(
                      onPressed: _isLoading || !canAffordMinStake 
                          ? null 
                          : _registerElderfier,
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
                              'Register Elderfier Node',
                              style: TextStyle(
                                fontSize: 18,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                    ),
                  ),
                  const SizedBox(height: 16),

                  // Warning
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
                          Icons.warning_amber,
                          color: AppTheme.warningColor,
                          size: 20,
                        ),
                        const SizedBox(width: 12),
                        Expanded(
                          child: Text(
                            'Your staked XFG will be locked for the duration of your Elderfier participation. '
                            'Make sure you can maintain network connectivity for optimal rewards.',
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

  Widget _buildRequirement(String text, bool isMet) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Row(
        children: [
          Icon(
            isMet ? Icons.check_circle : Icons.radio_button_unchecked,
            color: isMet ? AppTheme.successColor : AppTheme.textMuted,
            size: 16,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              text,
              style: TextStyle(
                fontSize: 14,
                color: isMet ? AppTheme.textPrimary : AppTheme.textSecondary,
              ),
            ),
          ),
        ],
      ),
    );
  }
}