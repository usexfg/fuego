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

#include "TransactionExtra.h"
#include "crypto/chacha8.h"
#include "Common/int-util.h"
#include "Common/MemoryInputStream.h"
#include "Common/StreamTools.h"
#include "Common/StringTools.h"
#include "Common/Varint.h"
#include "CryptoNoteTools.h"
#include "Serialization/BinaryOutputStreamSerializer.h"
#include "Serialization/BinaryInputStreamSerializer.h"
#include "crypto/keccak.h"
#include <memory>
#include <sstream>
#include <chrono>

using namespace Crypto;
using namespace Common;

namespace CryptoNote
{

  bool parseTransactionExtra(const std::vector<uint8_t> &transactionExtra, std::vector<TransactionExtraField> &transactionExtraFields)
  {
    transactionExtraFields.clear();

    if (transactionExtra.empty())
      return true;

    try
    {
      MemoryInputStream iss(transactionExtra.data(), transactionExtra.size());
      BinaryInputStreamSerializer ar(iss);

      int c = 0;

      while (!iss.endOfStream())
      {
        c = read<uint8_t>(iss);
        switch (c)
        {
        case TX_EXTRA_TAG_PADDING:
        {
          size_t size = 1;
          for (; !iss.endOfStream() && size <= TX_EXTRA_PADDING_MAX_COUNT; ++size)
          {
            if (read<uint8_t>(iss) != 0)
            {
              return false; // all bytes should be zero
            }
          }

          if (size > TX_EXTRA_PADDING_MAX_COUNT)
          {
            return false;
          }

          transactionExtraFields.push_back(TransactionExtraPadding{size});
          break;
        }

        case TX_EXTRA_TAG_PUBKEY:
        {
          TransactionExtraPublicKey extraPk;
          ar(extraPk.publicKey, "public_key");
          transactionExtraFields.push_back(extraPk);
          break;
        }

        case TX_EXTRA_NONCE:
        {
          TransactionExtraNonce extraNonce;
          uint8_t size = read<uint8_t>(iss);
          if (size > 0)
          {
            extraNonce.nonce.resize(size);
            read(iss, extraNonce.nonce.data(), extraNonce.nonce.size());
          }

          transactionExtraFields.push_back(extraNonce);
          break;
        }

        case TX_EXTRA_MERGE_MINING_TAG:
        {
          TransactionExtraMergeMiningTag mmTag;
          ar(mmTag, "mm_tag");
          transactionExtraFields.push_back(mmTag);
          break;
        }

        case TX_EXTRA_MESSAGE_TAG:
        {
          tx_extra_message message;
          ar(message.data, "message");
          transactionExtraFields.push_back(message);
          break;
        }

        case TX_EXTRA_TTL:
        {
          uint8_t size;
          readVarint(iss, size);
          TransactionExtraTTL ttl;
          readVarint(iss, ttl.ttl);
          transactionExtraFields.push_back(ttl);
          break;
        }

        case TX_EXTRA_ELDERFIER_DEPOSIT:
        {
          TransactionExtraElderfierDeposit deposit;
          if (getElderfierDepositFromExtra(transactionExtra, deposit)) {
            transactionExtraFields.push_back(deposit);
          } else {
            return false;
          }
          break;
        }
        }
      }
    }
    catch (std::exception &)
    {
      return false;
    }

    return true;
  }

  struct ExtraSerializerVisitor : public boost::static_visitor<bool>
  {
    std::vector<uint8_t> &extra;

    ExtraSerializerVisitor(std::vector<uint8_t> &tx_extra)
        : extra(tx_extra) {}

    bool operator()(const TransactionExtraPadding &t)
    {
      if (t.size > TX_EXTRA_PADDING_MAX_COUNT)
      {
        return false;
      }
      extra.insert(extra.end(), t.size, 0);
      return true;
    }

    bool operator()(const TransactionExtraPublicKey &t)
    {
      return addTransactionPublicKeyToExtra(extra, t.publicKey);
    }

    bool operator()(const TransactionExtraNonce &t)
    {
      return addExtraNonceToTransactionExtra(extra, t.nonce);
    }

    bool operator()(const TransactionExtraMergeMiningTag &t)
    {
      return appendMergeMiningTagToExtra(extra, t);
    }

