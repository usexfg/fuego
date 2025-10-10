// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'wallet.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

Wallet _$WalletFromJson(Map<String, dynamic> json) => Wallet(
      address: json['address'] as String,
      viewKey: json['viewKey'] as String,
      spendKey: json['spendKey'] as String,
      balance: json['balance'] as int,
      unlockedBalance: json['unlockedBalance'] as int,
      blockchainHeight: json['blockchainHeight'] as int,
      localHeight: json['localHeight'] as int,
      synced: json['synced'] as bool,
    );

Map<String, dynamic> _$WalletToJson(Wallet instance) => <String, dynamic>{
      'address': instance.address,
      'viewKey': instance.viewKey,
      'spendKey': instance.spendKey,
      'balance': instance.balance,
      'unlockedBalance': instance.unlockedBalance,
      'blockchainHeight': instance.blockchainHeight,
      'localHeight': instance.localHeight,
      'synced': instance.synced,
    };

WalletTransaction _$WalletTransactionFromJson(Map<String, dynamic> json) =>
    WalletTransaction(
      txid: json['txid'] as String,
      amount: json['amount'] as int,
      fee: json['fee'] as int,
      paymentId: json['paymentId'] as String,
      blockHeight: json['blockHeight'] as int,
      timestamp: json['timestamp'] as int,
      isSpending: json['isSpending'] as bool,
      address: json['address'] as String?,
      confirmations: json['confirmations'] as int,
    );

Map<String, dynamic> _$WalletTransactionToJson(WalletTransaction instance) =>
    <String, dynamic>{
      'txid': instance.txid,
      'amount': instance.amount,
      'fee': instance.fee,
      'paymentId': instance.paymentId,
      'blockHeight': instance.blockHeight,
      'timestamp': instance.timestamp,
      'isSpending': instance.isSpending,
      'address': instance.address,
      'confirmations': instance.confirmations,
    };

SendTransactionRequest _$SendTransactionRequestFromJson(
        Map<String, dynamic> json) =>
    SendTransactionRequest(
      address: json['address'] as String,
      amount: json['amount'] as int,
      paymentId: json['paymentId'] as String,
      fee: json['fee'] as int,
      mixins: json['mixins'] as int? ?? 7,
    );

Map<String, dynamic> _$SendTransactionRequestToJson(
        SendTransactionRequest instance) =>
    <String, dynamic>{
      'address': instance.address,
      'amount': instance.amount,
      'paymentId': instance.paymentId,
      'fee': instance.fee,
      'mixins': instance.mixins,
    };

ElderfierNode _$ElderfierNodeFromJson(Map<String, dynamic> json) =>
    ElderfierNode(
      nodeId: json['nodeId'] as String,
      customName: json['customName'] as String,
      address: json['address'] as String,
      stakeAmount: json['stakeAmount'] as int,
      isActive: json['isActive'] as bool,
      uptime: json['uptime'] as int,
      lastSeenBlock: json['lastSeenBlock'] as int,
      consensusType: json['consensusType'] as String,
    );

Map<String, dynamic> _$ElderfierNodeToJson(ElderfierNode instance) =>
    <String, dynamic>{
      'nodeId': instance.nodeId,
      'customName': instance.customName,
      'address': instance.address,
      'stakeAmount': instance.stakeAmount,
      'isActive': instance.isActive,
      'uptime': instance.uptime,
      'lastSeenBlock': instance.lastSeenBlock,
      'consensusType': instance.consensusType,
    };