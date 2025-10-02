import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../models/wallet.dart';
import '../../utils/theme.dart';
import 'register_elderfier_screen.dart';

class ElderfierScreen extends StatefulWidget {
  const ElderfierScreen({super.key});

  @override
  State<ElderfierScreen> createState() => _ElderfierScreenState();
}

class _ElderfierScreenState extends State<ElderfierScreen>
    with TickerProviderStateMixin {
  late TabController _tabController;
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    _loadElderfierData();
  }

  Future<void> _loadElderfierData() async {
    setState(() {
      _isLoading = true;
    });

    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      await walletProvider.refreshElderfierNodes();
    } finally {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }

  void _navigateToRegister() {
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (context) => const RegisterElderfierScreen(),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Elderfier Nodes'),
        elevation: 0,
        backgroundColor: Colors.transparent,
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: AppTheme.primaryColor,
          labelColor: AppTheme.primaryColor,
          unselectedLabelColor: AppTheme.textSecondary,
          tabs: const [
            Tab(text: 'My Nodes'),
            Tab(text: 'Network'),
          ],
        ),
        actions: [
          IconButton(
            onPressed: _loadElderfierData,
            icon: Icon(
              Icons.refresh,
              color: _isLoading ? AppTheme.primaryColor : AppTheme.textSecondary,
            ),
          ),
        ],
      ),
      body: TabBarView(
        controller: _tabController,
        children: [
          _buildMyNodesTab(),
          _buildNetworkTab(),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _navigateToRegister,
        backgroundColor: AppTheme.primaryColor,
        foregroundColor: Colors.white,
        icon: const Icon(Icons.add),
        label: const Text('Register Node'),
      ),
    );
  }

  Widget _buildMyNodesTab() {
    return Consumer<WalletProvider>(
      builder: (context, walletProvider, child) {
        final myNodes = walletProvider.elderfierNodes
            .where((node) => node.address == walletProvider.wallet?.address)
            .toList();

        if (_isLoading) {
          return const Center(
            child: CircularProgressIndicator(
              valueColor: AlwaysStoppedAnimation<Color>(AppTheme.primaryColor),
            ),
          );
        }

        if (myNodes.isEmpty) {
          return _buildEmptyMyNodes();
        }

        return RefreshIndicator(
          onRefresh: _loadElderfierData,
          color: AppTheme.primaryColor,
          child: ListView.builder(
            padding: const EdgeInsets.all(16),
            itemCount: myNodes.length,
            itemBuilder: (context, index) {
              return _buildMyNodeCard(myNodes[index]);
            },
          ),
        );
      },
    );
  }

  Widget _buildNetworkTab() {
    return Consumer<WalletProvider>(
      builder: (context, walletProvider, child) {
        final allNodes = walletProvider.elderfierNodes;

        if (_isLoading) {
          return const Center(
            child: CircularProgressIndicator(
              valueColor: AlwaysStoppedAnimation<Color>(AppTheme.primaryColor),
            ),
          );
        }

        if (allNodes.isEmpty) {
          return _buildEmptyNetwork();
        }

        return RefreshIndicator(
          onRefresh: _loadElderfierData,
          color: AppTheme.primaryColor,
          child: Column(
            children: [
              // Network stats
              Container(
                margin: const EdgeInsets.all(16),
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: AppTheme.cardColor,
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(
                    color: AppTheme.primaryColor.withOpacity(0.3),
                  ),
                ),
                child: Row(
                  children: [
                    Expanded(
                      child: _buildNetworkStat(
                        'Total Nodes',
                        allNodes.length.toString(),
                        Icons.account_tree,
                      ),
                    ),
                    Expanded(
                      child: _buildNetworkStat(
                        'Active Nodes',
                        allNodes.where((n) => n.isActive).length.toString(),
                        Icons.check_circle,
                      ),
                    ),
                    Expanded(
                      child: _buildNetworkStat(
                        'Total Stake',
                        '${_calculateTotalStake(allNodes).toStringAsFixed(0)} XFG',
                        Icons.lock,
                      ),
                    ),
                  ],
                ),
              ),
              // Nodes list
              Expanded(
                child: ListView.builder(
                  padding: const EdgeInsets.symmetric(horizontal: 16),
                  itemCount: allNodes.length,
                  itemBuilder: (context, index) {
                    return _buildNetworkNodeCard(allNodes[index]);
                  },
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildMyNodeCard(ElderfierNode node) {
    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: AppTheme.cardColor,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: node.isActive ? AppTheme.successColor : AppTheme.errorColor,
          width: 1,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  color: node.isActive 
                      ? AppTheme.successColor.withOpacity(0.1)
                      : AppTheme.errorColor.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Icon(
                  node.isActive ? Icons.check_circle : Icons.error,
                  color: node.isActive ? AppTheme.successColor : AppTheme.errorColor,
                  size: 20,
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      node.customName.isNotEmpty ? node.customName : 'Unnamed Node',
                      style: const TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                        color: AppTheme.textPrimary,
                      ),
                    ),
                    Text(
                      'ID: ${_truncateNodeId(node.nodeId)}',
                      style: TextStyle(
                        fontSize: 12,
                        color: AppTheme.textMuted,
                        fontFamily: 'monospace',
                      ),
                    ),
                  ],
                ),
              ),
              Text(
                node.isActive ? 'ACTIVE' : 'INACTIVE',
                style: TextStyle(
                  fontSize: 12,
                  fontWeight: FontWeight.bold,
                  color: node.isActive ? AppTheme.successColor : AppTheme.errorColor,
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          
          // Node details
          Row(
            children: [
              Expanded(
                child: _buildNodeDetail(
                  'Stake',
                  '${node.stakeAmountXFG.toStringAsFixed(0)} XFG',
                  Icons.lock,
                ),
              ),
              Expanded(
                child: _buildNodeDetail(
                  'Uptime',
                  '${(node.uptime / 3600).toStringAsFixed(1)}h',
                  Icons.timer,
                ),
              ),
              Expanded(
                child: _buildNodeDetail(
                  'Consensus',
                  node.consensusType,
                  Icons.how_to_vote,
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          
          // Last seen
          Row(
            children: [
              Icon(
                Icons.update,
                size: 16,
                color: AppTheme.textMuted,
              ),
              const SizedBox(width: 8),
              Text(
                'Last seen: Block ${node.lastSeenBlock}',
                style: TextStyle(
                  fontSize: 12,
                  color: AppTheme.textMuted,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildNetworkNodeCard(ElderfierNode node) {
    return Container(
      margin: const EdgeInsets.only(bottom: 8),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: AppTheme.surfaceColor,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        children: [
          Container(
            width: 8,
            height: 8,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: node.isActive ? AppTheme.successColor : AppTheme.errorColor,
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  node.customName.isNotEmpty ? node.customName : 'Node ${_truncateNodeId(node.nodeId)}',
                  style: const TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.w600,
                    color: AppTheme.textPrimary,
                  ),
                ),
                Text(
                  '${node.stakeAmountXFG.toStringAsFixed(0)} XFG • ${node.consensusType}',
                  style: TextStyle(
                    fontSize: 12,
                    color: AppTheme.textSecondary,
                  ),
                ),
              ],
            ),
          ),
          Text(
            '${(node.uptime / 3600).toStringAsFixed(0)}h',
            style: TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.w500,
              color: AppTheme.textSecondary,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildEmptyMyNodes() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.account_tree_outlined,
              size: 64,
              color: AppTheme.textMuted,
            ),
            const SizedBox(height: 16),
            const Text(
              'No Elderfier Nodes',
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: AppTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Register your first Elderfier node to start earning rewards and participating in network consensus.',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 16,
                color: AppTheme.textSecondary,
                height: 1.5,
              ),
            ),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: _navigateToRegister,
              icon: const Icon(Icons.add),
              label: const Text('Register Elderfier Node'),
            ),
            const SizedBox(height: 16),
            
            // Info card
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
                        'About Elderfier Nodes',
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
                    '• Requires 800 XFG minimum stake\n'
                    '• Earn rewards for network participation\n'
                    '• Help secure the Fuego network\n'
                    '• Participate in consensus decisions',
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

  Widget _buildEmptyNetwork() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.cloud_off,
              size: 64,
              color: AppTheme.textMuted,
            ),
            const SizedBox(height: 16),
            const Text(
              'No Network Data',
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: AppTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Unable to load Elderfier network information. Check your connection and try again.',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 16,
                color: AppTheme.textSecondary,
                height: 1.5,
              ),
            ),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: _loadElderfierData,
              icon: const Icon(Icons.refresh),
              label: const Text('Retry'),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildNetworkStat(String label, String value, IconData icon) {
    return Column(
      children: [
        Icon(
          icon,
          color: AppTheme.primaryColor,
          size: 24,
        ),
        const SizedBox(height: 8),
        Text(
          value,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
            color: AppTheme.textPrimary,
          ),
        ),
        Text(
          label,
          style: TextStyle(
            fontSize: 12,
            color: AppTheme.textSecondary,
          ),
        ),
      ],
    );
  }

  Widget _buildNodeDetail(String label, String value, IconData icon) {
    return Column(
      children: [
        Icon(
          icon,
          size: 16,
          color: AppTheme.primaryColor,
        ),
        const SizedBox(height: 4),
        Text(
          value,
          style: const TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.bold,
            color: AppTheme.textPrimary,
          ),
        ),
        Text(
          label,
          style: TextStyle(
            fontSize: 10,
            color: AppTheme.textSecondary,
          ),
        ),
      ],
    );
  }

  double _calculateTotalStake(List<ElderfierNode> nodes) {
    return nodes.fold(0.0, (sum, node) => sum + node.stakeAmountXFG);
  }

  String _truncateNodeId(String nodeId) {
    if (nodeId.length <= 12) return nodeId;
    return '${nodeId.substring(0, 6)}...${nodeId.substring(nodeId.length - 6)}';
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }
}