    bool operator()(const tx_extra_message &t)
    {
      return append_message_to_extra(extra, t);
    }

    bool operator()(const TransactionExtraTTL &t)
    {
      appendTTLToExtra(extra, t.ttl);
      return true;
    }

    bool operator()(const TransactionExtraElderfierDeposit &t)
    {
      return addElderfierDepositToExtra(extra, t);
    }

    bool operator()(const TransactionExtraHeatCommitment &t)
    {
      return addHeatCommitmentToExtra(extra, t);
    }

    bool operator()(const TransactionExtraYieldCommitment &t)
    {
      return addYieldCommitmentToExtra(extra, t);
    }

    bool operator()(const TransactionExtraCDDepositSecret &t)
    {
      return addCDDepositSecretToExtra(extra, t);
    }

  };

  bool writeTransactionExtra(std::vector<uint8_t> &tx_extra, const std::vector<TransactionExtraField> &tx_extra_fields)
  {
    ExtraSerializerVisitor visitor(tx_extra);

    for (const auto &tag : tx_extra_fields)
    {
      if (!boost::apply_visitor(visitor, tag))
      {
        return false;
      }
    }

    return true;
  }

  PublicKey getTransactionPublicKeyFromExtra(const std::vector<uint8_t> &tx_extra)
  {
    std::vector<TransactionExtraField> tx_extra_fields;
    parseTransactionExtra(tx_extra, tx_extra_fields);

    TransactionExtraPublicKey pub_key_field;
    if (!findTransactionExtraFieldByType(tx_extra_fields, pub_key_field))
      return boost::value_initialized<PublicKey>();

    return pub_key_field.publicKey;
  }

  bool addTransactionPublicKeyToExtra(std::vector<uint8_t> &tx_extra, const PublicKey &tx_pub_key)
  {
    tx_extra.resize(tx_extra.size() + 1 + sizeof(PublicKey));
    tx_extra[tx_extra.size() - 1 - sizeof(PublicKey)] = TX_EXTRA_TAG_PUBKEY;
    *reinterpret_cast<PublicKey *>(&tx_extra[tx_extra.size() - sizeof(PublicKey)]) = tx_pub_key;
    return true;
  }

  bool addExtraNonceToTransactionExtra(std::vector<uint8_t> &tx_extra, const BinaryArray &extra_nonce)
  {
    if (extra_nonce.size() > TX_EXTRA_NONCE_MAX_COUNT)
    {
      return false;
    }

    size_t start_pos = tx_extra.size();
    tx_extra.resize(tx_extra.size() + 2 + extra_nonce.size());
    //write tag
    tx_extra[start_pos] = TX_EXTRA_NONCE;
    //write len
    ++start_pos;
    tx_extra[start_pos] = static_cast<uint8_t>(extra_nonce.size());
    //write data
    ++start_pos;
    memcpy(&tx_extra[start_pos], extra_nonce.data(), extra_nonce.size());
    return true;
  }

  bool appendMergeMiningTagToExtra(std::vector<uint8_t> &tx_extra, const TransactionExtraMergeMiningTag &mm_tag)
  {
    BinaryArray blob;
    if (!toBinaryArray(mm_tag, blob))
    {
      return false;
    }

    tx_extra.push_back(TX_EXTRA_MERGE_MINING_TAG);
    std::copy(reinterpret_cast<const uint8_t *>(blob.data()), reinterpret_cast<const uint8_t *>(blob.data() + blob.size()), std::back_inserter(tx_extra));
    return true;
  }

  bool getMergeMiningTagFromExtra(const std::vector<uint8_t> &tx_extra, TransactionExtraMergeMiningTag &mm_tag)
  {
    std::vector<TransactionExtraField> tx_extra_fields;
    parseTransactionExtra(tx_extra, tx_extra_fields);

    return findTransactionExtraFieldByType(tx_extra_fields, mm_tag);
  }

