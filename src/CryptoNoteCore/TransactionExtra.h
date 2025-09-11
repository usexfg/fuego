// Copyright (c) 2017-2025 Fuego Developers
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

#include <algorithm>
#include <string>
#include <vector>
#include <boost/variant.hpp>

#include <CryptoNote.h>

#define TX_EXTRA_PADDING_MAX_COUNT          255
#define TX_EXTRA_NONCE_MAX_COUNT            255

#define TX_EXTRA_TAG_PADDING                0x00
#define TX_EXTRA_TAG_PUBKEY                 0x01
#define TX_EXTRA_NONCE                      0x02
#define TX_EXTRA_MERGE_MINING_TAG           0x03
#define TX_EXTRA_MESSAGE_TAG                0x04
#define TX_EXTRA_TTL                        0x05
#define TX_EXTRA_ELDERFIER_DEPOSIT          0x06
#define TX_EXTRA_YIELD_COMMITMENT           0x07
#define TX_EXTRA_HEAT_COMMITMENT            0x08
#define TX_EXTRA_CD_DEPOSIT_SECRET          0x09

#define TX_EXTRA_NONCE_PAYMENT_ID           0x00

namespace CryptoNote {

class ISerializer;

struct TransactionExtraPadding {
  size_t size;
};

struct TransactionExtraPublicKey {
  Crypto::PublicKey publicKey;
};

struct TransactionExtraNonce {
  std::vector<uint8_t> nonce;
};

struct TransactionExtraMergeMiningTag {
  size_t depth;
  Crypto::Hash merkleRoot;
};

struct tx_extra_message {
  std::string data;

  bool encrypt(std::size_t index, const std::string &message, const AccountPublicAddress* recipient, const KeyPair &txkey);
  bool decrypt(std::size_t index, const Crypto::PublicKey &txkey, const Crypto::SecretKey *recepient_secret_key, std::string &message) const;

  bool serialize(ISerializer& serializer);
};

struct TransactionExtraTTL {
  uint64_t ttl;
};

struct TransactionExtraHeatCommitment {
  Crypto::Hash commitment;       // 🔒 SECURE: Only commitment hash on blockchain
  uint64_t amount;
  std::vector<uint8_t> metadata;
  
  bool serialize(ISerializer& serializer);
};

struct TransactionExtraYieldCommitment {
  Crypto::Hash commitment;
  uint64_t amount;
  uint32_t term_months;
  std::string yield_scheme;
  std::vector<uint8_t> metadata;
  
  bool serialize(ISerializer& serializer);
};

struct TransactionExtraElderfierDeposit {
  Crypto::Hash depositHash;         // Unique deposit identifier
  uint64_t depositAmount;           // XFG amount (minimum 800 XFG)
  std::string elderfierAddress;     // Elderfier node address
  uint32_t securityWindow;          // Security window in seconds (8 hours = 28800)
  std::vector<uint8_t> metadata;   // Additional metadata
  std::vector<uint8_t> signature;   // Deposit signature
  bool isSlashable;                // True - deposits can be slashed by Elder Council
  
  bool serialize(ISerializer& serializer);
  bool isValid() const;
  std::string toString() const;
};

struct TransactionExtraCDDepositSecret {
  std::vector<uint8_t> secret_key;  // 32-byte deposit secret key
  uint64_t xfg_amount;              // XFG amount for CD conversion
  uint32_t apr_basis_points;        // APR in basis points
  uint8_t term_code;                // CD term code (1=30d, 2=90d, 3=180d)
  uint8_t chain_code;               // Chain code (1=testnet, 2=mainnet)
  std::vector<uint8_t> metadata;    // Additional metadata
  
