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
#include "../include/EldernodeIndexTypes.h"

#define TX_EXTRA_PADDING_MAX_COUNT          255
#define TX_EXTRA_NONCE_MAX_COUNT            255

// Transaction Extra Tag Categories:
//
// 0x_0 tags: Core system tags
#define TX_EXTRA_TAG_PADDING                0x00
#define TX_EXTRA_TAG_PUBKEY                 0x01
#define TX_EXTRA_NONCE                      0x02
#define TX_EXTRA_MERGE_MINING_TAG           0x03
#define TX_EXTRA_MESSAGE_TAG                0x04
#define TX_EXTRA_TTL                        0x05

// 0x_8 tags: Burn-related deposit types
#define TX_EXTRA_HEAT_COMMITMENT            0x08  // Heat commitment (burn)
#define TX_EXTRA_ELDERFIER_DEPOSIT          0xE8  // Elderfier staking (moved from 0x06)
#define TX_EXTRA_DIGM_MINT                  0xA8  // DIGM coin mint (Split 3 ways treasury, dev, burn)

// 0x_A tags: DIGM Artist related meta/msgs/txns
#define TX_EXTRA_DIGM_ALBUM                 0x0A  // Album metadata
// Future: 0x1A, 0x2A, 0x3A, etc.

// 0x_B tags: DIGM Listener related txns
#define TX_EXTRA_DIGM_LISTEN_RIGHTS         0x0B  // Listen rights purchase
// Future: 0x1B, 0x2B, 0x3B, etc.

// 0x_C tags: DIGM Curator related txns
#define TX_EXTRA_DIGM_CURATOR               0x0C  // Curator operations
#define TX_EXTRA_DIGM_CURATOR_COIN          0x1C  // CURA coin operations

// 0xCD tags: COLD (CD) yield deposits
#define TX_EXTRA_CD_DEPOSIT_SECRET          0xCD  // COLD yield deposits (moved from 0x09)

/*
 * COLD DEPOSIT (CD) YIELD SYSTEM
 * ===============================
 *
 * Fixed term deposit options with guaranteed APR rates:
 *
 * Term Code | Term Length | APR Rate | Days | Basis Points
 * ----------|-------------|----------|------|-------------
 *    1      |   3 months  |   8%     |  90  |    800
 *    2      |   9 months  |  18%     | 270  |   1800
 *    3      |   1 year    |  21%     | 365  |   2100
 *    4      |   3 years   |  33%     | 1095 |   3300
 *    5      |   5 years   |  80%     | 1825 |   8000
 *
 * Deposits are locked for the full term and earn compound interest.
 * Early withdrawal is not permitted - funds remain locked until maturity.
 *
 * Usage Example:
 *   createTxExtraWithCDDepositSecret(secret_key, amount, CD_APR_21PCT, CD_TERM_1YR_21PCT, chain_code, metadata, extra);
 */

// 0x_E tags: Elderfier system (consensus/messaging)
#define TX_EXTRA_ELDERFIER_MESSAGE          0xEF  // Elderfier messaging/consensus

// 0x_F tags: Encrypted P2P Media Messages (ephemeral content)
#define TX_EXTRA_ENCRYPTED_MEDIA_MESSAGE    0xF0  // Encrypted media message with TTL
#define TX_EXTRA_MEDIA_ATTACHMENT           0xF1  // Media attachment chunk
#define TX_EXTRA_MEDIA_TRANSFER_REQUEST     0xF2  // Request for media transfer
#define TX_EXTRA_MEDIA_TRANSFER_RESPONSE    0xF3  // Response to media transfer

