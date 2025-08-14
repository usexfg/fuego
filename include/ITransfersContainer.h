// Copyright (c) 2012-2018 The CryptoNote developers
// Copyright (c) 2017-2025 Elderfire Privacy Council
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful- but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You are encouraged to redistribute it and/or modify it
// under the terms of the GNU General Public License v3 or later
// versions as published by the Free Software Foundation.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include "crypto/hash.h"
#include "ITransaction.h"
#include "IObservable.h"
#include "IStreamSerializable.h"

namespace CryptoNote
{

  const uint32_t UNCONFIRMED_TRANSACTION_GLOBAL_OUTPUT_INDEX = std::numeric_limits<uint32_t>::max();

  struct TransactionInformation
  {
    // transaction info
    Crypto::Hash transactionHash;
    Crypto::PublicKey publicKey;
    uint32_t blockHeight;
    uint64_t timestamp;
    size_t firstDepositId;
    size_t depositCount = 0;
    uint64_t unlockTime;
    uint64_t totalAmountIn;
    uint64_t totalAmountOut;
    std::vector<uint8_t> extra;
    Crypto::Hash paymentId;
    std::vector<std::string> messages;
  };

  struct TransactionOutputInformation
  {
    // output info
    TransactionTypes::OutputType type;
    uint64_t amount;
    uint32_t globalOutputIndex;
    uint32_t outputInTransaction;

    // transaction info
    Crypto::Hash transactionHash;
    Crypto::PublicKey transactionPublicKey;

    union
    {
      Crypto::PublicKey outputKey; // Type: Key
      struct
      {
        uint32_t requiredSignatures; // Type: Multisignature
        uint32_t term;
      };
    };
  };

  struct TransactionSpentOutputInformation : public TransactionOutputInformation
  {
    uint32_t spendingBlockHeight;
    uint64_t timestamp;
    Crypto::Hash spendingTransactionHash;
    Crypto::KeyImage keyImage; //!< \attention Used only for TransactionTypes::OutputType::Key
    uint32_t inputInTransaction;
  };

  class ITransfersContainer : public IStreamSerializable
  {
  public:
    enum Flags : uint32_t
    {
      // state
      IncludeStateUnlocked = 0x01,
      IncludeStateLocked = 0x02,
      IncludeStateSoftLocked = 0x04,
      IncludeStateSpent = 0x08,
      // output type
      IncludeTypeKey = 0x100,
      IncludeTypeMultisignature = 0x200,
      IncludeTypeDeposit = 0x400,
      // combinations
      IncludeStateAll = 0xff,
      IncludeTypeAll = 0xff00,

      IncludeKeyUnlocked = IncludeTypeKey | IncludeStateUnlocked,
      IncludeKeyNotUnlocked = IncludeTypeKey | IncludeStateLocked | IncludeStateSoftLocked,

      IncludeAllLocked = IncludeTypeAll | IncludeStateLocked | IncludeStateSoftLocked,
      IncludeAllUnlocked = IncludeTypeAll | IncludeStateUnlocked,
      IncludeAll = IncludeTypeAll | IncludeStateAll,

      IncludeDefault = IncludeKeyUnlocked
    };

    enum class TransferState : uint32_t
    {
      TransferUnconfirmed,
      TransferLocked,
      TransferAvailable,
      TransferSpent
    };

    virtual size_t transfersCount() const = 0;
    virtual size_t transactionsCount() const = 0;
    virtual uint64_t balance(uint32_t flags = IncludeDefault) const = 0;
    virtual void getOutputs(std::vector<TransactionOutputInformation> &transfers, uint32_t flags = IncludeDefault) const = 0;
    virtual bool getTransactionInformation(const Crypto::Hash &transactionHash, TransactionInformation &info,
                                           uint64_t *amountIn = nullptr, uint64_t *amountOut = nullptr) const = 0;
    virtual std::vector<TransactionOutputInformation> getTransactionOutputs(const Crypto::Hash &transactionHash, uint32_t flags = IncludeDefault) const = 0;
    //only type flags are feasible for this function
    virtual std::vector<TransactionOutputInformation> getTransactionInputs(const Crypto::Hash &transactionHash, uint32_t flags) const = 0;
    virtual void getUnconfirmedTransactions(std::vector<Crypto::Hash> &transactions) const = 0;
    virtual std::vector<TransactionSpentOutputInformation> getSpentOutputs() const = 0;
    virtual bool getTransfer(const Crypto::Hash &transactionHash, uint32_t outputInTransaction, TransactionOutputInformation &transfer, TransferState &transferState) const = 0;
  };

} // namespace CryptoNote
