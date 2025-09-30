import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:connectivity_plus/connectivity_plus.dart';
import '../models/wallet.dart';
import '../services/fuego_rpc_service.dart';
import '../services/security_service.dart';

class WalletProvider extends ChangeNotifier {
  final FuegoRPCService _rpcService;
  final SecurityService _securityService;
  
  Wallet? _wallet;
  List<WalletTransaction> _transactions = [];
  List<ElderfierNode> _elderfierNodes = [];
  bool _isLoading = false;
  bool _isConnected = false;
  bool _isSyncing = false;
  bool _isMining = false;
  String? _error;
  String? _nodeUrl;
  Timer? _syncTimer;

  // Mining status
  int _miningSpeed = 0;
  int _miningThreads = 1;

  // Network status
  ConnectivityResult _connectivityResult = ConnectivityResult.none;

  WalletProvider({
    FuegoRPCService? rpcService,
    SecurityService? securityService,
  }) : _rpcService = rpcService ?? FuegoRPCService(),
       _securityService = securityService ?? SecurityService() {
    _initConnectivity();
  }

  // Getters
  Wallet? get wallet => _wallet;
  List<WalletTransaction> get transactions => _transactions;
  List<ElderfierNode> get elderfierNodes => _elderfierNodes;
  bool get isLoading => _isLoading;
  bool get isConnected => _isConnected;
  bool get isSyncing => _isSyncing;
  bool get isMining => _isMining;
  String? get error => _error;
  String? get nodeUrl => _nodeUrl;
  int get miningSpeed => _miningSpeed;
  int get miningThreads => _miningThreads;
  ConnectivityResult get connectivityResult => _connectivityResult;

  bool get hasWallet => _wallet != null;
  bool get isWalletSynced => _wallet?.synced ?? false;
  double get syncProgress => _wallet?.syncProgress ?? 0.0;

  // Initialize connectivity monitoring
  void _initConnectivity() {
    Connectivity().onConnectivityChanged.listen((ConnectivityResult result) {
      _connectivityResult = result;
      notifyListeners();
      
      if (result != ConnectivityResult.none) {
        _checkConnection();
      } else {
        _isConnected = false;
        notifyListeners();
      }
    });
  }

  // Wallet Management
  Future<bool> createWallet({
    required String pin,
    String? mnemonic,
  }) async {
    _setLoading(true);
    _clearError();

    try {
      final seed = mnemonic ?? SecurityService.generateMnemonic();
      
      if (!SecurityService.validateMnemonic(seed)) {
        throw Exception('Invalid mnemonic phrase');
      }

      // Store mnemonic securely
      await _securityService.storeWalletSeed(seed, pin);
      await _securityService.setPIN(pin);

      // In a real implementation, we would derive keys from the mnemonic
      // For now, we'll simulate this process
      final viewKey = 'view_key_placeholder_${DateTime.now().millisecondsSinceEpoch}';
      final spendKey = 'spend_key_placeholder_${DateTime.now().millisecondsSinceEpoch}';
      
      await _securityService.storeWalletKeys(
        viewKey: viewKey,
        spendKey: spendKey,
        pin: pin,
      );

      _setLoading(false);
      return true;
    } catch (e) {
      _setError(e.toString());
      _setLoading(false);
      return false;
    }
  }

  Future<bool> restoreWallet({
    required String mnemonic,
    required String pin,
  }) async {
    _setLoading(true);
    _clearError();

    try {
      if (!SecurityService.validateMnemonic(mnemonic)) {
        throw Exception('Invalid mnemonic phrase');
      }

      // Store mnemonic and create PIN
      await _securityService.storeWalletSeed(mnemonic, pin);
      await _securityService.setPIN(pin);

      // Derive keys from mnemonic (placeholder implementation)
      final viewKey = 'restored_view_key_${DateTime.now().millisecondsSinceEpoch}';
      final spendKey = 'restored_spend_key_${DateTime.now().millisecondsSinceEpoch}';
      
      await _securityService.storeWalletKeys(
        viewKey: viewKey,
        spendKey: spendKey,
        pin: pin,
      );

      _setLoading(false);
      return true;
    } catch (e) {
      _setError(e.toString());
      _setLoading(false);
      return false;
    }
  }