/*
 * ============================================================================
 * ENCRYPTED P2P MEDIA MESSAGING SYSTEM - DESIGN OVERVIEW
 * ============================================================================
 *
 * This system enables encrypted, ephemeral media sharing in the Fuego blockchain
 * with automatic content cleanup to prevent permanent storage on nodes.
 *
 * KEY FEATURES:
 * - End-to-end encryption using ECDH + AES-256-GCM
 * - Configurable TTL (Time To Live) with automatic cleanup
 * - Support for video, audio, images, documents (up to 100MB)
 * - Chunked transfer for large files
 * - Transfer request/response protocol
 * - Cryptographic signatures for authenticity
 *
 * ARCHITECTURE:
 *
 * 1. ENCRYPTED MEDIA MESSAGE (0xF0):
 *    - Contains encrypted media data or reference
 *    - Includes TTL for automatic cleanup
 *    - Signed by sender for authenticity
 *    - Can be inline (<64KB) or chunked (>64KB)
 *
 * 2. MEDIA ATTACHMENT (0xF1):
 *    - Individual chunks for large files
 *    - SHA3-256 integrity verification
 *    - Reference to parent message
 *
 * 3. TRANSFER REQUEST/RESPONSE (0xF2/0xF3):
 *    - Request media from other nodes
 *    - Response with availability status
 *    - Signed for authenticity
 *
 * USAGE WORKFLOW:
 *
 * 1. Sender encrypts media with recipient's public key
 * 2. Creates TX_EXTRA_ENCRYPTED_MEDIA_MESSAGE transaction
 * 3. If media > 64KB, creates additional TX_EXTRA_MEDIA_ATTACHMENT chunks
 * 4. Recipient receives and decrypts using private key
 * 5. After TTL expires, nodes automatically delete content
 *
 * TRANSFER PROTOCOL:
 *
 * 1. Node A sends TRANSFER_REQUEST for media hash
 * 2. Node B responds with TRANSFER_RESPONSE (accepted/rejected)
 * 3. If accepted, Node B sends media via new transaction(s)
 * 4. Node A receives and validates media integrity
 *
 * SECURITY:
 * - ECDH key exchange for symmetric encryption
 * - AES-256-GCM authenticated encryption
 * - SHA3-256 content hashing
 * - Ed25519 digital signatures
 * - TTL enforcement prevents permanent storage
 *
 * EXAMPLE USAGE:
 *
 * // Create encrypted video message
 * std::vector<uint8_t> videoData = loadVideoFile("myvideo.mp4");
 * std::vector<uint8_t> extra;
 *
 * bool success = createTxExtraWithEncryptedMediaMessage(
 *     senderPublicKey, recipientPublicKey, 86400, // 24 hour TTL
 *     MEDIA_TYPE_VIDEO, videoData, recipientAddress, senderKeys, extra
 * );
 *
 * ============================================================================
 */

// Legacy/compatibility (deprecated)
#define TX_EXTRA_YIELD_COMMITMENT           0x07  // Legacy yield commitment

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

struct TransactionExtraElderfierMessage {
  Crypto::PublicKey senderKey;         // Elderfier node public key
  Crypto::PublicKey recipientKey;      // Target Elderfier node public key (or broadcast)
  uint32_t messageType;                // Message type (consensus, slashing, monitoring, etc.)
  uint64_t timestamp;                  // Message timestamp
  std::vector<uint8_t> messageData;    // Encrypted message payload
  std::vector<uint8_t> signature;      // Message signature

  // Consensus requirements (for 0xEF transactions)
  bool consensusRequired;              // Whether this message requires consensus validation
  ElderfierConsensusType consensusType; // Type of consensus required (QUORUM, PROOF, WITNESS)
  uint32_t requiredThreshold;          // Threshold required (e.g., 80 for quorum)
  Crypto::Hash targetDepositHash;      // Target 0xE8 deposit hash (for slashing messages)

  bool serialize(ISerializer& serializer);
  bool isValid() const;
  bool requiresQuorumConsensus() const; // Check if this message requires quorum
  std::string toString() const;
};

// DIGM transaction extra structures will be implemented later
// Reserved tags: 0x0A (Album), 0x0B (Listen Rights), 0x0C (Curator), 0x1C (CURA Coin), 0xA8 (DIGM Mint)