  bool append_message_to_extra(std::vector<uint8_t> &tx_extra, const tx_extra_message &message)
  {
    BinaryArray blob;
    if (!toBinaryArray(message, blob))
    {
      return false;
    }

    tx_extra.reserve(tx_extra.size() + 1 + blob.size());
    tx_extra.push_back(TX_EXTRA_MESSAGE_TAG);
    std::copy(reinterpret_cast<const uint8_t *>(blob.data()), reinterpret_cast<const uint8_t *>(blob.data() + blob.size()), std::back_inserter(tx_extra));

    return true;
  }

  std::vector<std::string> get_messages_from_extra(const std::vector<uint8_t> &extra, const Crypto::PublicKey &txkey, const Crypto::SecretKey *recepient_secret_key)
  {
    std::vector<TransactionExtraField> tx_extra_fields;
    std::vector<std::string> result;
    if (!parseTransactionExtra(extra, tx_extra_fields))
    {
      return result;
    }
    size_t i = 0;
    for (const auto &f : tx_extra_fields)
    {
      if (f.type() != typeid(tx_extra_message))
      {
        continue;
      }
      std::string res;
      if (boost::get<tx_extra_message>(f).decrypt(i, txkey, recepient_secret_key, res))
      {
        result.push_back(res);
      }
      ++i;
    }
    return result;
  }

  void appendTTLToExtra(std::vector<uint8_t> &tx_extra, uint64_t ttl)
  {
    std::string ttlData = Tools::get_varint_data(ttl);
    std::string extraFieldSize = Tools::get_varint_data(ttlData.size());

    tx_extra.reserve(tx_extra.size() + 1 + extraFieldSize.size() + ttlData.size());
    tx_extra.push_back(TX_EXTRA_TTL);
    std::copy(extraFieldSize.begin(), extraFieldSize.end(), std::back_inserter(tx_extra));
    std::copy(ttlData.begin(), ttlData.end(), std::back_inserter(tx_extra));
  }

  void setPaymentIdToTransactionExtraNonce(std::vector<uint8_t> &extra_nonce, const Hash &payment_id)
  {
    extra_nonce.clear();
    extra_nonce.push_back(TX_EXTRA_NONCE_PAYMENT_ID);
    const uint8_t *payment_id_ptr = reinterpret_cast<const uint8_t *>(&payment_id);
    std::copy(payment_id_ptr, payment_id_ptr + sizeof(payment_id), std::back_inserter(extra_nonce));
  }

  bool getPaymentIdFromTransactionExtraNonce(const std::vector<uint8_t> &extra_nonce, Hash &payment_id)
  {
    if (sizeof(Hash) + 1 != extra_nonce.size())
      return false;
    if (TX_EXTRA_NONCE_PAYMENT_ID != extra_nonce[0])
      return false;
    payment_id = *reinterpret_cast<const Hash *>(extra_nonce.data() + 1);
    return true;
  }

  bool parsePaymentId(const std::string &paymentIdString, Hash &paymentId)
  {
    return Common::podFromHex(paymentIdString, paymentId);
  }

  bool createTxExtraWithPaymentId(const std::string &paymentIdString, std::vector<uint8_t> &extra)
  {
    Hash paymentIdBin;

    if (!parsePaymentId(paymentIdString, paymentIdBin))
    {
      return false;
    }

    std::vector<uint8_t> extraNonce;
    CryptoNote::setPaymentIdToTransactionExtraNonce(extraNonce, paymentIdBin);

    if (!CryptoNote::addExtraNonceToTransactionExtra(extra, extraNonce))
    {
      return false;
    }

    return true;
  }

  bool getPaymentIdFromTxExtra(const std::vector<uint8_t> &extra, Hash &paymentId)
  {
    std::vector<TransactionExtraField> tx_extra_fields;
    if (!parseTransactionExtra(extra, tx_extra_fields))
    {
      return false;
    }

    TransactionExtraNonce extra_nonce;
    if (findTransactionExtraFieldByType(tx_extra_fields, extra_nonce))
    {
      if (!getPaymentIdFromTransactionExtraNonce(extra_nonce.nonce, paymentId))
      {
        return false;
      }
    }
    else
    {
      return false;
    }

    return true;
  }

#define TX_EXTRA_MESSAGE_CHECKSUM_SIZE 4

#pragma pack(push, 1)
  struct message_key_data
  {
    KeyDerivation derivation;
    uint8_t magic1, magic2;
  };
#pragma pack(pop)
  static_assert(sizeof(message_key_data) == 34, "Invalid structure size");

