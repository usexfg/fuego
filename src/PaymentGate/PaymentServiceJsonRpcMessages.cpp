// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "PaymentServiceJsonRpcMessages.h"
#include "Serialization/SerializationOverloads.h"

namespace PaymentService
{

void Save::Request::serialize(CryptoNote::ISerializer & /*serializer*/)
{
}

void Save::Response::serialize(CryptoNote::ISerializer & /*serializer*/)
{
}

void Reset::Request::serialize(CryptoNote::ISerializer& serializer) {
  serializer(viewSecretKey, "privateViewKey");
  serializer(scanHeight, "scanHeight");
}

void Reset::Response::serialize(CryptoNote::ISerializer& serializer) {
}

void ExportWallet::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(exportFilename, "exportFilename");
}

void ExportWallet::Response::serialize(CryptoNote::ISerializer &serializer)
{
}

void ExportWalletKeys::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(exportFilename, "exportFilename");
}

void ExportWalletKeys::Response::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetViewKey::Request::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetViewKey::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(viewSecretKey, "privateViewKey");
}

void GetStatus::Request::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetStatus::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(blockCount, "blockCount");
  serializer(knownBlockCount, "knownBlockCount");
  serializer(lastBlockHash, "lastBlockHash");
  serializer(peerCount, "peerCount");
  serializer(depositCount, "depositCount");
  serializer(transactionCount, "transactionCount");
  serializer(addressCount, "addressCount");
}

void CreateDeposit::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(amount, "amount");
  serializer(term, "term");
  serializer(sourceAddress, "sourceAddress");
}

void CreateDeposit::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHash, "transactionHash");
}

void WithdrawDeposit::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(depositId, "depositId");
}

void WithdrawDeposit::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHash, "transactionHash");
}

void SendDeposit::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(amount, "amount");
  serializer(term, "term");
  serializer(sourceAddress, "sourceAddress");
  serializer(destinationAddress, "destinationAddress");
}

void SendDeposit::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHash, "transactionHash");
}


void GetDeposit::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(depositId, "depositId");
}

void GetDeposit::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(amount, "amount");
  serializer(term, "term");
  serializer(interest, "interest");
  serializer(creatingTransactionHash, "creatingTransactionHash");
  serializer(spendingTransactionHash, "spendingTransactionHash");
  serializer(height, "height");
  serializer(unlockHeight, "unlockHeight");
  serializer(locked, "locked");
  serializer(address, "address");
}

void GetAddresses::Request::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetAddresses::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(addresses, "addresses");
}

void CreateAddress::Request::serialize(CryptoNote::ISerializer &serializer)
{
  bool hasSecretKey = serializer(spendSecretKey, "privateSpendKey");
  bool hasPublicKey = serializer(spendPublicKey, "publicSpendKey");

  if (hasSecretKey && hasPublicKey)
  {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }
}

void CreateAddress::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(address, "address");
}

void CreateAddressList::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(spendSecretKeys, "privateSpendKeys"))
  {
    throw RequestSerializationError();
  }
}

void CreateAddressList::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(addresses, "addresses");
}

void DeleteAddress::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(address, "address"))
  {
    throw RequestSerializationError();
  }
}

void DeleteAddress::Response::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetSpendKeys::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(address, "address"))
  {
    throw RequestSerializationError();
  }
}

void GetSpendKeys::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(spendSecretKey, "privateSpendKey");
  serializer(spendPublicKey, "publicSpendKey");
}

void GetBalance::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(address, "address");
}

void GetBalance::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(availableBalance, "availableBalance");
  serializer(lockedAmount, "lockedAmount");
  serializer(lockedDepositBalance, "lockedDepositBalance");
  serializer(unlockedDepositBalance, "unlockedDepositBalance");
}

void GetBlockHashes::Request::serialize(CryptoNote::ISerializer &serializer)
{
  bool r = serializer(firstBlockIndex, "firstBlockIndex");
  r &= serializer(blockCount, "blockCount");

  if (!r)
  {
    throw RequestSerializationError();
  }
}

void GetBlockHashes::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(blockHashes, "blockHashes");
}

void TransactionHashesInBlockRpcInfo::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(blockHash, "blockHash");
  serializer(transactionHashes, "transactionHashes");
}

void GetTransactionHashes::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(addresses, "addresses");

  if (serializer(blockHash, "blockHash") == serializer(firstBlockIndex, "firstBlockIndex"))
  {
    throw RequestSerializationError();
  }

  if (!serializer(blockCount, "blockCount"))
  {
    throw RequestSerializationError();
  }

  serializer(paymentId, "paymentId");
}

void GetTransactionHashes::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(items, "items");
}

void CreateIntegrated::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(address, "address");
  serializer(payment_id, "payment_id");
}

void CreateIntegrated::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(integrated_address, "integrated_address");
}

void SplitIntegrated::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(integrated_address, "integrated_address");
}

void SplitIntegrated::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(address, "address");
  serializer(payment_id, "payment_id");
}