struct TransactionExtraCDDepositSecret {
  std::vector<uint8_t> secret_key;  // 32-byte deposit secret key
  uint64_t xfg_amount;              // XFG amount for CD conversion
  uint32_t apr_basis_points;        // APR in basis points (800=8%, 1800=18%, etc.)
  uint8_t term_code;                // CD term code (1=3mo/8%, 2=9mo/18%, 3=1yr/21%, 4=3yr/33%, 5=5yr/80%)
  uint8_t chain_code;               // Chain code (1=testnet, 2=mainnet)
  std::vector<uint8_t> metadata;    // Additional metadata

  bool serialize(ISerializer& serializer);
};

// Encrypted P2P Media Message structures
struct TransactionExtraEncryptedMediaMessage {
  Crypto::PublicKey senderKey;        // Sender's public key
  Crypto::PublicKey recipientKey;     // Recipient's public key
  uint64_t timestamp;                 // Message creation timestamp
  uint64_t ttl;                       // Time to live in seconds from timestamp
  uint32_t mediaType;                 // 0=text, 1=image, 2=video, 3=audio, 4=document, etc.
  std::string mediaHash;              // SHA3-256 hash of original media content
  uint64_t mediaSize;                 // Size of media content in bytes
  std::vector<uint8_t> encryptedContent; // AES-256-GCM encrypted media data
  std::vector<uint8_t> encryptionNonce;   // 12-byte nonce for AES-GCM
  std::vector<uint8_t> encryptionKey;     // ECDH-derived key (encrypted with recipient's pubkey)
  std::vector<uint8_t> signature;         // Ed25519 signature of the entire message

  bool encrypt(const std::vector<uint8_t>& mediaData, const AccountPublicAddress& recipient, const KeyPair& senderKeys);
  bool decrypt(std::vector<uint8_t>& mediaData, const Crypto::SecretKey& recipientPrivateKey) const;
  bool verifySignature() const;
  bool isExpired(uint64_t currentTime) const;
  std::string getMediaTypeString() const;
  bool isValid() const;
};

struct TransactionExtraMediaAttachment {
  Crypto::Hash messageId;     // Reference to the main media message
  uint32_t chunkIndex;        // Index of this chunk (0-based)
  uint32_t totalChunks;       // Total number of chunks
  std::vector<uint8_t> chunkData;  // Encrypted chunk data
  Crypto::Hash chunkHash;     // SHA3-256 of chunkData for integrity

  bool isValid() const;
  bool verifyIntegrity() const;
};

struct TransactionExtraMediaTransferRequest {
  Crypto::Hash mediaHash;          // Hash of the media content requested
  Crypto::PublicKey requesterKey;  // Public key of requester
  uint64_t timestamp;              // Request timestamp
  uint32_t priority;               // Transfer priority (0=low, 1=normal, 2=high, 3=critical)
  std::vector<uint8_t> signature;  // Signature by requester

  bool isValid() const;
  bool verifySignature() const;
};

struct TransactionExtraMediaTransferResponse {
  Crypto::Hash mediaHash;          // Hash of the media content
  Crypto::PublicKey responderKey;  // Public key of responder
  uint64_t timestamp;              // Response timestamp
  uint32_t responseCode;           // 0=accepted, 1=rejected, 2=not_found, 3=rate_limited
  std::string responseMessage;     // Optional response message
  std::vector<uint8_t> signature;  // Signature by responder

  bool isValid() const;
  bool verifySignature() const;
};

// Removed duplicate TransactionExtraElderfierDeposit struct
// tx_extra_field format, except tx_extra_padding and tx_extra_pub_key:
//   varint tag;
//   varint size;
//   varint data[];
typedef boost::variant<CryptoNote::TransactionExtraPadding, CryptoNote::TransactionExtraPublicKey, CryptoNote::TransactionExtraNonce, CryptoNote::TransactionExtraMergeMiningTag, CryptoNote::tx_extra_message, CryptoNote::TransactionExtraTTL, CryptoNote::TransactionExtraElderfierDeposit, CryptoNote::TransactionExtraElderfierMessage, CryptoNote::TransactionExtraHeatCommitment, CryptoNote::TransactionExtraYieldCommitment, CryptoNote::TransactionExtraCDDepositSecret, CryptoNote::TransactionExtraEncryptedMediaMessage, CryptoNote::TransactionExtraMediaAttachment, CryptoNote::TransactionExtraMediaTransferRequest, CryptoNote::TransactionExtraMediaTransferResponse> TransactionExtraField;



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