  bool tx_extra_message::encrypt(size_t index, const std::string &message, const AccountPublicAddress *recipient, const KeyPair &txkey)
  {
    size_t mlen = message.size();
    std::unique_ptr<char[]> buf(new char[mlen + TX_EXTRA_MESSAGE_CHECKSUM_SIZE]);
    memcpy(buf.get(), message.data(), mlen);
    memset(buf.get() + mlen, 0, TX_EXTRA_MESSAGE_CHECKSUM_SIZE);
    mlen += TX_EXTRA_MESSAGE_CHECKSUM_SIZE;
    if (recipient)
    {
      message_key_data key_data;
      if (!generate_key_derivation(recipient->spendPublicKey, txkey.secretKey, key_data.derivation))
      {
        return false;
      }
      key_data.magic1 = 0x80;
      key_data.magic2 = 0;
      Hash h = cn_fast_hash(&key_data, sizeof(message_key_data));
      uint64_t nonce = SWAP64LE(index);
      chacha8(buf.get(), mlen, reinterpret_cast<uint8_t *>(&h), reinterpret_cast<uint8_t *>(&nonce), buf.get());
    }
    data.assign(buf.get(), mlen);
    return true;
  }

  bool tx_extra_message::decrypt(size_t index, const Crypto::PublicKey &txkey, const Crypto::SecretKey *recepient_secret_key, std::string &message) const
  {
    size_t mlen = data.size();
    if (mlen < TX_EXTRA_MESSAGE_CHECKSUM_SIZE)
    {
      return false;
    }
    const char *buf;
    std::unique_ptr<char[]> ptr;
    if (recepient_secret_key != nullptr)
    {
      ptr.reset(new char[mlen]);
      assert(ptr);
      message_key_data key_data;
      if (!generate_key_derivation(txkey, *recepient_secret_key, key_data.derivation))
      {
        return false;
      }
      key_data.magic1 = 0x80;
      key_data.magic2 = 0;
      Hash h = cn_fast_hash(&key_data, sizeof(message_key_data));
      uint64_t nonce = SWAP64LE(index);
      chacha8(data.data(), mlen, reinterpret_cast<uint8_t *>(&h), reinterpret_cast<uint8_t *>(&nonce), ptr.get());
      buf = ptr.get();
    }
    else
    {
      buf = data.data();
    }
    mlen -= TX_EXTRA_MESSAGE_CHECKSUM_SIZE;
    for (size_t i = 0; i < TX_EXTRA_MESSAGE_CHECKSUM_SIZE; i++)
    {
      if (buf[mlen + i] != 0)
      {
        return false;
      }
    }
    message.assign(buf, mlen);
    return true;
  }

  bool tx_extra_message::serialize(ISerializer &s)
  {
    s(data, "data");
    return true;
  }

  // HEAT commitment serialization
  bool TransactionExtraHeatCommitment::serialize(ISerializer &s)
  {
    s(commitment, "commitment");   // 🔒 SECURE: Only commitment hash
    s(amount, "amount");
    s(metadata, "metadata");
    return true;
  }

  // Yield commitment serialization
  bool TransactionExtraYieldCommitment::serialize(ISerializer &s)
  {
    s(commitment, "commitment");
    s(amount, "amount");
    s(term_months, "term_months");
    s(yield_scheme, "yield_scheme");
    s(metadata, "metadata");
    return true;
  }

  // Elderfier deposit helper functions (contingency-based)
  bool TransactionExtraElderfierDeposit::serialize(ISerializer& s)
  {
    s(depositHash, "depositHash");
    s(depositAmount, "depositAmount");
    s(elderfierAddress, "elderfierAddress");
    s(securityWindow, "securityWindow");
    s(metadata, "metadata");
    s(signature, "signature");
    s(isSlashable, "isSlashable");
    return true;
  }

  bool TransactionExtraElderfierDeposit::isValid() const
  {
    return depositAmount >= 800000000000 && // Minimum 800 XFG
           !elderfierAddress.empty() &&
           securityWindow > 0 &&
           isSlashable; // Always true for contingency deposits
  }

