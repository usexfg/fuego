import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../providers/wallet_provider.dart';
import '../../utils/theme.dart';
import 'send_message_screen.dart';

class MessagingScreen extends StatefulWidget {
  const MessagingScreen({super.key});

  @override
  State<MessagingScreen> createState() => _MessagingScreenState();
}

class _MessagingScreenState extends State<MessagingScreen>
    with TickerProviderStateMixin {
  late TabController _tabController;
  List<Map<String, dynamic>> _messages = [];
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    _loadMessages();
  }

  Future<void> _loadMessages() async {
    setState(() {
      _isLoading = true;
    });

    try {
      final walletProvider = Provider.of<WalletProvider>(context, listen: false);
      final messages = await walletProvider.loadMessages();
      
      setState(() {
        _messages = messages;
      });
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Failed to load messages: $e'),
            backgroundColor: AppTheme.errorColor,
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }

  void _navigateToSendMessage() {
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (context) => const SendMessageScreen(),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Encrypted Messages'),
        elevation: 0,
        backgroundColor: Colors.transparent,
        bottom: TabBar(
          controller: _tabController,
          indicatorColor: AppTheme.primaryColor,
          labelColor: AppTheme.primaryColor,
          unselectedLabelColor: AppTheme.textSecondary,
          tabs: const [
            Tab(text: 'Inbox'),
            Tab(text: 'Sent'),
          ],
        ),
        actions: [
          IconButton(
            onPressed: _loadMessages,
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
          _buildInboxTab(),
          _buildSentTab(),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _navigateToSendMessage,
        backgroundColor: AppTheme.primaryColor,
        foregroundColor: Colors.white,
        icon: const Icon(Icons.send),
        label: const Text('New Message'),
      ),
    );
  }

  Widget _buildInboxTab() {
    final inboxMessages = _messages
        .where((msg) => msg['type'] == 'received')
        .toList();

    if (_isLoading) {
      return const Center(
        child: CircularProgressIndicator(
          valueColor: AlwaysStoppedAnimation<Color>(AppTheme.primaryColor),
        ),
      );
    }

    if (inboxMessages.isEmpty) {
      return _buildEmptyInbox();
    }

    return RefreshIndicator(
      onRefresh: _loadMessages,
      color: AppTheme.primaryColor,
      child: ListView.builder(
        padding: const EdgeInsets.all(16),
        itemCount: inboxMessages.length,
        itemBuilder: (context, index) {
          return _buildMessageCard(inboxMessages[index], isReceived: true);
        },
      ),
    );
  }

  Widget _buildSentTab() {
    final sentMessages = _messages
        .where((msg) => msg['type'] == 'sent')
        .toList();

    if (_isLoading) {
      return const Center(
        child: CircularProgressIndicator(
          valueColor: AlwaysStoppedAnimation<Color>(AppTheme.primaryColor),
        ),
      );
    }

    if (sentMessages.isEmpty) {
      return _buildEmptySent();
    }

    return RefreshIndicator(
      onRefresh: _loadMessages,
      color: AppTheme.primaryColor,
      child: ListView.builder(
        padding: const EdgeInsets.all(16),
        itemCount: sentMessages.length,
        itemBuilder: (context, index) {
          return _buildMessageCard(sentMessages[index], isReceived: false);
        },
      ),
    );
  }

  Widget _buildMessageCard(Map<String, dynamic> message, {required bool isReceived}) {
    final timestamp = DateTime.fromMillisecondsSinceEpoch(
      (message['timestamp'] as int) * 1000,
    );
    final isUnread = message['unread'] as bool? ?? false;
    final hasAttachment = message['attachment'] as bool? ?? false;
    final selfDestruct = message['self_destruct'] as bool? ?? false;

    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      decoration: BoxDecoration(
        color: isUnread ? AppTheme.primaryColor.withOpacity(0.05) : AppTheme.cardColor,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: isUnread 
              ? AppTheme.primaryColor.withOpacity(0.3)
              : AppTheme.textMuted.withOpacity(0.1),
        ),
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          borderRadius: BorderRadius.circular(12),
          onTap: () => _openMessage(message),
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Container(
                      width: 40,
                      height: 40,
                      decoration: BoxDecoration(
                        color: isReceived 
                            ? AppTheme.successColor.withOpacity(0.1)
                            : AppTheme.primaryColor.withOpacity(0.1),
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: Icon(
                        isReceived ? Icons.arrow_downward : Icons.arrow_upward,
                        color: isReceived ? AppTheme.successColor : AppTheme.primaryColor,
                        size: 20,
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            children: [
                              Expanded(
                                child: Text(
                                  isReceived ? 'From:' : 'To:',
                                  style: TextStyle(
                                    fontSize: 12,
                                    color: AppTheme.textSecondary,
                                  ),
                                ),
                              ),
                              if (isUnread)
                                Container(
                                  width: 8,
                                  height: 8,
                                  decoration: const BoxDecoration(
                                    color: AppTheme.primaryColor,
                                    shape: BoxShape.circle,
                                  ),
                                ),
                            ],
                          ),
                          Text(
                            _truncateAddress(message['address'] as String),
                            style: const TextStyle(
                              fontSize: 14,
                              fontWeight: FontWeight.w600,
                              color: AppTheme.textPrimary,
                              fontFamily: 'monospace',
                            ),
                          ),
                        ],
                      ),
                    ),
                    Text(
                      _formatMessageTime(timestamp),
                      style: TextStyle(
                        fontSize: 12,
                        color: AppTheme.textSecondary,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                
                // Message preview
                Text(
                  message['preview'] as String? ?? 'Encrypted message',
                  style: TextStyle(
                    fontSize: 14,
                    color: AppTheme.textSecondary,
                  ),
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: 8),
                
                // Message flags
                Row(
                  children: [
                    if (hasAttachment) ...[
                      Icon(
                        Icons.attach_file,
                        size: 16,
                        color: AppTheme.textMuted,
                      ),
                      const SizedBox(width: 4),
                    ],
                    if (selfDestruct) ...[
                      Icon(
                        Icons.timer,
                        size: 16,
                        color: AppTheme.warningColor,
                      ),
                      const SizedBox(width: 4),
                      Text(
                        'Self-destruct',
                        style: TextStyle(
                          fontSize: 11,
                          color: AppTheme.warningColor,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                      const SizedBox(width: 8),
                    ],
                    Icon(
                      Icons.lock,
                      size: 16,
                      color: AppTheme.successColor,
                    ),
                    const SizedBox(width: 4),
                    Text(
                      'Encrypted',
                      style: TextStyle(
                        fontSize: 11,
                        color: AppTheme.successColor,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildEmptyInbox() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.inbox_outlined,
              size: 64,
              color: AppTheme.textMuted,
            ),
            const SizedBox(height: 16),
            const Text(
              'No Messages',
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: AppTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Your encrypted messages will appear here when received.',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 16,
                color: AppTheme.textSecondary,
                height: 1.5,
              ),
            ),
            const SizedBox(height: 24),
            
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
                        Icons.security,
                        color: AppTheme.primaryColor,
                      ),
                      SizedBox(width: 8),
                      Text(
                        'Secure Messaging',
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
                    '• End-to-end encrypted on blockchain\n'
                    '• Messages cannot be censored\n'
                    '• Optional self-destruct feature\n'
                    '• Complete privacy protection',
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

  Widget _buildEmptySent() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.send_outlined,
              size: 64,
              color: AppTheme.textMuted,
            ),
            const SizedBox(height: 16),
            const Text(
              'No Sent Messages',
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: AppTheme.textPrimary,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Messages you send will be stored here for your reference.',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: 16,
                color: AppTheme.textSecondary,
                height: 1.5,
              ),
            ),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: _navigateToSendMessage,
              icon: const Icon(Icons.send),
              label: const Text('Send Your First Message'),
            ),
          ],
        ),
      ),
    );
  }

  void _openMessage(Map<String, dynamic> message) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return Dialog(
          backgroundColor: AppTheme.cardColor,
          child: Container(
            constraints: const BoxConstraints(maxHeight: 600),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                // Header
                Container(
                  padding: const EdgeInsets.all(16),
                  decoration: const BoxDecoration(
                    border: Border(
                      bottom: BorderSide(
                        color: AppTheme.textMuted,
                        width: 0.5,
                      ),
                    ),
                  ),
                  child: Row(
                    children: [
                      Icon(
                        Icons.message,
                        color: AppTheme.primaryColor,
                      ),
                      const SizedBox(width: 8),
                      const Text(
                        'Encrypted Message',
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: AppTheme.textPrimary,
                        ),
                      ),
                      const Spacer(),
                      IconButton(
                        onPressed: () => Navigator.of(context).pop(),
                        icon: const Icon(Icons.close),
                      ),
                    ],
                  ),
                ),
                
                // Content
                Expanded(
                  child: Padding(
                    padding: const EdgeInsets.all(16),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        // From/To
                        Text(
                          message['type'] == 'received' ? 'From:' : 'To:',
                          style: TextStyle(
                            fontSize: 12,
                            color: AppTheme.textSecondary,
                          ),
                        ),
                        SelectableText(
                          message['address'] as String,
                          style: const TextStyle(
                            fontSize: 14,
                            fontWeight: FontWeight.w600,
                            color: AppTheme.textPrimary,
                            fontFamily: 'monospace',
                          ),
                        ),
                        const SizedBox(height: 16),
                        
                        // Timestamp
                        Text(
                          'Received: ${_formatFullDateTime(DateTime.fromMillisecondsSinceEpoch((message['timestamp'] as int) * 1000))}',
                          style: TextStyle(
                            fontSize: 12,
                            color: AppTheme.textSecondary,
                          ),
                        ),
                        const SizedBox(height: 16),
                        
                        // Message content
                        Expanded(
                          child: Container(
                            width: double.infinity,
                            padding: const EdgeInsets.all(16),
                            decoration: BoxDecoration(
                              color: AppTheme.surfaceColor,
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: SelectableText(
                              message['content'] as String? ?? 'Message content encrypted',
                              style: const TextStyle(
                                fontSize: 16,
                                color: AppTheme.textPrimary,
                                height: 1.5,
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                
                // Actions
                Container(
                  padding: const EdgeInsets.all(16),
                  decoration: const BoxDecoration(
                    border: Border(
                      top: BorderSide(
                        color: AppTheme.textMuted,
                        width: 0.5,
                      ),
                    ),
                  ),
                  child: Row(
                    children: [
                      if (message['type'] == 'received')
                        Expanded(
                          child: OutlinedButton.icon(
                            onPressed: () {
                              Navigator.of(context).pop();
                              _replyToMessage(message);
                            },
                            icon: const Icon(Icons.reply),
                            label: const Text('Reply'),
                          ),
                        ),
                      if (message['type'] == 'received') const SizedBox(width: 12),
                      Expanded(
                        child: ElevatedButton(
                          onPressed: () => Navigator.of(context).pop(),
                          child: const Text('Close'),
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
    );
  }

  void _replyToMessage(Map<String, dynamic> message) {
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (context) => SendMessageScreen(
          recipientAddress: message['address'] as String,
        ),
      ),
    );
  }

  String _truncateAddress(String address) {
    if (address.length <= 20) return address;
    return '${address.substring(0, 10)}...${address.substring(address.length - 10)}';
  }

  String _formatMessageTime(DateTime dateTime) {
    final now = DateTime.now();
    final difference = now.difference(dateTime);
    
    if (difference.inDays > 0) {
      return '${difference.inDays}d ago';
    } else if (difference.inHours > 0) {
      return '${difference.inHours}h ago';
    } else if (difference.inMinutes > 0) {
      return '${difference.inMinutes}m ago';
    } else {
      return 'Just now';
    }
  }

  String _formatFullDateTime(DateTime dateTime) {
    return '${dateTime.day}/${dateTime.month}/${dateTime.year} at ${dateTime.hour.toString().padLeft(2, '0')}:${dateTime.minute.toString().padLeft(2, '0')}';
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }
}