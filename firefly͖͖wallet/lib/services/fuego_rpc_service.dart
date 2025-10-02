import 'dart:convert';
import 'dart:io';
import 'package:dio/dio.dart';
import 'package:crypto/crypto.dart';
import '../models/wallet.dart';

class FuegoRPCService {
  final Dio _dio;
  final String _baseUrl;
  final String? _password;
  
  static const int defaultRpcPort = 28180;
  static const int defaultWalletPort = 8070;

  FuegoRPCService({
    String host = 'localhost',
    int port = defaultRpcPort,
    String? password,
  }) : _baseUrl = 'http://$host:$port',
       _password = password,
       _dio = Dio(BaseOptions(
         connectTimeout: const Duration(seconds: 30),
         receiveTimeout: const Duration(seconds: 30),
         headers: {'Content-Type': 'application/json'},
       ));

  // Daemon RPC Methods
  Future<Map<String, dynamic>> getInfo() async {
    return _makeRPCCall('getinfo', {});
  }

  Future<int> getHeight() async {
    final response = await _makeRPCCall('getheight', {});
    return response['height'] as int;
  }

  Future<Map<String, dynamic>> getBlockHash(int height) async {
    return _makeRPCCall('on_getblockhash', [height]);
  }

  Future<Map<String, dynamic>> getBlock(String hash) async {
    return _makeRPCCall('getblock', {'hash': hash});
  }

  // Wallet RPC Methods (requires wallet service running)
  Future<Wallet> getBalance() async {
    try {
      final response = await _makeWalletRPCCall('getBalance', {});
      final info = await getInfo();
      
      return Wallet(
        address: '', // Will be filled by getAddress
        viewKey: '',
        spendKey: '',
        balance: response['availableBalance'] as int,
        unlockedBalance: response['lockedAmount'] as int,
        blockchainHeight: info['height'] as int,
        localHeight: response['blockCount'] as int,
        synced: (info['height'] - response['blockCount']) <= 1,
      );
    } catch (e) {
      throw FuegoRPCException('Failed to get balance: $e');
    }
  }

  Future<String> getAddress() async {
    try {
      final response = await _makeWalletRPCCall('getAddresses', {});
      final addresses = response['addresses'] as List;
      return addresses.isNotEmpty ? addresses.first : '';
    } catch (e) {
      throw FuegoRPCException('Failed to get address: $e');
    }
  }

  Future<List<WalletTransaction>> getTransactions({
    int blockCount = 1000000,
    int firstBlockIndex = 0,
  }) async {
    try {
      final response = await _makeWalletRPCCall('getTransactions', {
        'blockCount': blockCount,
        'firstBlockIndex': firstBlockIndex,
      });

      final items = response['items'] as List;
      return items.map((item) {
        final transactions = item['transactions'] as List;
        return transactions.map((tx) => WalletTransaction(
          txid: tx['transactionHash'] as String,
          amount: tx['amount'] as int,
          fee: tx['fee'] as int,
          paymentId: tx['paymentId'] as String? ?? '',
          blockHeight: item['blockHash'] != null ? 
              (item['blockIndex'] as int) : 0,
          timestamp: tx['timestamp'] as int,
          isSpending: (tx['amount'] as int) < 0,
          address: tx['transfers']?.isNotEmpty == true ? 
              tx['transfers'][0]['address'] as String? : null,
          confirmations: tx['confirmations'] as int? ?? 0,
        )).toList();
      }).expand((x) => x).toList();
    } catch (e) {
      throw FuegoRPCException('Failed to get transactions: $e');
    }
  }

  Future<String> sendTransaction(SendTransactionRequest request) async {
    try {
      final response = await _makeWalletRPCCall('sendTransaction', {
        'destinations': [{
          'amount': request.amount,
          'address': request.address,
        }],
        'fee': request.fee,
        'anonymity': request.mixins,
        'paymentId': request.paymentId.isNotEmpty ? request.paymentId : null,
      });
      
      return response['transactionHash'] as String;
    } catch (e) {
      throw FuegoRPCException('Failed to send transaction: $e');
    }
  }

  Future<String> createIntegratedAddress(String paymentId) async {
    try {
      final address = await getAddress();
      // Simple integrated address format - in real implementation, this would
      // use proper CryptoNote integrated address encoding
      return '${address}_$paymentId';
    } catch (e) {
      throw FuegoRPCException('Failed to create integrated address: $e');
    }
  }

  Future<String> generatePaymentId() async {
    final bytes = List<int>.generate(32, (i) => 
        DateTime.now().millisecondsSinceEpoch + i);
    return sha256.convert(bytes).toString().substring(0, 64);
  }