  bool serialize(ISerializer& serializer);
};

// Removed duplicate TransactionExtraElderfierDeposit struct
// tx_extra_field format, except tx_extra_padding and tx_extra_pub_key:
//   varint tag;
//   varint size;
//   varint data[];
typedef boost::variant<CryptoNote::TransactionExtraPadding, CryptoNote::TransactionExtraPublicKey, CryptoNote::TransactionExtraNonce, CryptoNote::TransactionExtraMergeMiningTag, CryptoNote::tx_extra_message, CryptoNote::TransactionExtraTTL, CryptoNote::TransactionExtraElderfierDeposit, CryptoNote::TransactionExtraHeatCommitment, CryptoNote::TransactionExtraYieldCommitment, CryptoNote::TransactionExtraCDDepositSecret> TransactionExtraField;



template<typename T>
bool findTransactionExtraFieldByType(const std::vector<TransactionExtraField>& tx_extra_fields, T& field) {
  auto it = std::find_if(tx_extra_fields.begin(), tx_extra_fields.end(),
    [](const TransactionExtraField& f) { return typeid(T) == f.type(); });

  if (tx_extra_fields.end() == it)
    return false;

  field = boost::get<T>(*it);
  return true;
}

bool parseTransactionExtra(const std::vector<uint8_t>& tx_extra, std::vector<TransactionExtraField>& tx_extra_fields);
bool writeTransactionExtra(std::vector<uint8_t>& tx_extra, const std::vector<TransactionExtraField>& tx_extra_fields);

Crypto::PublicKey getTransactionPublicKeyFromExtra(const std::vector<uint8_t>& tx_extra);
bool addTransactionPublicKeyToExtra(std::vector<uint8_t>& tx_extra, const Crypto::PublicKey& tx_pub_key);
bool addExtraNonceToTransactionExtra(std::vector<uint8_t>& tx_extra, const BinaryArray& extra_nonce);
void setPaymentIdToTransactionExtraNonce(BinaryArray& extra_nonce, const Crypto::Hash& payment_id);
bool getPaymentIdFromTransactionExtraNonce(const BinaryArray& extra_nonce, Crypto::Hash& payment_id);
bool appendMergeMiningTagToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraMergeMiningTag& mm_tag);
bool append_message_to_extra(std::vector<uint8_t>& tx_extra, const tx_extra_message& message);
std::vector<std::string> get_messages_from_extra(const std::vector<uint8_t>& extra, const Crypto::PublicKey &txkey, const Crypto::SecretKey *recepient_secret_key);
void appendTTLToExtra(std::vector<uint8_t>& tx_extra, uint64_t ttl);
bool getMergeMiningTagFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraMergeMiningTag& mm_tag);

bool createTxExtraWithPaymentId(const std::string& paymentIdString, std::vector<uint8_t>& extra);
//returns false if payment id is not found or parse error
bool getPaymentIdFromTxExtra(const std::vector<uint8_t>& extra, Crypto::Hash& paymentId);
bool parsePaymentId(const std::string& paymentIdString, Crypto::Hash& paymentId);

// HEAT commitment helper functions
bool createTxExtraWithHeatCommitment(const Crypto::Hash& commitment, uint64_t amount, const std::vector<uint8_t>& metadata, std::vector<uint8_t>& extra);
bool addHeatCommitmentToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraHeatCommitment& commitment);
bool getHeatCommitmentFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraHeatCommitment& commitment);

// Yield commitment helper functions
bool createTxExtraWithYieldCommitment(const Crypto::Hash& commitment, uint64_t amount, uint32_t term_months, const std::string& yield_scheme, const std::vector<uint8_t>& metadata, std::vector<uint8_t>& extra);
bool addYieldCommitmentToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraYieldCommitment& commitment);
bool getYieldCommitmentFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraYieldCommitment& commitment);

// Elderfier Deposit helper functions (contingency-based)
bool createTxExtraWithElderfierDeposit(const Crypto::Hash& depositHash, uint64_t depositAmount, const std::string& elderfierAddress, uint32_t securityWindow, const std::vector<uint8_t>& metadata, std::vector<uint8_t>& extra);
bool addElderfierDepositToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraElderfierDeposit& deposit);
bool getElderfierDepositFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraElderfierDeposit& deposit);

// CD Deposit Secret helper functions
bool createTxExtraWithCDDepositSecret(const std::vector<uint8_t>& secret_key, uint64_t xfg_amount, uint32_t apr_basis_points, uint8_t term_code, uint8_t chain_code, const std::vector<uint8_t>& metadata, std::vector<uint8_t>& extra);
bool addCDDepositSecretToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraCDDepositSecret& deposit_secret);
bool getCDDepositSecretFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraCDDepositSecret& deposit_secret);

// Helper APIs for wallet integration
// Computes Keccak256(address || "recipient") into out_hash
bool computeHeatRecipientHash(const std::string& eth_address, Crypto::Hash& out_hash);
// Computes Keccak256(secret || le64(amount) || tx_prefix_hash || recipient_hash || network_id || target_chain_id || version)
Crypto::Hash computeHeatCommitment(const std::array<uint8_t, 32>& secret,
                                   uint64_t amount_atomic,
                                   const Crypto::Hash& tx_prefix_hash,
                                   const std::string& eth_address,
                                   uint32_t network_id,
                                   uint32_t target_chain_id,
                                   uint32_t commitment_version);
// Builds tx.extra with TX_EXTRA_HEAT_COMMITMENT (0x08) given inputs
bool buildHeatExtra(const std::array<uint8_t, 32>& secret,
                    uint64_t amount_atomic,
                    const Crypto::Hash& tx_prefix_hash,
                    const std::string& eth_address,
                    uint32_t network_id,
                    uint32_t target_chain_id,
                    uint32_t commitment_version,
                    const std::vector<uint8_t>& metadata,
                    std::vector<uint8_t>& extra);

}