  Future<bool> unlockWallet(String pin) async {
    _setLoading(true);
    _clearError();

    try {
      final isValidPin = await _securityService.verifyPIN(pin);
      if (!isValidPin) {
        throw Exception('Invalid PIN');
      }

      final keys = await _securityService.getWalletKeys(pin);
      if (keys == null) {
        throw Exception('Wallet keys not found');
      }

      // Initialize wallet with placeholder data
      // In real implementation, this would open wallet with the keys
      await refreshWallet();
      
      _setLoading(false);
      return true;
    } catch (e) {
      _setError(e.toString());
      _setLoading(false);
      return false;
    }
  }

  Future<void> lockWallet() async {
    _wallet = null;
    _transactions.clear();
    _stopSyncTimer();
    notifyListeners();
  }

  Future<bool> hasWalletData() async {
    return await _securityService.hasWalletData();
  }

  // Wallet Operations
  Future<void> refreshWallet() async {
    if (!_isConnected) {
      await _checkConnection();
      if (!_isConnected) {
        _setError('Not connected to Fuego node');
        return;
      }
    }

    try {
      _setLoading(true);
      _clearError();

      // Get wallet balance and info
      final balance = await _rpcService.getBalance();
      final address = await _rpcService.getAddress();
      
      _wallet = balance.copyWith(address: address);
      
      // Start sync timer if not already running
      if (!isWalletSynced) {
        _startSyncTimer();
      }

      notifyListeners();
    } catch (e) {
      _setError(e.toString());
    } finally {
      _setLoading(false);
    }
  }

  Future<void> refreshTransactions() async {
    try {
      final txs = await _rpcService.getTransactions();
      _transactions = txs;
      notifyListeners();
    } catch (e) {
      _setError('Failed to refresh transactions: $e');
    }
  }

  Future<String?> sendTransaction({
    required String address,
    required double amount,
    String? paymentId,
    int mixins = 7,
  }) async {
    _setLoading(true);
    _clearError();

    try {
      final atomicAmount = (amount * 100000000).round();
      final fee = 10000000; // Default fee in atomic units
      
      final request = SendTransactionRequest(
        address: address,
        amount: atomicAmount,
        paymentId: paymentId ?? '',
        fee: fee,
        mixins: mixins,
      );

      final txHash = await _rpcService.sendTransaction(request);
      
      // Refresh wallet after sending
      await refreshWallet();
      await refreshTransactions();
      
      _setLoading(false);
      return txHash;
    } catch (e) {
      _setError(e.toString());
      _setLoading(false);
      return null;
    }
  }

  Future<String> generatePaymentId() async {
    return await _rpcService.generatePaymentId();
  }

  Future<String> createIntegratedAddress(String paymentId) async {
    return await _rpcService.createIntegratedAddress(paymentId);
  }

  // Mining Operations
  Future<void> startMining({int threads = 1}) async {
    try {
      _miningThreads = threads;
      final success = await _rpcService.startMining(threads: threads);
      
      if (success) {
        _isMining = true;
        _startMiningStatusTimer();
      }
      
      notifyListeners();
    } catch (e) {
      _setError('Failed to start mining: $e');
    }
  }

  Future<void> stopMining() async {
    try {
      await _rpcService.stopMining();
      _isMining = false;
      _miningSpeed = 0;
      notifyListeners();
    } catch (e) {
      _setError('Failed to stop mining: $e');
    }
  }

  Future<void> refreshMiningStatus() async {
    try {
      final status = await _rpcService.getMiningStatus();
      _isMining = status['active'] as bool;
      _miningSpeed = status['speed'] as int;
      _miningThreads = status['threads'] as int;
      notifyListeners();
    } catch (e) {
      // Mining might not be supported, ignore errors
    }
  }

  // Elderfier Operations
  Future<void> refreshElderfierNodes() async {
    try {
      final nodes = await _rpcService.getElderfierNodes();
      _elderfierNodes = nodes;
      notifyListeners();
    } catch (e) {
      // Elderfier functionality might not be available
    }
  }