void TransferRpcInfo::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(type, "type");
  serializer(address, "address");
  serializer(amount, "amount");
  serializer(message, "message");
}

void TransactionRpcInfo::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(state, "state");
  serializer(transactionHash, "transactionHash");
  serializer(blockIndex, "blockIndex");
  serializer(confirmations, "confirmations");
  serializer(timestamp, "timestamp");
  serializer(isBase, "isBase");
  serializer(unlockTime, "unlockTime");
  serializer(amount, "amount");
  serializer(fee, "fee");
  serializer(transfers, "transfers");
  serializer(extra, "extra");
  serializer(firstDepositId, "firstDepositId");
  serializer(depositCount, "depositCount");
  serializer(paymentId, "paymentId");
}

void GetTransaction::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(transactionHash, "transactionHash"))
  {
    throw RequestSerializationError();
  }
}

void GetTransaction::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transaction, "transaction");
}

void TransactionsInBlockRpcInfo::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(blockHash, "blockHash");
  serializer(transactions, "transactions");
}

void GetTransactions::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(addresses, "addresses");

  if (serializer(blockHash, "blockHash") == serializer(firstBlockIndex, "firstBlockIndex"))
  {
    throw RequestSerializationError();
  }

  if (!serializer(blockCount, "blockCount"))
  {
    throw RequestSerializationError();
  }

  serializer(paymentId, "paymentId");
}

void GetTransactions::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(items, "items");
}

void GetUnconfirmedTransactionHashes::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(addresses, "addresses");
}

void GetUnconfirmedTransactionHashes::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHashes, "transactionHashes");
}

void WalletRpcOrder::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(message, "message");
  bool r = serializer(address, "address");
  r &= serializer(amount, "amount");

  if (!r)
  {
    throw RequestSerializationError();
  }
}

void WalletRpcMessage::serialize(CryptoNote::ISerializer &serializer)
{
  bool r = serializer(address, "address");
  r &= serializer(message, "message");

  if (!r)
  {
    throw RequestSerializationError();
  }
}

void SendTransaction::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(sourceAddresses, "addresses");

  if (!serializer(transfers, "transfers"))
  {
    throw RequestSerializationError();
  }

  serializer(changeAddress, "changeAddress");

  if (!serializer(fee, "fee"))
  {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity"))
  {
    throw RequestSerializationError();
  }

  bool hasExtra = serializer(extra, "extra");
  bool hasPaymentId = serializer(paymentId, "paymentId");

  if (hasExtra && hasPaymentId)
  {
    throw RequestSerializationError();
  }

  serializer(unlockTime, "unlockTime");
}

void SendTransaction::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHash, "transactionHash");
  serializer(transactionSecretKey, "transactionSecretKey");
}

void CreateDelayedTransaction::Request::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(addresses, "addresses");

  if (!serializer(transfers, "transfers"))
  {
    throw RequestSerializationError();
  }

  serializer(changeAddress, "changeAddress");

  if (!serializer(fee, "fee"))
  {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity"))
  {
    throw RequestSerializationError();
  }

  bool hasExtra = serializer(extra, "extra");
  bool hasPaymentId = serializer(paymentId, "paymentId");

  if (hasExtra && hasPaymentId)
  {
    throw RequestSerializationError();
  }

  serializer(unlockTime, "unlockTime");
}

void CreateDelayedTransaction::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHash, "transactionHash");
}

void GetDelayedTransactionHashes::Request::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetDelayedTransactionHashes::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHashes, "transactionHashes");
}

void DeleteDelayedTransaction::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(transactionHash, "transactionHash"))
  {
    throw RequestSerializationError();
  }
}

void DeleteDelayedTransaction::Response::serialize(CryptoNote::ISerializer &serializer)
{
}

void SendDelayedTransaction::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(transactionHash, "transactionHash"))
  {
    throw RequestSerializationError();
  }
}

void SendDelayedTransaction::Response::serialize(CryptoNote::ISerializer &serializer)
{
}

void GetMessagesFromExtra::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(extra, "extra"))
  {
    throw RequestSerializationError();
  }
}

void GetMessagesFromExtra::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(messages, "messages");
}

void EstimateFusion::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(threshold, "threshold"))
  {
    throw RequestSerializationError();
  }

  serializer(addresses, "addresses");
}

void EstimateFusion::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(fusionReadyCount, "fusionReadyCount");
  serializer(totalOutputCount, "totalOutputCount");
}

void SendFusionTransaction::Request::serialize(CryptoNote::ISerializer &serializer)
{
  if (!serializer(threshold, "threshold"))
  {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity"))
  {
    throw RequestSerializationError();
  }

  serializer(addresses, "addresses");
  serializer(destinationAddress, "destinationAddress");
}

void SendFusionTransaction::Response::serialize(CryptoNote::ISerializer &serializer)
{
  serializer(transactionHash, "transactionHash");
}

} // namespace PaymentService
