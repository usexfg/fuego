// Copyright (c) 2017-2022 Fuego Developers
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

#pragma once

#include "IBlockchainSynchronizer.h"
#include "ITransfersSynchronizer.h"
#include "TransfersSubscription.h"
#include "TypeHelpers.h"

#include "crypto/crypto.h"
#include "Logging/LoggerRef.h"

#include "IObservableImpl.h"

#include <unordered_set>

namespace CryptoNote {

class INode;

class TransfersConsumer: public IObservableImpl<IBlockchainConsumerObserver, IBlockchainConsumer> {
public:

  TransfersConsumer(const CryptoNote::Currency& currency, INode& node, Logging::ILogger& logger, const Crypto::SecretKey& viewSecret);

  ITransfersSubscription& addSubscription(const AccountSubscription& subscription);
  // returns true if no subscribers left
  bool removeSubscription(const AccountPublicAddress& address);
  ITransfersSubscription* getSubscription(const AccountPublicAddress& acc);
  void getSubscriptions(std::vector<AccountPublicAddress>& subscriptions);

  void initTransactionPool(const std::unordered_set<Crypto::Hash>& uncommitedTransactions);
  void addPublicKeysSeen(const Crypto::Hash& transactionHash, const Crypto::PublicKey& outputKey);
  
  // IBlockchainConsumer
  virtual SynchronizationStart getSyncStart() override;
  virtual void onBlockchainDetach(uint32_t height) override;
  virtual bool onNewBlocks(const CompleteBlock* blocks, uint32_t startHeight, uint32_t count) override;
  virtual std::error_code onPoolUpdated(const std::vector<std::unique_ptr<ITransactionReader>>& addedTransactions, const std::vector<Crypto::Hash>& deletedTransactions) override;
  virtual const std::unordered_set<Crypto::Hash>& getKnownPoolTxIds() const override;

  virtual std::error_code addUnconfirmedTransaction(const ITransactionReader& transaction) override;
  virtual void removeUnconfirmedTransaction(const Crypto::Hash& transactionHash) override;

private:

  template <typename F>
  void forEachSubscription(F action) {
    for (const auto& kv : m_subscriptions) {
      action(*kv.second);
    }
  }

  struct PreprocessInfo {
    std::unordered_map<Crypto::PublicKey, std::vector<TransactionOutputInformationIn>> outputs;
    std::vector<uint32_t> globalIdxs;
  };

  std::error_code preprocessOutputs(const TransactionBlockInfo& blockInfo, const ITransactionReader& tx, PreprocessInfo& info);
  std::error_code processTransaction(const TransactionBlockInfo& blockInfo, const ITransactionReader& tx);
  void processTransaction(const TransactionBlockInfo& blockInfo, const ITransactionReader& tx, const PreprocessInfo& info);
  void processOutputs(const TransactionBlockInfo& blockInfo, TransfersSubscription& sub, const ITransactionReader& tx,
    const std::vector<TransactionOutputInformationIn>& outputs, const std::vector<uint32_t>& globalIdxs, bool& contains, bool& updated);

  std::error_code getGlobalIndices(const Crypto::Hash& transactionHash, std::vector<uint32_t>& outsGlobalIndices);

  void updateSyncStart();

  SynchronizationStart m_syncStart;
  const Crypto::SecretKey m_viewSecret;
  // map { spend public key -> subscription }
  std::unordered_map<Crypto::PublicKey, std::unique_ptr<TransfersSubscription>> m_subscriptions;
  std::unordered_set<Crypto::PublicKey> m_spendKeys;
  std::unordered_set<Crypto::Hash> m_poolTxs;

  INode& m_node;
  const CryptoNote::Currency& m_currency;
  Logging::LoggerRef m_logger;
};

}