  // Mining Methods
  Future<bool> startMining({
    String? address,
    int threads = 1,
  }) async {
    try {
      final minerAddress = address ?? await getAddress();
      await _makeRPCCall('start_mining', {
        'miner_address': minerAddress,
        'threads_count': threads,
      });
      return true;
    } catch (e) {
      throw FuegoRPCException('Failed to start mining: $e');
    }
  }

  Future<bool> stopMining() async {
    try {
      await _makeRPCCall('stop_mining', {});
      return true;
    } catch (e) {
      throw FuegoRPCException('Failed to stop mining: $e');
    }
  }

  Future<Map<String, dynamic>> getMiningStatus() async {
    try {
      final info = await getInfo();
      return {
        'active': info['mining_speed'] as int > 0,
        'speed': info['mining_speed'] as int,
        'threads': info['threads_count'] as int? ?? 0,
      };
    } catch (e) {
      throw FuegoRPCException('Failed to get mining status: $e');
    }
  }

  // Elderfier Methods (custom implementation for Fuego)
  Future<List<ElderfierNode>> getElderfierNodes() async {
    try {
      // This would be a custom RPC call specific to Fuego's Elderfier system
      final response = await _makeRPCCall('get_elderfier_nodes', {});
      final nodes = response['nodes'] as List;
      
      return nodes.map((node) => ElderfierNode(
        nodeId: node['node_id'] as String,
        customName: node['custom_name'] as String,
        address: node['address'] as String,
        stakeAmount: node['stake_amount'] as int,
        isActive: node['is_active'] as bool,
        uptime: node['uptime'] as int,
        lastSeenBlock: node['last_seen_block'] as int,
        consensusType: node['consensus_type'] as String,
      )).toList();
    } catch (e) {
      // Return empty list if Elderfier functionality is not available
      return [];
    }
  }

  Future<bool> registerElderfierNode({
    required String customName,
    required String address,
    required int stakeAmount,
  }) async {
    try {
      await _makeRPCCall('register_elderfier', {
        'custom_name': customName,
        'address': address,
        'stake_amount': stakeAmount,
      });
      return true;
    } catch (e) {
      throw FuegoRPCException('Failed to register Elderfier node: $e');
    }
  }

  // Message Methods (encrypted messaging)
  Future<bool> sendMessage({
    required String recipientAddress,
    required String message,
    bool selfDestruct = false,
    int? destructTime,
  }) async {
    try {
      await _makeRPCCall('send_message', {
        'recipient': recipientAddress,
        'message': message,
        'self_destruct': selfDestruct,
        'destruct_time': destructTime,
      });
      return true;
    } catch (e) {
      throw FuegoRPCException('Failed to send message: $e');
    }
  }

  Future<List<Map<String, dynamic>>> getMessages() async {
    try {
      final response = await _makeRPCCall('get_messages', {});
      return List<Map<String, dynamic>>.from(response['messages'] as List);
    } catch (e) {
      return []; // Return empty list if messaging not available
    }
  }

  // Private helper methods
  Future<Map<String, dynamic>> _makeRPCCall(
    String method, 
    dynamic params,
  ) async {
    try {
      final response = await _dio.post(
        '$_baseUrl/json_rpc',
        data: json.encode({
          'jsonrpc': '2.0',
          'id': DateTime.now().millisecondsSinceEpoch,
          'method': method,
          'params': params,
        }),
      );

      final data = response.data as Map<String, dynamic>;
      
      if (data.containsKey('error')) {
        throw FuegoRPCException(data['error']['message'] as String);
      }
      
      return data['result'] as Map<String, dynamic>;
    } on DioException catch (e) {
      throw FuegoRPCException('Network error: ${e.message}');
    }
  }

  Future<Map<String, dynamic>> _makeWalletRPCCall(
    String method, 
    Map<String, dynamic> params,
  ) async {
    try {
      final walletUrl = _baseUrl.replaceAll(':$defaultRpcPort', ':$defaultWalletPort');
      
      final response = await _dio.post(
        '$walletUrl/json_rpc',
        data: json.encode({
          'jsonrpc': '2.0',
          'id': DateTime.now().millisecondsSinceEpoch,
          'method': method,
          'params': params,
        }),
      );

      final data = response.data as Map<String, dynamic>;
      
      if (data.containsKey('error')) {
        throw FuegoRPCException(data['error']['message'] as String);
      }
      
      return data['result'] as Map<String, dynamic>;
    } on DioException catch (e) {
      throw FuegoRPCException('Wallet service error: ${e.message}');
    }
  }

  // Test connection
  Future<bool> testConnection() async {
    try {
      await getInfo();
      return true;
    } catch (e) {
      return false;
    }
  }

  void dispose() {
    _dio.close();
  }
}

class FuegoRPCException implements Exception {
  final String message;
  
  FuegoRPCException(this.message);
  
  @override
  String toString() => 'FuegoRPCException: $message';
}