// Elderfier Message helper functions (messaging/monitoring)
bool createTxExtraWithElderfierMessage(const Crypto::PublicKey& senderKey, const Crypto::PublicKey& recipientKey, uint32_t messageType, uint64_t timestamp, const std::vector<uint8_t>& messageData, std::vector<uint8_t>& extra);
bool addElderfierMessageToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraElderfierMessage& message);
bool getElderfierMessageFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraElderfierMessage& message);

// Consensus-specific message creation functions
bool createElderfierQuorumMessage(const Crypto::PublicKey& senderKey, const Crypto::PublicKey& recipientKey, const Crypto::Hash& targetDepositHash, uint32_t messageType, const std::vector<uint8_t>& messageData, uint64_t timestamp, TransactionExtraElderfierMessage& message);
bool createElderfierProofMessage(const Crypto::PublicKey& senderKey, const Crypto::PublicKey& recipientKey, uint32_t messageType, const std::vector<uint8_t>& messageData, uint64_t timestamp, TransactionExtraElderfierMessage& message);
bool createElderfierWitnessMessage(const Crypto::PublicKey& senderKey, const Crypto::PublicKey& recipientKey, uint32_t messageType, const std::vector<uint8_t>& messageData, uint64_t timestamp, TransactionExtraElderfierMessage& message);

// DIGM helper functions will be implemented later

// CD Deposit Secret helper functions
bool createTxExtraWithCDDepositSecret(const std::vector<uint8_t>& secret_key, uint64_t xfg_amount, uint32_t apr_basis_points, uint8_t term_code, uint8_t chain_code, const std::vector<uint8_t>& metadata, std::vector<uint8_t>& extra);
bool addCDDepositSecretToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraCDDepositSecret& deposit_secret);
bool getCDDepositSecretFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraCDDepositSecret& deposit_secret);

// CD Deposit validation and utility functions
bool validateCDTermAndAPR(uint8_t term_code, uint32_t apr_basis_points);
uint64_t getCDTermDays(uint8_t term_code);
double getCDAPRPercent(uint8_t term_code);

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

// Encrypted Media Message helper functions
bool createTxExtraWithEncryptedMediaMessage(const Crypto::PublicKey& senderKey,
                                           const Crypto::PublicKey& recipientKey,
                                           uint64_t ttl,
                                           uint32_t mediaType,
                                           const std::vector<uint8_t>& mediaData,
                                           const AccountPublicAddress& recipientAddr,
                                           const KeyPair& senderKeys,
                                           std::vector<uint8_t>& extra);
bool addEncryptedMediaMessageToExtra(std::vector<uint8_t>& tx_extra,
                                    const TransactionExtraEncryptedMediaMessage& message);
bool getEncryptedMediaMessageFromExtra(const std::vector<uint8_t>& tx_extra,
                                      TransactionExtraEncryptedMediaMessage& message);

// Media Attachment helper functions
bool createTxExtraWithMediaAttachment(const Crypto::Hash& messageId,
                                     uint32_t chunkIndex,
                                     uint32_t totalChunks,
                                     const std::vector<uint8_t>& chunkData,
                                     std::vector<uint8_t>& extra);
bool addMediaAttachmentToExtra(std::vector<uint8_t>& tx_extra,
                              const TransactionExtraMediaAttachment& attachment);
bool getMediaAttachmentFromExtra(const std::vector<uint8_t>& tx_extra,
                                TransactionExtraMediaAttachment& attachment);