  Future<bool> registerElderfierNode({
    required String customName,
    required String address,
    required double stakeAmount,
  }) async {
    try {
      final atomicStake = (stakeAmount * 100000000).round();
      final success = await _rpcService.registerElderfierNode(
        customName: customName,
        address: address,
        stakeAmount: atomicStake,
      );
      
      if (success) {
        await refreshElderfierNodes();
      }
      
      return success;
    } catch (e) {
      _setError('Failed to register Elderfier node: $e');
      return false;
    }
  }

  // Connection Management
  Future<void> connectToNode(String url) async {
    _setLoading(true);
    _nodeUrl = url;
    
    await _checkConnection();
    
    if (_isConnected && hasWallet) {
      await refreshWallet();
    }
    
    _setLoading(false);
  }

  Future<void> _checkConnection() async {
    try {
      _isConnected = await _rpcService.testConnection();
    } catch (e) {
      _isConnected = false;
    }
    notifyListeners();
  }

  // Timer Management
  void _startSyncTimer() {
    if (_syncTimer?.isActive == true) return;
    
    _syncTimer = Timer.periodic(const Duration(seconds: 5), (timer) {
      if (_wallet != null && !isWalletSynced) {
        _refreshSyncStatus();
      } else {
        _stopSyncTimer();
      }
    });
  }

  void _stopSyncTimer() {
    _syncTimer?.cancel();
    _syncTimer = null;
    _isSyncing = false;
    notifyListeners();
  }

  void _startMiningStatusTimer() {
    Timer.periodic(const Duration(seconds: 3), (timer) {
      if (_isMining) {
        refreshMiningStatus();
      } else {
        timer.cancel();
      }
    });
  }

  Future<void> _refreshSyncStatus() async {
    try {
      _isSyncing = true;
      final info = await _rpcService.getInfo();
      final balance = await _rpcService.getBalance();
      
      if (_wallet != null) {
        _wallet = _wallet!.copyWith(
          blockchainHeight: info['height'] as int,
          localHeight: balance.localHeight,
          synced: (info['height'] - balance.localHeight) <= 1,
        );
      }
      
      _isSyncing = false;
      notifyListeners();
    } catch (e) {
      _isSyncing = false;
      notifyListeners();
    }
  }

  // Messaging Operations
  Future<bool> sendMessage({
    required String recipientAddress,
    required String message,
    bool selfDestruct = false,
    int? destructTime,
  }) async {
    try {
      final success = await _rpcService.sendMessage(
        recipientAddress: recipientAddress,
        message: message,
        selfDestruct: selfDestruct,
        destructTime: destructTime,
      );
      
      if (success) {
        // Message sent successfully
        return true;
      } else {
        _setError('Failed to send message');
        return false;
      }
    } catch (e) {
      _setError('Failed to send message: $e');
      return false;
    }
  }

  Future<List<Map<String, dynamic>>> loadMessages() async {
    try {
      final messages = await _rpcService.getMessages();
      
      // Transform messages for UI consumption
      return messages.map((msg) {
        return {
          'id': msg['id'] ?? '',
          'type': msg['type'] ?? 'received', // 'received' or 'sent'
          'address': msg['address'] ?? '',
          'content': msg['content'] ?? '',
          'preview': _generateMessagePreview(msg['content'] as String? ?? ''),
          'timestamp': msg['timestamp'] ?? DateTime.now().millisecondsSinceEpoch ~/ 1000,
          'unread': msg['unread'] ?? false,
          'self_destruct': msg['self_destruct'] ?? false,
          'attachment': msg['attachment'] ?? false,
        };
      }).toList();
    } catch (e) {
      _setError('Failed to load messages: $e');
      return [];
    }
  }

  String _generateMessagePreview(String content) {
    if (content.isEmpty) return 'Encrypted message';
    if (content.length <= 50) return content;
    return '${content.substring(0, 50)}...';
  }

  // Helper methods
  void _setLoading(bool loading) {
    _isLoading = loading;
    notifyListeners();
  }

  void _setError(String error) {
    _error = error;
    notifyListeners();
  }

  void _clearError() {
    _error = null;
    notifyListeners();
  }

  @override
  void dispose() {
    _syncTimer?.cancel();
    _rpcService.dispose();
    super.dispose();
  }
}