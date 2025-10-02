import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../utils/theme.dart';
import '../../widgets/balance_card.dart';
import '../../widgets/quick_actions.dart';
import '../../widgets/recent_transactions.dart';
import '../transactions/send_screen.dart';
import '../transactions/receive_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> with TickerProviderStateMixin {
  late AnimationController _refreshController;
  late Animation<double> _refreshAnimation;
  bool _isRefreshing = false;

  @override
  void initState() {
    super.initState();
    _setupAnimations();
    _initializeWallet();
  }

  void _setupAnimations() {
    _refreshController = AnimationController(
      duration: const Duration(milliseconds: 1000),
      vsync: this,
    );

    _refreshAnimation = Tween<double>(
      begin: 0,
      end: 1,
    ).animate(CurvedAnimation(
      parent: _refreshController,
      curve: Curves.easeInOut,
    ));
  }

  Future<void> _initializeWallet() async {
    final walletProvider = Provider.of<WalletProvider>(context, listen: false);
    await walletProvider.refreshWallet();
    await walletProvider.refreshTransactions();
  }

  Future<void> _onRefresh() async {
    if (_isRefreshing) return;
    
    setState(() {
      _isRefreshing = true;
    });
    
    _refreshController.forward();
    
    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      await Future.wait([
        walletProvider.refreshWallet(),
        walletProvider.refreshTransactions(),
      ]);
    } finally {
      _refreshController.reset();
      setState(() {
        _isRefreshing = false;
      });
    }
  }

  void _navigateToSend() {
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (context) => const SendScreen(),
      ),
    );
  }

  void _navigateToReceive() {
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (context) => const ReceiveScreen(),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: AppTheme.backgroundGradient,
        ),
        child: SafeArea(
          child: RefreshIndicator(
            onRefresh: _onRefresh,
            color: AppTheme.primaryColor,
            backgroundColor: AppTheme.cardColor,
            child: CustomScrollView(
              slivers: [
                // App Bar
                SliverAppBar(
                  expandedHeight: 120,
                  floating: true,
                  pinned: true,
                  backgroundColor: Colors.transparent,
                  elevation: 0,
                  flexibleSpace: FlexibleSpaceBar(
                    titlePadding: const EdgeInsets.only(left: 20, bottom: 16),
                    title: Column(
                      mainAxisAlignment: MainAxisAlignment.end,
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          'Firefly Wallet',
                          style: TextStyle(
                            color: AppTheme.textPrimary,
                            fontSize: 24,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        Consumer<WalletProvider>(
                          builder: (context, wallet, child) {
                            return Text(
                              wallet.isConnected 
                                  ? (wallet.isWalletSynced 
                                      ? 'Synchronized' 
                                      : 'Syncing...')
                                  : 'Offline',
                              style: TextStyle(
                                color: wallet.isConnected 
                                    ? (wallet.isWalletSynced 
                                        ? AppTheme.successColor 
                                        : AppTheme.warningColor)
                                    : AppTheme.errorColor,
                                fontSize: 12,
                                fontWeight: FontWeight.w500,
                              ),
                            );
                          },
                        ),
                      ],
                    ),
                  ),
                  actions: [
                    Consumer<WalletProvider>(
                      builder: (context, wallet, child) {
                        return AnimatedBuilder(
                          animation: _refreshAnimation,
                          builder: (context, child) {
                            return Transform.rotate(
                              angle: _refreshAnimation.value * 2 * 3.14159,
                              child: IconButton(
                                icon: Icon(
                                  Icons.refresh,
                                  color: _isRefreshing 
                                      ? AppTheme.primaryColor 
                                      : AppTheme.textSecondary,
                                ),
                                onPressed: _isRefreshing ? null : _onRefresh,
                              ),
                            );
                          },
                        );
                      },
                    ),
                  ],
                ),
                
                // Content
                SliverPadding(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                  sliver: SliverList(
                    delegate: SliverChildListDelegate([
                      // Balance Card
                      const BalanceCard(),
                      const SizedBox(height: 20),
                      
                      // Quick Actions
                      QuickActions(
                        onSendTap: _navigateToSend,
                        onReceiveTap: _navigateToReceive,
                        onScanTap: _onScanQR,
                        onMineTap: _onToggleMining,
                      ),
                      const SizedBox(height: 20),
                      
                      // Sync Progress (if syncing)
                      Consumer<WalletProvider>(
                        builder: (context, wallet, child) {
                          if (!wallet.isSyncing || wallet.isWalletSynced) {
                            return const SizedBox.shrink();
                          }
                          
                          return Container(
                            margin: const EdgeInsets.only(bottom: 20),
                            padding: const EdgeInsets.all(16),
                            decoration: BoxDecoration(
                              color: AppTheme.cardColor,
                              borderRadius: BorderRadius.circular(12),
                            ),
                            child: Column(
                              children: [
                                Row(
                                  children: [
                                    const Icon(
                                      Icons.sync,
                                      color: AppTheme.primaryColor,
                                      size: 20,
                                    ),
                                    const SizedBox(width: 8),
                                    Text(
                                      'Synchronizing Blockchain',
                                      style: TextStyle(
                                        color: AppTheme.textPrimary,
                                        fontWeight: FontWeight.w600,
                                      ),
                                    ),
                                    const Spacer(),
                                    Text(
                                      '${(wallet.syncProgress * 100).toStringAsFixed(1)}%',
                                      style: TextStyle(
                                        color: AppTheme.textSecondary,
                                        fontSize: 12,
                                      ),
                                    ),
                                  ],
                                ),
                                const SizedBox(height: 12),
                                ClipRRect(
                                  borderRadius: BorderRadius.circular(4),
                                  child: LinearProgressIndicator(
                                    value: wallet.syncProgress,
                                    backgroundColor: AppTheme.surfaceColor,
                                    valueColor: const AlwaysStoppedAnimation<Color>(
                                      AppTheme.primaryColor,
                                    ),
                                  ),
                                ),
                                const SizedBox(height: 8),
                                Text(
                                  '${wallet.wallet?.localHeight ?? 0} / ${wallet.wallet?.blockchainHeight ?? 0} blocks',
                                  style: TextStyle(
                                    color: AppTheme.textMuted,
                                    fontSize: 11,
                                  ),
                                ),
                              ],
                            ),
                          );
                        },
                      ),
                      
                      // Recent Transactions
                      const RecentTransactions(),
                      const SizedBox(height: 20),
                      
                      // Mining Status (if applicable)
                      Consumer<WalletProvider>(
                        builder: (context, wallet, child) {
                          if (!wallet.isMining) return const SizedBox.shrink();
                          
                          return Container(
                            padding: const EdgeInsets.all(16),
                            decoration: BoxDecoration(
                              color: AppTheme.successColor.withOpacity(0.1),
                              borderRadius: BorderRadius.circular(12),
                              border: Border.all(
                                color: AppTheme.successColor.withOpacity(0.3),
                              ),
                            ),
                            child: Row(
                              children: [
                                const Icon(
                                  Icons.memory,
                                  color: AppTheme.successColor,
                                ),
                                const SizedBox(width: 12),
                                Expanded(
                                  child: Column(
                                    crossAxisAlignment: CrossAxisAlignment.start,
                                    children: [
                                      const Text(
                                        'Mining Active',
                                        style: TextStyle(
                                          color: AppTheme.successColor,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                      Text(
                                        '${wallet.miningSpeed} H/s with ${wallet.miningThreads} threads',
                                        style: TextStyle(
                                          color: AppTheme.textSecondary,
                                          fontSize: 12,
                                        ),
                                      ),
                                    ],
                                  ),
                                ),
                                TextButton(
                                  onPressed: () => wallet.stopMining(),
                                  child: const Text('Stop'),
                                ),
                              ],
                            ),
                          );
                        },
                      ),
                    ]),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  void _onScanQR() {
    // TODO: Implement QR code scanning
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('QR code scanning coming soon'),
        backgroundColor: AppTheme.primaryColor,
      ),
    );
  }

  void _onToggleMining() {
    final walletProvider = Provider.of<WalletProvider>(context, listen: false);
    
    if (walletProvider.isMining) {
      walletProvider.stopMining();
    } else {
      _showMiningDialog();
    }
  }

  void _showMiningDialog() {
    int threads = 1;
    
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (context, setState) {
            return AlertDialog(
              backgroundColor: AppTheme.cardColor,
              title: const Text(
                'Start Mining',
                style: TextStyle(color: AppTheme.textPrimary),
              ),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    'Choose the number of CPU threads to use for mining:',
                    style: TextStyle(color: AppTheme.textSecondary),
                  ),
                  const SizedBox(height: 16),
                  Row(
                    children: [
                      const Text(
                        'Threads:',
                        style: TextStyle(color: AppTheme.textPrimary),
                      ),
                      Expanded(
                        child: Slider(
                          value: threads.toDouble(),
                          min: 1,
                          max: 8,
                          divisions: 7,
                          label: threads.toString(),
                          activeColor: AppTheme.primaryColor,
                          onChanged: (value) {
                            setState(() {
                              threads = value.round();
                            });
                          },
                        ),
                      ),
                    ],
                  ),
                ],
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.of(context).pop(),
                  child: const Text(
                    'Cancel',
                    style: TextStyle(color: AppTheme.textSecondary),
                  ),
                ),
                ElevatedButton(
                  onPressed: () {
                    final walletProvider = Provider.of<WalletProvider>(
                      context,
                      listen: false,
                    );
                    walletProvider.startMining(threads: threads);
                    Navigator.of(context).pop();
                  },
                  child: const Text('Start Mining'),
                ),
              ],
            );
          },
        );
      },
    );
  }

  @override
  void dispose() {
    _refreshController.dispose();
    super.dispose();
  }
}

// TODO: Create these widget files
// - balance_card.dart
// - quick_actions.dart  
// - recent_transactions.dart
// - send_screen.dart
// - receive_screen.dart