// Media Transfer Request/Response helper functions
bool createTxExtraWithMediaTransferRequest(const Crypto::Hash& mediaHash,
                                          const Crypto::PublicKey& requesterKey,
                                          uint32_t priority,
                                          const Crypto::SecretKey& requesterSecretKey,
                                          std::vector<uint8_t>& extra);
bool addMediaTransferRequestToExtra(std::vector<uint8_t>& tx_extra,
                                   const TransactionExtraMediaTransferRequest& request);
bool getMediaTransferRequestFromExtra(const std::vector<uint8_t>& tx_extra,
                                     TransactionExtraMediaTransferRequest& request);

bool createTxExtraWithMediaTransferResponse(const Crypto::Hash& mediaHash,
                                           const Crypto::PublicKey& responderKey,
                                           uint32_t responseCode,
                                           const std::string& responseMessage,
                                           const Crypto::SecretKey& responderSecretKey,
                                           std::vector<uint8_t>& extra);
bool addMediaTransferResponseToExtra(std::vector<uint8_t>& tx_extra,
                                    const TransactionExtraMediaTransferResponse& response);
bool getMediaTransferResponseFromExtra(const std::vector<uint8_t>& tx_extra,
                                      TransactionExtraMediaTransferResponse& response);

// Media type constants
enum MediaType {
  MEDIA_TYPE_TEXT = 0,
  MEDIA_TYPE_IMAGE = 1,
  MEDIA_TYPE_VIDEO = 2,
  MEDIA_TYPE_AUDIO = 3,
  MEDIA_TYPE_DOCUMENT = 4,
  MEDIA_TYPE_ARCHIVE = 5,
  MEDIA_TYPE_EXECUTABLE = 6,
  MEDIA_TYPE_OTHER = 255
};

// Transfer priority constants
enum TransferPriority {
  TRANSFER_PRIORITY_LOW = 0,
  TRANSFER_PRIORITY_NORMAL = 1,
  TRANSFER_PRIORITY_HIGH = 2,
  TRANSFER_PRIORITY_CRITICAL = 3
};

// Transfer response codes
enum TransferResponseCode {
  TRANSFER_RESPONSE_ACCEPTED = 0,
  TRANSFER_RESPONSE_REJECTED = 1,
  TRANSFER_RESPONSE_NOT_FOUND = 2,
  TRANSFER_RESPONSE_RATE_LIMITED = 3,
  TRANSFER_RESPONSE_BUSY = 4,
  TRANSFER_RESPONSE_STORAGE_FULL = 5
};

// Cold Deposit (CD) term codes and APR rates
enum CDTermCode {
  CD_TERM_3MO_8PCT = 1,      // 3 months / 8% APR (90 days)
  CD_TERM_9MO_18PCT = 2,     // 9 months / 18% APR (270 days)
  CD_TERM_1YR_21PCT = 3,     // 1 year / 21% APR (365 days)
  CD_TERM_3YR_33PCT = 4,     // 3 years / 33% APR (1095 days)
  CD_TERM_5YR_80PCT = 5      // 5 years / 80% APR (1825 days)
};

// Cold Deposit APR rates in basis points (1% = 100 basis points)
enum CDAPRRate {
  CD_APR_8PCT = 800,         // 8% APR = 800 basis points
  CD_APR_18PCT = 1800,       // 18% APR = 1800 basis points
  CD_APR_21PCT = 2100,       // 21% APR = 2100 basis points
  CD_APR_33PCT = 3300,       // 33% APR = 3300 basis points
  CD_APR_80PCT = 8000        // 80% APR = 8000 basis points
};

// Media chunk size constants (for splitting large files)
const size_t MAX_MEDIA_CHUNK_SIZE = 1024 * 1024;     // 1MB per chunk
const size_t MAX_MEDIA_INLINE_SIZE = 64 * 1024;      // 64KB for inline storage
const uint64_t MAX_MEDIA_FILE_SIZE = 100 * 1024 * 1024; // 100MB max file size
const uint64_t DEFAULT_MEDIA_TTL = 24 * 60 * 60;     // 24 hours default TTL

}
