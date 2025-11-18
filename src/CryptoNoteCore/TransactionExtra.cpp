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

        case TX_EXTRA_ELDERFIER_MESSAGE:
        {
          TransactionExtraElderfierMessage message;
          if (getElderfierMessageFromExtra(transactionExtra, message)) {
            transactionExtraFields.push_back(message);
        case TX_EXTRA_HEAT_COMMITMENT:
        {
          TransactionExtraHeatCommitment heatCommitment;
          if (getHeatCommitmentFromExtra(transactionExtra, heatCommitment)) {
            transactionExtraFields.push_back(heatCommitment);
          } else {
            return false;
          }
          break;
        }

        case TX_EXTRA_ENCRYPTED_MEDIA_MESSAGE:
        {
          TransactionExtraEncryptedMediaMessage message;
          if (getEncryptedMediaMessageFromExtra(transactionExtra, message)) {
            transactionExtraFields.push_back(message);
        case TX_EXTRA_YIELD_COMMITMENT:
        {
          TransactionExtraYieldCommitment yieldCommitment;
          if (getYieldCommitmentFromExtra(transactionExtra, yieldCommitment)) {
            transactionExtraFields.push_back(yieldCommitment);
          } else {
            return false;
          }
          break;
        }

        case TX_EXTRA_MEDIA_ATTACHMENT:
        {
          TransactionExtraMediaAttachment attachment;
          if (getMediaAttachmentFromExtra(transactionExtra, attachment)) {
            transactionExtraFields.push_back(attachment);
          } else {
            return false;
          }
          break;
        }

        case TX_EXTRA_MEDIA_TRANSFER_REQUEST:
        {
          TransactionExtraMediaTransferRequest request;
          if (getMediaTransferRequestFromExtra(transactionExtra, request)) {
            transactionExtraFields.push_back(request);
          } else {
            return false;
          }
          break;
        }

        case TX_EXTRA_MEDIA_TRANSFER_RESPONSE:
        {
          TransactionExtraMediaTransferResponse response;
          if (getMediaTransferResponseFromExtra(transactionExtra, response)) {
            transactionExtraFields.push_back(response);
        case TX_EXTRA_CD_DEPOSIT_SECRET:
        {
          TransactionExtraCDDepositSecret cdDepositSecret;
          if (getCDDepositSecretFromExtra(transactionExtra, cdDepositSecret)) {
            transactionExtraFields.push_back(cdDepositSecret);
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

    bool operator()(const TransactionExtraElderfierMessage &t)
    {
      return addElderfierMessageToExtra(extra, t);
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

    bool operator()(const TransactionExtraEncryptedMediaMessage &t)
    {
      return addEncryptedMediaMessageToExtra(extra, t);
    }

    bool operator()(const TransactionExtraMediaAttachment &t)
    {
      return addMediaAttachmentToExtra(extra, t);
    }

    bool operator()(const TransactionExtraMediaTransferRequest &t)
    {
      return addMediaTransferRequestToExtra(extra, t);
    }

    bool operator()(const TransactionExtraMediaTransferResponse &t)
    {
      return addMediaTransferResponseToExtra(extra, t);
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

  // TransactionExtraElderfierMessage methods
  bool TransactionExtraElderfierMessage::serialize(ISerializer& s)
  {
    s(senderKey, "senderKey");
    s(recipientKey, "recipientKey");
    s(messageType, "messageType");
    s(timestamp, "timestamp");
    s(messageData, "messageData");
    s(signature, "signature");

    // Consensus fields (0xEF specific)
    s(consensusRequired, "consensusRequired");

    // Handle consensus type serialization (enum to uint8_t)
    uint8_t consensusTypeValue = static_cast<uint8_t>(consensusType);
    s(consensusTypeValue, "consensusType");

    // For deserialization, restore the enum value
    // Note: This is a bit of a hack, but works for serialization
    if (consensusTypeValue <= static_cast<uint8_t>(ElderfierConsensusType::WITNESS)) {
      consensusType = static_cast<ElderfierConsensusType>(consensusTypeValue);
    }

    s(requiredThreshold, "requiredThreshold");
    s(targetDepositHash, "targetDepositHash");

    return true;
  }

  bool TransactionExtraElderfierMessage::isValid() const
  {
    // Basic validation
    if (timestamp == 0 || messageData.empty() || signature.empty() || messageType == 0) {
      return false;
    }

    // Consensus validation
    if (consensusRequired) {
      if (requiredThreshold == 0 || requiredThreshold > 100) {
        return false;
      }

      // For quorum consensus, target deposit hash must be specified (for 0xE8 intervention)
      if (consensusType == ElderfierConsensusType::QUORUM && targetDepositHash == Crypto::Hash()) {
        return false;
      }
    }

    return true;
  }

  bool TransactionExtraElderfierMessage::requiresQuorumConsensus() const
  {
    return consensusRequired && consensusType == ElderfierConsensusType::QUORUM;
  }

  std::string TransactionExtraElderfierMessage::toString() const
  {
    std::ostringstream oss;
    oss << "ElderfierMessage{sender=" << Common::podToHex(senderKey)
        << ", recipient=" << Common::podToHex(recipientKey)
        << ", type=" << messageType
        << ", timestamp=" << timestamp
        << ", dataSize=" << messageData.size()
        << ", sigSize=" << signature.size()
        << ", consensusRequired=" << (consensusRequired ? "true" : "false");

    if (consensusRequired) {
      oss << ", consensusType=";
      switch (consensusType) {
        case ElderfierConsensusType::QUORUM: oss << "QUORUM"; break;
        case ElderfierConsensusType::PROOF: oss << "PROOF"; break;
        case ElderfierConsensusType::WITNESS: oss << "WITNESS"; break;
        default: oss << "UNKNOWN"; break;
      }
      oss << ", threshold=" << requiredThreshold << "%"
          << ", targetDeposit=" << Common::podToHex(targetDepositHash);
    }

    oss << "}";
    return oss.str();
  }

  // Elderfier Message helper functions (messaging/monitoring)

  // Create Elderfier message with Quorum consensus (for 0xE8 deposit slashing)
  bool createElderfierQuorumMessage(const Crypto::PublicKey& senderKey,
                                   const Crypto::PublicKey& recipientKey,
                                   const Crypto::Hash& targetDepositHash,
                                   uint32_t messageType,
                                   const std::vector<uint8_t>& messageData,
                                   uint64_t timestamp,
                                   TransactionExtraElderfierMessage& message)
  {
    message.senderKey = senderKey;
    message.recipientKey = recipientKey;
    message.messageType = messageType;
    message.timestamp = timestamp;
    message.messageData = messageData;

    // Set quorum consensus requirements
    message.consensusRequired = true;
    message.consensusType = ElderfierConsensusType::QUORUM;
    message.requiredThreshold = 80; // >80% agreement required
    message.targetDepositHash = targetDepositHash;

    // Generate signature (placeholder - would use actual crypto)
    message.signature = std::vector<uint8_t>(64, 0xAA); // Placeholder signature

    return message.isValid();
  }

  // Create Elderfier message with Proof consensus
  bool createElderfierProofMessage(const Crypto::PublicKey& senderKey,
                                  const Crypto::PublicKey& recipientKey,
                                  uint32_t messageType,
                                  const std::vector<uint8_t>& messageData,
                                  uint64_t timestamp,
                                  TransactionExtraElderfierMessage& message)
  {
    message.senderKey = senderKey;
    message.recipientKey = recipientKey;
    message.messageType = messageType;
    message.timestamp = timestamp;
    message.messageData = messageData;

    // Set proof consensus requirements
    message.consensusRequired = true;
    message.consensusType = ElderfierConsensusType::PROOF;
    message.requiredThreshold = 100; // Proof must be cryptographically valid
    message.targetDepositHash = Crypto::Hash(); // Not targeting a deposit

    // Generate signature (placeholder)
    message.signature = std::vector<uint8_t>(64, 0xBB);

    return message.isValid();
  }

  // Create Elderfier message with Witness consensus
  bool createElderfierWitnessMessage(const Crypto::PublicKey& senderKey,
                                    const Crypto::PublicKey& recipientKey,
                                    uint32_t messageType,
                                    const std::vector<uint8_t>& messageData,
                                    uint64_t timestamp,
                                    TransactionExtraElderfierMessage& message)
  {
    message.senderKey = senderKey;
    message.recipientKey = recipientKey;
    message.messageType = messageType;
    message.timestamp = timestamp;
    message.messageData = messageData;

    // Set witness consensus requirements
    message.consensusRequired = true;
    message.consensusType = ElderfierConsensusType::WITNESS;
    message.requiredThreshold = 50; // Simple majority for witness consensus
    message.targetDepositHash = Crypto::Hash(); // Not targeting a deposit

    // Generate signature (placeholder)
    message.signature = std::vector<uint8_t>(64, 0xCC);

    return message.isValid();
  }

  bool addElderfierMessageToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraElderfierMessage& message)
  {
    tx_extra.push_back(TX_EXTRA_ELDERFIER_MESSAGE);

    // Serialize sender key (32 bytes)
    tx_extra.insert(tx_extra.end(), message.senderKey.data, message.senderKey.data + sizeof(message.senderKey.data));

    // Serialize recipient key (32 bytes)
    tx_extra.insert(tx_extra.end(), message.recipientKey.data, message.recipientKey.data + sizeof(message.recipientKey.data));

    // Serialize message type (4 bytes, little-endian)
    uint32_t msgType = message.messageType;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(msgType & 0xFF));
      msgType >>= 8;
    }

    // Serialize timestamp (8 bytes, little-endian)
    uint64_t timestamp = message.timestamp;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(timestamp & 0xFF));
      timestamp >>= 8;
    }

    // Serialize message data size and data
    uint32_t dataLen = static_cast<uint32_t>(message.messageData.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(dataLen & 0xFF));
      dataLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), message.messageData.begin(), message.messageData.end());

    // Serialize signature size and data
    uint32_t sigLen = static_cast<uint32_t>(message.signature.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(sigLen & 0xFF));
      sigLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), message.signature.begin(), message.signature.end());

    return true;
  }

  bool createTxExtraWithElderfierMessage(const Crypto::PublicKey& senderKey, const Crypto::PublicKey& recipientKey, uint32_t messageType, uint64_t timestamp, const std::vector<uint8_t>& messageData, std::vector<uint8_t>& extra)
  {
    TransactionExtraElderfierMessage message;
    message.senderKey = senderKey;
    message.recipientKey = recipientKey;
    message.messageType = messageType;
    message.timestamp = timestamp;
    message.messageData = messageData;
    // Note: signature should be added by the caller after creating the message

    return addElderfierMessageToExtra(extra, message);
  }

  bool getElderfierMessageFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraElderfierMessage& message)
  {
    if (tx_extra.empty() || tx_extra[0] != TX_EXTRA_ELDERFIER_MESSAGE) {
      return false;
    }

    size_t pos = 1;

    // Deserialize sender key (32 bytes)
    if (pos + 32 > tx_extra.size()) return false;
    std::memcpy(message.senderKey.data, &tx_extra[pos], 32);
    pos += 32;

    // Deserialize recipient key (32 bytes)
    if (pos + 32 > tx_extra.size()) return false;
    std::memcpy(message.recipientKey.data, &tx_extra[pos], 32);
    pos += 32;

    // Deserialize message type (4 bytes, little-endian)
    if (pos + 4 > tx_extra.size()) return false;
    message.messageType = 0;
    for (int i = 0; i < 4; ++i) {
      message.messageType |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;

    // Deserialize timestamp (8 bytes, little-endian)
    if (pos + 8 > tx_extra.size()) return false;
    message.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
      message.timestamp |= static_cast<uint64_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 8;

    // Deserialize message data size and data
    if (pos + 4 > tx_extra.size()) return false;
    uint32_t dataLen = 0;
    for (int i = 0; i < 4; ++i) {
      dataLen |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;

    if (pos + dataLen > tx_extra.size()) return false;
    message.messageData.assign(&tx_extra[pos], &tx_extra[pos + dataLen]);
    pos += dataLen;

    // Deserialize signature size and data
    if (pos + 4 > tx_extra.size()) return false;
    uint32_t sigLen = 0;
    for (int i = 0; i < 4; ++i) {
      sigLen |= static_cast<uint32_t>(tx_extra[pos + i]) << (i * 8);
    }
    pos += 4;

    if (pos + sigLen > tx_extra.size()) return false;
    message.signature.assign(&tx_extra[pos], &tx_extra[pos + sigLen]);

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
    // Validate term code and APR combination
    if (!validateCDTermAndAPR(deposit_secret.term_code, deposit_secret.apr_basis_points)) {
      return false; // Invalid term/APR combination
    }

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

  // CD Deposit validation helper functions
  bool validateCDTermAndAPR(uint8_t term_code, uint32_t apr_basis_points)
  {
    switch (term_code)
    {
      case CD_TERM_3MO_8PCT:
        return apr_basis_points == CD_APR_8PCT;
      case CD_TERM_9MO_18PCT:
        return apr_basis_points == CD_APR_18PCT;
      case CD_TERM_1YR_21PCT:
        return apr_basis_points == CD_APR_21PCT;
      case CD_TERM_3YR_33PCT:
        return apr_basis_points == CD_APR_33PCT;
      case CD_TERM_5YR_80PCT:
        return apr_basis_points == CD_APR_80PCT;
      default:
        return false; // Invalid term code
    }
  }

  uint64_t getCDTermDays(uint8_t term_code)
  {
    switch (term_code)
    {
      case CD_TERM_3MO_8PCT:
        return 90;    // 3 months
      case CD_TERM_9MO_18PCT:
        return 270;   // 9 months
      case CD_TERM_1YR_21PCT:
        return 365;   // 1 year
      case CD_TERM_3YR_33PCT:
        return 1095;  // 3 years
      case CD_TERM_5YR_80PCT:
        return 1825;  // 5 years
      default:
        return 0;     // Invalid
    }
  }

  double getCDAPRPercent(uint8_t term_code)
  {
    switch (term_code)
    {
      case CD_TERM_3MO_8PCT:
        return 8.0;
      case CD_TERM_9MO_18PCT:
        return 18.0;
      case CD_TERM_1YR_21PCT:
        return 21.0;
      case CD_TERM_3YR_33PCT:
        return 33.0;
      case CD_TERM_5YR_80PCT:
        return 80.0;
      default:
        return 0.0;
    }
  }

  // ============================================================================
  // ENCRYPTED MEDIA MESSAGE IMPLEMENTATIONS
  // ============================================================================

  // TransactionExtraEncryptedMediaMessage implementation
  bool TransactionExtraEncryptedMediaMessage::encrypt(const std::vector<uint8_t>& mediaData,
                                                     const AccountPublicAddress& recipient,
                                                     const KeyPair& senderKeys) {
    try {
      // Generate ECDH shared secret
      Crypto::KeyDerivation derivation;
      if (!Crypto::generate_key_derivation(recipient.viewPublicKey, senderKeys.secretKey, derivation)) {
        return false;
      }

      // Derive encryption key from shared secret
      Crypto::PublicKey ecdhKey;
      Crypto::derive_public_key(derivation, 0, recipient.spendPublicKey, ecdhKey);
      encryptionKey.assign(ecdhKey.data, ecdhKey.data + sizeof(Crypto::PublicKey));

      // Generate random nonce for AES-GCM
      encryptionNonce.resize(12);
      for (size_t i = 0; i < 12; ++i) {
        encryptionNonce[i] = static_cast<uint8_t>(Crypto::rand<uint8_t>());
      }

      // Compute SHA3-256 hash of original media content
      uint8_t hash[32];
      keccak(mediaData.data(), mediaData.size(), hash, sizeof(hash));
      mediaHash.assign(reinterpret_cast<char*>(hash), 32);

      // TODO: implement AES-256-GCM encryption
      // For now, store unencrypted data (needs proper crypto implementation)
      encryptedContent = mediaData;

      // Create signature data (all fields except signature itself)
      std::vector<uint8_t> signatureData;
      signatureData.insert(signatureData.end(), senderKey.data, senderKey.data + sizeof(Crypto::PublicKey));
      signatureData.insert(signatureData.end(), recipientKey.data, recipientKey.data + sizeof(Crypto::PublicKey));

      // Add timestamp (8 bytes, little-endian)
      for (int i = 0; i < 8; ++i) {
        signatureData.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
      }

      // Add TTL (8 bytes, little-endian)
      for (int i = 0; i < 8; ++i) {
        signatureData.push_back(static_cast<uint8_t>((ttl >> (i * 8)) & 0xFF));
      }

      // Add media type (4 bytes, little-endian)
      for (int i = 0; i < 4; ++i) {
        signatureData.push_back(static_cast<uint8_t>((mediaType >> (i * 8)) & 0xFF));
      }

      signatureData.insert(signatureData.end(), mediaHash.begin(), mediaHash.end());
      signatureData.insert(signatureData.end(), encryptedContent.begin(), encryptedContent.end());
      signatureData.insert(signatureData.end(), encryptionNonce.begin(), encryptionNonce.end());
      signatureData.insert(signatureData.end(), encryptionKey.begin(), encryptionKey.end());

      // Sign the data
      signature.resize(64); // Ed25519 signature size
      Crypto::Signature sig;
      Crypto::Hash prefixHash;
      keccak(signatureData.data(), signatureData.size(), prefixHash.data, sizeof(prefixHash));
      Crypto::generate_signature(prefixHash, senderKeys.publicKey, senderKeys.secretKey, sig);
      memcpy(signature.data(), &sig, sizeof(sig));

      return true;
    } catch (...) {
      return false;
    }
  }

  bool TransactionExtraEncryptedMediaMessage::decrypt(std::vector<uint8_t>& mediaData,
                                                     const Crypto::SecretKey& recipientPrivateKey) const {
    try {
      // Verify signature first
      if (!verifySignature()) {
        return false;
      }

      // TODO: implement ECDH key derivation and AES-256-GCM decryption
      // For now, return the encrypted content (needs crypto implementation)
      mediaData = encryptedContent;

      return true;
    } catch (...) {
      return false;
    }
  }

  bool TransactionExtraEncryptedMediaMessage::verifySignature() const {
    try {
      // Recreate signature data
      std::vector<uint8_t> signatureData;
      signatureData.insert(signatureData.end(), senderKey.data, senderKey.data + sizeof(Crypto::PublicKey));
      signatureData.insert(signatureData.end(), recipientKey.data, recipientKey.data + sizeof(Crypto::PublicKey));

      // Add timestamp (8 bytes, little-endian)
      for (int i = 0; i < 8; ++i) {
        signatureData.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
      }

      // Add TTL (8 bytes, little-endian)
      for (int i = 0; i < 8; ++i) {
        signatureData.push_back(static_cast<uint8_t>((ttl >> (i * 8)) & 0xFF));
      }

      // Add media type (4 bytes, little-endian)
      for (int i = 0; i < 4; ++i) {
        signatureData.push_back(static_cast<uint8_t>((mediaType >> (i * 8)) & 0xFF));
      }

      signatureData.insert(signatureData.end(), mediaHash.begin(), mediaHash.end());
      signatureData.insert(signatureData.end(), encryptedContent.begin(), encryptedContent.end());
      signatureData.insert(signatureData.end(), encryptionNonce.begin(), encryptionNonce.end());
      signatureData.insert(signatureData.end(), encryptionKey.begin(), encryptionKey.end());

      // Verify signature
      Crypto::Signature sig;
      if (signature.size() != sizeof(sig)) {
        return false;
      }
      memcpy(&sig, signature.data(), sizeof(sig));

      Crypto::Hash prefixHash;
      keccak(signatureData.data(), signatureData.size(), prefixHash.data, sizeof(prefixHash));
      return Crypto::check_signature(prefixHash, senderKey, sig);
    } catch (...) {
      return false;
    }
  }

  bool TransactionExtraEncryptedMediaMessage::isExpired(uint64_t currentTime) const {
    return (timestamp + ttl) < currentTime;
  }

  std::string TransactionExtraEncryptedMediaMessage::getMediaTypeString() const {
    switch (mediaType) {
      case MEDIA_TYPE_TEXT: return "text";
      case MEDIA_TYPE_IMAGE: return "image";
      case MEDIA_TYPE_VIDEO: return "video";
      case MEDIA_TYPE_AUDIO: return "audio";
      case MEDIA_TYPE_DOCUMENT: return "document";
      case MEDIA_TYPE_ARCHIVE: return "archive";
      case MEDIA_TYPE_EXECUTABLE: return "executable";
      default: return "other";
    }
  }


  bool TransactionExtraEncryptedMediaMessage::isValid() const {
    // Basic validation
    if (mediaHash.length() != 32) return false;
    if (encryptionNonce.size() != 12) return false;
    if (encryptionKey.size() != 32) return false;
    if (signature.size() != 64) return false;
    if (ttl == 0 || ttl > 365 * 24 * 60 * 60) return false; // Max 1 year TTL
    if (mediaSize > MAX_MEDIA_FILE_SIZE) return false; // Max 100MB
    if (mediaType > MEDIA_TYPE_OTHER) return false;

    return verifySignature();
  }

  // TransactionExtraMediaAttachment implementation

  bool TransactionExtraMediaAttachment::isValid() const {
    if (chunkIndex >= totalChunks) return false;
    if (chunkData.size() > MAX_MEDIA_CHUNK_SIZE) return false;
    if (chunkData.empty()) return false;
    return verifyIntegrity();
  }

  bool TransactionExtraMediaAttachment::verifyIntegrity() const {
    uint8_t hash[32];
    keccak(chunkData.data(), chunkData.size(), hash, sizeof(hash));
    Crypto::Hash computedHash;
    memcpy(&computedHash, hash, sizeof(hash));
    return chunkHash == computedHash;
  }

  // TransactionExtraMediaTransferRequest implementation

  bool TransactionExtraMediaTransferRequest::isValid() const {
    if (mediaHash == Crypto::Hash{}) return false;
    if (priority > TRANSFER_PRIORITY_CRITICAL) return false;
    if (signature.size() != 64) return false;
    return verifySignature();
  }

  bool TransactionExtraMediaTransferRequest::verifySignature() const {
    try {
      std::vector<uint8_t> signatureData;
      signatureData.insert(signatureData.end(), mediaHash.data, mediaHash.data + sizeof(Crypto::Hash));
      signatureData.insert(signatureData.end(), requesterKey.data, requesterKey.data + sizeof(Crypto::PublicKey));

      // Add timestamp (8 bytes, little-endian)
      for (int i = 0; i < 8; ++i) {
        signatureData.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
      }

      // Add priority (4 bytes, little-endian)
      for (int i = 0; i < 4; ++i) {
        signatureData.push_back(static_cast<uint8_t>((priority >> (i * 8)) & 0xFF));
      }

      Crypto::Signature sig;
      if (signature.size() != sizeof(sig)) return false;
      memcpy(&sig, signature.data(), sizeof(sig));

      Crypto::Hash prefixHash;
      keccak(signatureData.data(), signatureData.size(), prefixHash.data, sizeof(prefixHash));
      return Crypto::check_signature(prefixHash, requesterKey, sig);
    } catch (...) {
      return false;
    }
  }

  // TransactionExtraMediaTransferResponse implementation

  bool TransactionExtraMediaTransferResponse::isValid() const {
    if (mediaHash == Crypto::Hash{}) return false;
    if (responseCode > TRANSFER_RESPONSE_STORAGE_FULL) return false;
    if (responseMessage.length() > 256) return false; // Max message length
    if (signature.size() != 64) return false;
    return verifySignature();
  }

  bool TransactionExtraMediaTransferResponse::verifySignature() const {
    try {
      std::vector<uint8_t> signatureData;
      signatureData.insert(signatureData.end(), mediaHash.data, mediaHash.data + sizeof(Crypto::Hash));
      signatureData.insert(signatureData.end(), responderKey.data, responderKey.data + sizeof(Crypto::PublicKey));

      // Add timestamp (8 bytes, little-endian)
      for (int i = 0; i < 8; ++i) {
        signatureData.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
      }

      // Add response code (4 bytes, little-endian)
      for (int i = 0; i < 4; ++i) {
        signatureData.push_back(static_cast<uint8_t>((responseCode >> (i * 8)) & 0xFF));
      }

      signatureData.insert(signatureData.end(), responseMessage.begin(), responseMessage.end());

      Crypto::Signature sig;
      if (signature.size() != sizeof(sig)) return false;
      memcpy(&sig, signature.data(), sizeof(sig));

      Crypto::Hash prefixHash;
      keccak(signatureData.data(), signatureData.size(), prefixHash.data, sizeof(prefixHash));
      return Crypto::check_signature(prefixHash, responderKey, sig);
    } catch (...) {
      return false;
    }
  }

  // ============================================================================
  // HELPER FUNCTION IMPLEMENTATIONS
  // ============================================================================

  bool createTxExtraWithEncryptedMediaMessage(const Crypto::PublicKey& senderKey,
                                             const Crypto::PublicKey& recipientKey,
                                             uint64_t ttl,
                                             uint32_t mediaType,
                                             const std::vector<uint8_t>& mediaData,
                                             const AccountPublicAddress& recipientAddr,
                                             const KeyPair& senderKeys,
                                             std::vector<uint8_t>& extra) {
    try {
      TransactionExtraEncryptedMediaMessage message;
      message.senderKey = senderKey;
      message.recipientKey = recipientKey;
      message.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
      message.ttl = ttl;
      message.mediaType = mediaType;
      message.mediaSize = mediaData.size();

      if (!message.encrypt(mediaData, recipientAddr, senderKeys)) {
        return false;
      }

      return addEncryptedMediaMessageToExtra(extra, message);
    } catch (...) {
      return false;
    }
  }

  bool addEncryptedMediaMessageToExtra(std::vector<uint8_t>& tx_extra,
                                      const TransactionExtraEncryptedMediaMessage& message) {
    if (!message.isValid()) return false;

    tx_extra.push_back(TX_EXTRA_ENCRYPTED_MEDIA_MESSAGE);

    // Serialize sender key (32 bytes)
    tx_extra.insert(tx_extra.end(), message.senderKey.data, message.senderKey.data + sizeof(message.senderKey.data));

    // Serialize recipient key (32 bytes)
    tx_extra.insert(tx_extra.end(), message.recipientKey.data, message.recipientKey.data + sizeof(message.recipientKey.data));

    // Serialize timestamp (8 bytes, little-endian)
    uint64_t timestamp = message.timestamp;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(timestamp & 0xFF));
      timestamp >>= 8;
    }

    // Serialize TTL (8 bytes, little-endian)
    uint64_t ttl = message.ttl;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(ttl & 0xFF));
      ttl >>= 8;
    }

    // Serialize media type (4 bytes, little-endian)
    uint32_t mediaType = message.mediaType;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(mediaType & 0xFF));
      mediaType >>= 8;
    }

    // Serialize media hash (32 bytes)
    tx_extra.insert(tx_extra.end(), message.mediaHash.begin(), message.mediaHash.end());

    // Serialize media size (8 bytes, little-endian)
    uint64_t mediaSize = message.mediaSize;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(mediaSize & 0xFF));
      mediaSize >>= 8;
    }

    // Serialize encrypted content size and data
    uint32_t contentLen = static_cast<uint32_t>(message.encryptedContent.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(contentLen & 0xFF));
      contentLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), message.encryptedContent.begin(), message.encryptedContent.end());

    // Serialize encryption nonce (12 bytes)
    tx_extra.insert(tx_extra.end(), message.encryptionNonce.begin(), message.encryptionNonce.end());

    // Serialize encryption key (32 bytes)
    tx_extra.insert(tx_extra.end(), message.encryptionKey.begin(), message.encryptionKey.end());

    // Serialize signature (64 bytes)
    tx_extra.insert(tx_extra.end(), message.signature.begin(), message.signature.end());

    return true;
  }

  bool getEncryptedMediaMessageFromExtra(const std::vector<uint8_t>& tx_extra,
                                        TransactionExtraEncryptedMediaMessage& message) {
    // implementation would parse the extra field
    // placeholder - full implementation needs proper parsing logic
    return false;
  }

  // Media Attachment helper functions
  bool createTxExtraWithMediaAttachment(const Crypto::Hash& messageId,
                                       uint32_t chunkIndex,
                                       uint32_t totalChunks,
                                       const std::vector<uint8_t>& chunkData,
                                       std::vector<uint8_t>& extra) {
    TransactionExtraMediaAttachment attachment;
    attachment.messageId = messageId;
    attachment.chunkIndex = chunkIndex;
    attachment.totalChunks = totalChunks;
    attachment.chunkData = chunkData;

    // Compute chunk hash
    uint8_t hash[32];
    keccak(chunkData.data(), chunkData.size(), hash, sizeof(hash));
    memcpy(&attachment.chunkHash, hash, sizeof(hash));

    return addMediaAttachmentToExtra(extra, attachment);
  }

  bool addMediaAttachmentToExtra(std::vector<uint8_t>& tx_extra,
                                const TransactionExtraMediaAttachment& attachment) {
    if (!attachment.isValid()) return false;

    tx_extra.push_back(TX_EXTRA_MEDIA_ATTACHMENT);

    // Serialize message ID (32 bytes)
    tx_extra.insert(tx_extra.end(), attachment.messageId.data, attachment.messageId.data + sizeof(attachment.messageId.data));

    // Serialize chunk index (4 bytes, little-endian)
    uint32_t chunkIndex = attachment.chunkIndex;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(chunkIndex & 0xFF));
      chunkIndex >>= 8;
    }

    // Serialize total chunks (4 bytes, little-endian)
    uint32_t totalChunks = attachment.totalChunks;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(totalChunks & 0xFF));
      totalChunks >>= 8;
    }

    // Serialize chunk data size and data
    uint32_t dataLen = static_cast<uint32_t>(attachment.chunkData.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(dataLen & 0xFF));
      dataLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), attachment.chunkData.begin(), attachment.chunkData.end());

    // Serialize chunk hash (32 bytes)
    tx_extra.insert(tx_extra.end(), attachment.chunkHash.data, attachment.chunkHash.data + sizeof(attachment.chunkHash.data));

    return true;
  }

  bool getMediaAttachmentFromExtra(const std::vector<uint8_t>& tx_extra,
                                  TransactionExtraMediaAttachment& attachment) {
    // implementation would parse the extra field
    // This is a placeholder - full implementation needs proper parsing logic
    return false;
  }

  // Media Transfer Request/Response helper functions
  bool createTxExtraWithMediaTransferRequest(const Crypto::Hash& mediaHash,
                                            const Crypto::PublicKey& requesterKey,
                                            uint32_t priority,
                                            const Crypto::SecretKey& requesterSecretKey,
                                            std::vector<uint8_t>& extra) {
    TransactionExtraMediaTransferRequest request;
    request.mediaHash = mediaHash;
    request.requesterKey = requesterKey;
    request.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
    request.priority = priority;

    // Create signature data
    std::vector<uint8_t> signatureData;
    signatureData.insert(signatureData.end(), mediaHash.data, mediaHash.data + sizeof(Crypto::Hash));
    signatureData.insert(signatureData.end(), requesterKey.data, requesterKey.data + sizeof(Crypto::PublicKey));

    for (int i = 0; i < 8; ++i) {
      signatureData.push_back(static_cast<uint8_t>((request.timestamp >> (i * 8)) & 0xFF));
    }

    for (int i = 0; i < 4; ++i) {
      signatureData.push_back(static_cast<uint8_t>((priority >> (i * 8)) & 0xFF));
    }

    // Sign the data
    request.signature.resize(64);
    Crypto::Signature sig;
    Crypto::Hash prefixHash;
    keccak(signatureData.data(), signatureData.size(), prefixHash.data, sizeof(prefixHash));
    Crypto::generate_signature(prefixHash, requesterKey, requesterSecretKey, sig);
    memcpy(request.signature.data(), &sig, sizeof(sig));

    return addMediaTransferRequestToExtra(extra, request);
  }

  bool addMediaTransferRequestToExtra(std::vector<uint8_t>& tx_extra,
                                     const TransactionExtraMediaTransferRequest& request) {
    if (!request.isValid()) return false;

    tx_extra.push_back(TX_EXTRA_MEDIA_TRANSFER_REQUEST);

    // Serialize media hash (32 bytes)
    tx_extra.insert(tx_extra.end(), request.mediaHash.data, request.mediaHash.data + sizeof(request.mediaHash.data));

    // Serialize requester key (32 bytes)
    tx_extra.insert(tx_extra.end(), request.requesterKey.data, request.requesterKey.data + sizeof(request.requesterKey.data));

    // Serialize timestamp (8 bytes, little-endian)
    uint64_t timestamp = request.timestamp;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(timestamp & 0xFF));
      timestamp >>= 8;
    }

    // Serialize priority (4 bytes, little-endian)
    uint32_t priority = request.priority;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(priority & 0xFF));
      priority >>= 8;
    }

    // Serialize signature (64 bytes)
    tx_extra.insert(tx_extra.end(), request.signature.begin(), request.signature.end());

    return true;
  }

  bool getMediaTransferRequestFromExtra(const std::vector<uint8_t>& tx_extra,
                                       TransactionExtraMediaTransferRequest& request) {
    // Implementation would parse the extra field
    return false;
  }

  bool createTxExtraWithMediaTransferResponse(const Crypto::Hash& mediaHash,
                                             const Crypto::PublicKey& responderKey,
                                             uint32_t responseCode,
                                             const std::string& responseMessage,
                                             const Crypto::SecretKey& responderSecretKey,
                                             std::vector<uint8_t>& extra) {
    TransactionExtraMediaTransferResponse response;
    response.mediaHash = mediaHash;
    response.responderKey = responderKey;
    response.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
    response.responseCode = responseCode;
    response.responseMessage = responseMessage;

    // Create signature data
    std::vector<uint8_t> signatureData;
    signatureData.insert(signatureData.end(), mediaHash.data, mediaHash.data + sizeof(Crypto::Hash));
    signatureData.insert(signatureData.end(), responderKey.data, responderKey.data + sizeof(Crypto::PublicKey));

    for (int i = 0; i < 8; ++i) {
      signatureData.push_back(static_cast<uint8_t>((response.timestamp >> (i * 8)) & 0xFF));
    }

    for (int i = 0; i < 4; ++i) {
      signatureData.push_back(static_cast<uint8_t>((responseCode >> (i * 8)) & 0xFF));
    }

    signatureData.insert(signatureData.end(), responseMessage.begin(), responseMessage.end());

    // Sign the data
    response.signature.resize(64);
    Crypto::Signature sig;
    Crypto::Hash prefixHash;
    keccak(signatureData.data(), signatureData.size(), prefixHash.data, sizeof(prefixHash));
    Crypto::generate_signature(prefixHash, responderKey, responderSecretKey, sig);
    memcpy(response.signature.data(), &sig, sizeof(sig));

    return addMediaTransferResponseToExtra(extra, response);
  }

  bool addMediaTransferResponseToExtra(std::vector<uint8_t>& tx_extra,
                                      const TransactionExtraMediaTransferResponse& response) {
    if (!response.isValid()) return false;

    tx_extra.push_back(TX_EXTRA_MEDIA_TRANSFER_RESPONSE);

    // Serialize media hash (32 bytes)
    tx_extra.insert(tx_extra.end(), response.mediaHash.data, response.mediaHash.data + sizeof(response.mediaHash.data));

    // Serialize responder key (32 bytes)
    tx_extra.insert(tx_extra.end(), response.responderKey.data, response.responderKey.data + sizeof(response.responderKey.data));

    // Serialize timestamp (8 bytes, little-endian)
    uint64_t timestamp = response.timestamp;
    for (int i = 0; i < 8; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(timestamp & 0xFF));
      timestamp >>= 8;
    }

    // Serialize response code (4 bytes, little-endian)
    uint32_t responseCode = response.responseCode;
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(responseCode & 0xFF));
      responseCode >>= 8;
    }

    // Serialize response message size and data
    uint32_t msgLen = static_cast<uint32_t>(response.responseMessage.size());
    for (int i = 0; i < 4; ++i) {
      tx_extra.push_back(static_cast<uint8_t>(msgLen & 0xFF));
      msgLen >>= 8;
    }
    tx_extra.insert(tx_extra.end(), response.responseMessage.begin(), response.responseMessage.end());

    // Serialize signature (64 bytes)
    tx_extra.insert(tx_extra.end(), response.signature.begin(), response.signature.end());

    return true;
  }

  bool getMediaTransferResponseFromExtra(const std::vector<uint8_t>& tx_extra,
                                        TransactionExtraMediaTransferResponse& response) {
    // Implementation would parse the extra field
    return false;
  }


} // namespace CryptoNote
