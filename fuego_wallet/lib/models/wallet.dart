import 'package:json_annotation/json_annotation.dart';

part 'wallet.g.dart';

@JsonSerializable()
class Wallet {
  final String address;
  final String viewKey;
  final String spendKey;
  final int balance;
  final int unlockedBalance;
  final int blockchainHeight;
  final int localHeight;
  final bool synced;

  const Wallet({
    required this.address,
    required this.viewKey,
    required this.spendKey,
    required this.balance,
    required this.unlockedBalance,
    required this.blockchainHeight,
    required this.localHeight,
    required this.synced,
  });

  factory Wallet.fromJson(Map<String, dynamic> json) => _$WalletFromJson(json);
  Map<String, dynamic> toJson() => _$WalletToJson(this);

  Wallet copyWith({
    String? address,
    String? viewKey,
    String? spendKey,
    int? balance,
    int? unlockedBalance,
    int? blockchainHeight,
    int? localHeight,
    bool? synced,
  }) {
    return Wallet(
      address: address ?? this.address,
      viewKey: viewKey ?? this.viewKey,
      spendKey: spendKey ?? this.spendKey,
      balance: balance ?? this.balance,
      unlockedBalance: unlockedBalance ?? this.unlockedBalance,
      blockchainHeight: blockchainHeight ?? this.blockchainHeight,
      localHeight: localHeight ?? this.localHeight,
      synced: synced ?? this.synced,
    );
  }

  // Convert atomic units to XFG (assuming 8 decimal places)
  double get balanceXFG => balance / 100000000.0;
  double get unlockedBalanceXFG => unlockedBalance / 100000000.0;
  
  // Sync progress percentage
  double get syncProgress => 
      blockchainHeight > 0 ? (localHeight / blockchainHeight).clamp(0.0, 1.0) : 0.0;
}

@JsonSerializable()
class WalletTransaction {
  final String txid;
  final int amount;
  final int fee;
  final String paymentId;
  final int blockHeight;
  final int timestamp;
  final bool isSpending;
  final String? address;
  final int confirmations;

  const WalletTransaction({
    required this.txid,
    required this.amount,
    required this.fee,
    required this.paymentId,
    required this.blockHeight,
    required this.timestamp,
    required this.isSpending,
    this.address,
    required this.confirmations,
  });

  factory WalletTransaction.fromJson(Map<String, dynamic> json) => 
      _$WalletTransactionFromJson(json);
  Map<String, dynamic> toJson() => _$WalletTransactionToJson(this);

  // Convert atomic units to XFG
  double get amountXFG => amount / 100000000.0;
  double get feeXFG => fee / 100000000.0;
  
  // Get transaction status
  TransactionStatus get status {
    if (confirmations == 0) return TransactionStatus.pending;
    if (confirmations < 10) return TransactionStatus.confirming;
    return TransactionStatus.confirmed;
  }

  DateTime get dateTime => DateTime.fromMillisecondsSinceEpoch(timestamp * 1000);
}

enum TransactionStatus { pending, confirming, confirmed }

@JsonSerializable()
class SendTransactionRequest {
  final String address;
  final int amount;
  final String paymentId;
  final int fee;
  final int mixins;

  const SendTransactionRequest({
    required this.address,
    required this.amount,
    required this.paymentId,
    required this.fee,
    this.mixins = 7, // Default ring size
  });

  factory SendTransactionRequest.fromJson(Map<String, dynamic> json) => 
      _$SendTransactionRequestFromJson(json);
  Map<String, dynamic> toJson() => _$SendTransactionRequestToJson(this);
}

@JsonSerializable()
class ElderfierNode {
  final String nodeId;
  final String customName;
  final String address;
  final int stakeAmount;
  final bool isActive;
  final int uptime;
  final int lastSeenBlock;
  final String consensusType;

  const ElderfierNode({
    required this.nodeId,
    required this.customName,
    required this.address,
    required this.stakeAmount,
    required this.isActive,
    required this.uptime,
    required this.lastSeenBlock,
    required this.consensusType,
  });

  factory ElderfierNode.fromJson(Map<String, dynamic> json) => 
      _$ElderfierNodeFromJson(json);
  Map<String, dynamic> toJson() => _$ElderfierNodeToJson(this);

  double get stakeAmountXFG => stakeAmount / 100000000.0;
}