  std::string TransactionExtraElderfierDeposit::toString() const
  {
    std::ostringstream oss;
    oss << "ElderfierDeposit{hash=" << Common::podToHex(depositHash) 
        << ", amount=" << depositAmount 
        << ", address=" << elderfierAddress
        << ", securityWindow=" << securityWindow
        << ", slashable=" << (isSlashable ? "true" : "false") << "}";
    return oss.str();
  }

  bool createTxExtraWithElderfierDeposit(const Crypto::Hash& depositHash, uint64_t depositAmount, const std::string& elderfierAddress, uint32_t securityWindow, const std::vector<uint8_t>& metadata, std::vector<uint8_t>& extra)
  {
    TransactionExtraElderfierDeposit deposit;
    deposit.depositHash = depositHash;
    deposit.depositAmount = depositAmount;
    deposit.elderfierAddress = elderfierAddress;
    deposit.securityWindow = securityWindow;
    deposit.metadata = metadata;
    deposit.isSlashable = true; // Always true for contingency deposits
    
    return addElderfierDepositToExtra(extra, deposit);
  }

  bool addElderfierDepositToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraElderfierDeposit& deposit)
  {
    tx_extra.push_back(TX_EXTRA_ELDERFIER_DEPOSIT);
    
    // Serialize deposit hash (32 bytes)
    tx_extra.insert(tx_extra.end(), deposit.depositHash.data, deposit.depositHash.data + sizeof(deposit.depositHash.data));
    
    // Serialize amount (8 bytes, little-endian)
    uint64_t amount = deposit.depositAmount;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(amount & 0xFF));
      amount >>= 8;
    }
    
    // Serialize address length and data
    uint32_t addrLen = static_cast<uint32_t>(deposit.elderfierAddress.length());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(addrLen & 0xFF));
      addrLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), deposit.elderfierAddress.begin(), deposit.elderfierAddress.end());
    
    // Serialize security window (4 bytes, little-endian)
    uint32_t window = deposit.securityWindow;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(window & 0xFF));
      window >>= 8;
    }
    
    // Serialize metadata size and data
    uint32_t metaLen = static_cast<uint32_t>(deposit.metadata.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(metaLen & 0xFF));
      metaLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), deposit.metadata.begin(), deposit.metadata.end());
    
    // Serialize signature size and data
    uint32_t sigLen = static_cast<uint32_t>(deposit.signature.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(sigLen & 0xFF));
      sigLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), deposit.signature.begin(), deposit.signature.end());
    
    // Serialize slashable flag (1 byte)
    tx_extra.push_back(deposit.isSlashable ? 1 : 0);
    
    return true;
  }

  bool getElderfierDepositFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraElderfierDeposit& deposit)
  {
    if (tx_extra.empty() || tx_extra[0] != TX_EXTRA_ELDERFIER_DEPOSIT) {
      return false;
    }
    
    size_t pos = 1;
    
    // Deserialize deposit hash (32 bytes)
    if (pos + 32 > tx_extra.size()) return false;
    std::memcpy(deposit.depositHash.data, &tx_extra[pos], 32);
    pos += 32;
    
    // Deserialize amount (8 bytes, little-endian)
    if (pos + 8 > tx_extra.size()) return false;
    deposit.depositAmount = 0;
    for (int i = 0; i < 8; ++i) {
      deposit.depositAmount |= static_cast<uint64_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 8;
    
    // Deserialize address length and data
    if (pos + 4 > tx_extra.size()) return false;
    uint32_t addrLen = 0;
    for (int i = 0; i < 4; ++i) {
      addrLen |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;
    
    if (pos + addrLen > tx_extra.size()) return false;
    deposit.elderfierAddress.assign(reinterpret_cast<const char*>(&tx_extra[pos]), addrLen);
    pos += addrLen;
    
    // Deserialize security window (4 bytes, little-endian)
    if (pos + 4 > tx_extra.size()) return false;
    deposit.securityWindow = 0;
    for (int i = 0; i < 4; ++i) {
      deposit.securityWindow |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;
    
    // Deserialize metadata size and data
    if (pos + 4 > tx_extra.size()) return false;
    uint32_t metaLen = 0;
    for (int i = 0; i < 4; ++i) {
      metaLen |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;
    
    if (pos + metaLen > tx_extra.size()) return false;
    deposit.metadata.assign(&tx_extra[pos], &tx_extra[pos] + metaLen);
    pos += metaLen;
    
    // Deserialize signature size and data
    if (pos + 4 > tx_extra.size()) return false;
    uint32_t sigLen = 0;
    for (int i = 0; i < 4; ++i) {
      sigLen |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;
    
    if (pos + sigLen > tx_extra.size()) return false;
    deposit.signature.assign(&tx_extra[pos], &tx_extra[pos] + sigLen);
    pos += sigLen;
    
    // Deserialize slashable flag (1 byte)
    if (pos >= tx_extra.size()) return false;
    deposit.isSlashable = (tx_extra[pos] != 0);
    
    return true;
  }
  // HEAT commitment helper functions
  bool addHeatCommitmentToExtra(std::vector<uint8_t> &tx_extra, const TransactionExtraHeatCommitment &commitment)
  {
    tx_extra.push_back(TX_EXTRA_HEAT_COMMITMENT);
    
    // Serialize commitment hash (32 bytes)
    tx_extra.insert(tx_extra.end(), commitment.commitment.data, commitment.commitment.data + sizeof(commitment.commitment.data));
    
    // Serialize amount (8 bytes, little-endian)
    uint64_t amount = commitment.amount;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(amount & 0xFF));
      amount >>= 8;
    }
    
    // Serialize metadata size and data
    uint8_t metadataSize = static_cast<uint8_t>(commitment.metadata.size());
    tx_extra.push_back(metadataSize);
    
    if (metadataSize > 0) {
      tx_extra.insert(tx_extra.end(), commitment.metadata.begin(), commitment.metadata.end());
    }
    
    return true;
  }

  bool createTxExtraWithHeatCommitment(const Crypto::Hash &commitment, uint64_t amount, const std::vector<uint8_t> &metadata, std::vector<uint8_t> &extra)
  {
    TransactionExtraHeatCommitment heatCommitment;
    heatCommitment.commitment = commitment;       //  Only commitment hash
    heatCommitment.amount = amount;
    heatCommitment.metadata = metadata;
    
    return addHeatCommitmentToExtra(extra, heatCommitment);
  }

  bool getHeatCommitmentFromExtra(const std::vector<uint8_t> &tx_extra, TransactionExtraHeatCommitment &commitment)
  {
    // CODL3 implementation will parse the extra field to extract HEAT commitment
    // This is a placeholder until CODL3 merge-mining is implemented - full implementation needss parsing logic
    return false;
  }

  // Yield commitment helper functions
  bool addYieldCommitmentToExtra(std::vector<uint8_t> &tx_extra, const TransactionExtraYieldCommitment &commitment)
  {
    tx_extra.push_back(TX_EXTRA_YIELD_COMMITMENT);
    
    // Serialize commitment hash (32 bytes)
    tx_extra.insert(tx_extra.end(), commitment.commitment.data, commitment.commitment.data + sizeof(commitment.commitment.data));
    
    // Serialize amount (8 bytes, little-endian)
    uint64_t amount = commitment.amount;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(amount & 0xFF));
      amount >>= 8;
    }
    
    // Serialize term_months (4 bytes, little-endian)
    uint32_t term_months = commitment.term_months;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(term_months & 0xFF));
      term_months >>= 8;
    }
    
    // Serialize yield_scheme length and string
    uint8_t schemeLen = static_cast<uint8_t>(commitment.yield_scheme.size());
    tx_extra.push_back(schemeLen);
    tx_extra.insert(tx_extra.end(), commitment.yield_scheme.begin(), commitment.yield_scheme.end());
    
    // Serialize metadata size and data
    uint8_t metadataSize = static_cast<uint8_t>(commitment.metadata.size());
    tx_extra.push_back(metadataSize);
    
    if (metadataSize > 0) {
      tx_extra.insert(tx_extra.end(), commitment.metadata.begin(), commitment.metadata.end());
    }
    
    return true;
  }

  bool createTxExtraWithYieldCommitment(const Crypto::Hash &commitment, uint64_t amount, uint32_t term_months, const std::string &yield_scheme, const std::vector<uint8_t> &metadata, std::vector<uint8_t> &extra)
  {
    TransactionExtraYieldCommitment yieldCommitment;
    yieldCommitment.commitment = commitment;
    yieldCommitment.amount = amount;
    yieldCommitment.term_months = term_months;
    yieldCommitment.yield_scheme = yield_scheme;
    yieldCommitment.metadata = metadata;
    
    return addYieldCommitmentToExtra(extra, yieldCommitment);
  }

  bool getYieldCommitmentFromExtra(const std::vector<uint8_t> &tx_extra, TransactionExtraYieldCommitment &commitment)
  {
    // Implementation would parse the extra field to extract yield commitment
    // This is a placeholder - full implementation would need proper parsing logic
    return false;
  }

  // ---------------- HEAT wallet helpers ----------------
  // Computes Keccak256(address || "recipient") into out_hash
  bool computeHeatRecipientHash(const std::string &eth_address, Crypto::Hash &out_hash)
  {
    // Normalize and decode 0x-prefixed hex address
    std::string addr = eth_address;
    if (addr.size() >= 2 && (addr[0] == '0') && (addr[1] == 'x' || addr[1] == 'X')) {
      addr = addr.substr(2);
    }
    std::vector<uint8_t> addr_bytes;
    try {
      if (!Common::fromHex(addr, addr_bytes)) {
        return false;
      }
    } catch (...) {
      return false;
    }
    if (addr_bytes.size() != 20) {
      return false;
    }

    // Compute Keccak256(address || "recipient")
    uint8_t md[32];
    std::vector<uint8_t> preimage;
    preimage.reserve(20 + 9);
    preimage.insert(preimage.end(), addr_bytes.begin(), addr_bytes.end());
    static const char tag[] = "recipient";
    preimage.insert(preimage.end(), reinterpret_cast<const uint8_t*>(tag), reinterpret_cast<const uint8_t*>(tag) + sizeof(tag) - 1);
    keccak(preimage.data(), static_cast<int>(preimage.size()), md, sizeof(md));
    memcpy(&out_hash, md, sizeof(out_hash));
    return true;
  }

  // Computes Keccak256(secret || le64(amount) || tx_prefix_hash || recipient_hash || network_id || target_chain_id || version)
  Crypto::Hash computeHeatCommitment(const std::array<uint8_t, 32> &secret,
                                     uint64_t amount_atomic,
                                     const Crypto::Hash &tx_prefix_hash,
                                     const std::string &eth_address,
                                     uint32_t network_id,
                                     uint32_t target_chain_id,
                                     uint32_t commitment_version)
  {
    Crypto::Hash recipient_hash = {};
    if (!computeHeatRecipientHash(eth_address, recipient_hash)) {
      return Crypto::Hash{};
    }

    std::vector<uint8_t> preimage;
    preimage.reserve(32 + 8 + 32 + 32 + 4 + 4 + 4); // secret + amount + tx_prefix_hash + recipient_hash + network_id + target_chain_id + version

    // Secret (32 bytes)
    preimage.insert(preimage.end(), secret.begin(), secret.end());

    // Amount (8 bytes, LE)
    uint64_t amt = amount_atomic;
    for (int i = 0; i < 8; ++i) {
      preimage.push_back(static_cast<uint8_t>(amt & 0xFF));
      amt >>= 8;
    }

    // Tx prefix hash (32 bytes)
    preimage.insert(preimage.end(), reinterpret_cast<const uint8_t*>(&tx_prefix_hash), reinterpret_cast<const uint8_t*>(&tx_prefix_hash) + sizeof(tx_prefix_hash));

    // Recipient hash (32 bytes)
    preimage.insert(preimage.end(), reinterpret_cast<const uint8_t*>(&recipient_hash), reinterpret_cast<const uint8_t*>(&recipient_hash) + sizeof(recipient_hash));

    // Network ID (4 bytes, LE)
    uint32_t net_id = network_id;
    for (int i = 0; i < 4; ++i) {
      preimage.push_back(static_cast<uint8_t>(net_id & 0xFF));
      net_id >>= 8;
    }

    // Target chain ID (4 bytes, LE)
    uint32_t target_id = target_chain_id;
    for (int i = 0; i < 4; ++i) {
      preimage.push_back(static_cast<uint8_t>(target_id & 0xFF));
      target_id >>= 8;
    }

    // Commitment version (4 bytes, LE)
    uint32_t version = commitment_version;
    for (int i = 0; i < 4; ++i) {
      preimage.push_back(static_cast<uint8_t>(version & 0xFF));
      version >>= 8;
    }

    uint8_t md[32];
    keccak(preimage.data(), static_cast<int>(preimage.size()), md, sizeof(md));
    Crypto::Hash out{};
    memcpy(&out, md, sizeof(out));
    return out;
  }

  // Builds tx.extra with TX_EXTRA_HEAT_COMMITMENT (0x08)
  bool buildHeatExtra(const std::array<uint8_t, 32> &secret,
                      uint64_t amount_atomic,
                      const Crypto::Hash &tx_prefix_hash,
                      const std::string &eth_address,
                      uint32_t network_id,
                      uint32_t target_chain_id,
                      uint32_t commitment_version,
                      const std::vector<uint8_t> &metadata,
                      std::vector<uint8_t> &extra)
  {
    // Compute commitment with full domain separation
    Crypto::Hash commitment = computeHeatCommitment(secret, amount_atomic, tx_prefix_hash, eth_address, network_id, target_chain_id, commitment_version);

    // If commitment is zero (failed), bail
    const Crypto::Hash zero = {};
    if (!memcmp(&commitment, &zero, sizeof(zero))) {
      return false;
    }

    return createTxExtraWithHeatCommitment(commitment, amount_atomic, metadata, extra);
  }
 
  // CD Deposit Secret helper functions
  bool addCDDepositSecretToExtra(std::vector<uint8_t> &tx_extra, const TransactionExtraCDDepositSecret &deposit_secret)
  {
    tx_extra.push_back(TX_EXTRA_CD_DEPOSIT_SECRET);
    
    // Serialize secret key (32 bytes)
    if (deposit_secret.secret_key.size() != 32) {
      return false; // Invalid secret key size
    }
    tx_extra.insert(tx_extra.end(), deposit_secret.secret_key.begin(), deposit_secret.secret_key.end());
    
    // Serialize XFG amount (8 bytes, little-endian)
    uint64_t xfg_amount = deposit_secret.xfg_amount;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(xfg_amount & 0xFF));
      xfg_amount >>= 8;
    }
    
    // Serialize APR basis points (4 bytes, little-endian)
    uint32_t apr_basis_points = deposit_secret.apr_basis_points;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(apr_basis_points & 0xFF));
      apr_basis_points >>= 8;
    }
    
    // Serialize term code (1 byte)
    tx_extra.push_back(deposit_secret.term_code);
    
    // Serialize chain code (1 byte)
    tx_extra.push_back(deposit_secret.chain_code);
    
    // Serialize metadata size and data
    uint8_t metadataSize = static_cast<uint8_t>(deposit_secret.metadata.size());
    tx_extra.push_back(metadataSize);
    
    if (metadataSize > 0) {
      tx_extra.insert(tx_extra.end(), deposit_secret.metadata.begin(), deposit_secret.metadata.end());
    }
    
    return true;
  }

  bool createTxExtraWithCDDepositSecret(const std::vector<uint8_t> &secret_key, uint64_t xfg_amount, uint32_t apr_basis_points, uint8_t term_code, uint8_t chain_code, const std::vector<uint8_t> &metadata, std::vector<uint8_t> &extra)
  {
    TransactionExtraCDDepositSecret depositSecret;
    depositSecret.secret_key = secret_key;
    depositSecret.xfg_amount = xfg_amount;
    depositSecret.apr_basis_points = apr_basis_points;
    depositSecret.term_code = term_code;
    depositSecret.chain_code = chain_code;
    depositSecret.metadata = metadata;
    
    return addCDDepositSecretToExtra(extra, depositSecret);
  }

  bool getCDDepositSecretFromExtra(const std::vector<uint8_t> &tx_extra, TransactionExtraCDDepositSecret &deposit_secret)
  {
    // Implementation would parse the extra field to extract CD deposit secret
    // This is a placeholder - full implementation would need proper parsing logic
    return false;
  }

} // namespace CryptoNote
