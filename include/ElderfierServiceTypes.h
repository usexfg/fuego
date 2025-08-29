// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <string>
#include "../src/crypto/crypto.h"
#include "../src/CryptoNoteCore/CryptoNoteBasic.h"

namespace CryptoNote {

// ============================================================================
// ELDERFIER SERVICE ACCESS CONTROL CONSTANTS
// ============================================================================

// Service access control constants (NOT blockchain consensus)
#define ELDERFIER_SERVICE_MODIFIER_INTERVAL  1000   // Update every 1000 blocks
#define ELDERFIER_SERVICE_PROOF_WINDOW       3600   // 1 hour validity
#define ELDERFIER_SERVICE_PROOF_STEP         300    // 5 minutes alignment
#define ELDERFIER_STARTER_PROOF_HASH         "0000000000000000000000000000000000000000000000000000000000000000"

// Proof types for service access (NOT blockchain consensus types)
#define ELDERFIER_SERVICE_TYPE_BASIC         0x01   // Basic service access
#define ELDERFIER_SERVICE_TYPE_ENHANCED      0x02   // Enhanced service features
#define ELDERFIER_SERVICE_TYPE_DELEGATED     0x03   // Delegated service access

// Proof versions
#define ELDERFIER_PROOF_VERSION_1            0x01   // Initial version

// ============================================================================
// CORE DATA STRUCTURES FOR SERVICE ACCESS CONTROL
// ============================================================================

#pragma pack(push, 1)

/**
 * @brief Service access modifier to prevent proof grinding attacks
 * 
 * This is NOT a blockchain consensus modifier - it's only for controlling
 * access to the Elderfier service based on stake requirements.
 */
struct ElderfierServiceModifier {
    Crypto::Hash last_pow_block_hash;         // Hash of last PoW block
    uint64_t last_pow_block_height;          // Height of last PoW block
    uint64_t modifier_timestamp;              // When modifier was created
    uint64_t modifier_sequence;               // Incremental sequence number
    
    // Default constructor
    ElderfierServiceModifier() 
        : last_pow_block_hash(Crypto::NULL_HASH)
        , last_pow_block_height(0)
        , modifier_timestamp(0)
        , modifier_sequence(0) {}
    
    // Equality comparison
    bool operator==(const ElderfierServiceModifier& other) const {
        return last_pow_block_hash == other.last_pow_block_hash &&
               last_pow_block_height == other.last_pow_block_height &&
               modifier_timestamp == other.modifier_timestamp &&
               modifier_sequence == other.modifier_sequence;
    }
    
    // Inequality comparison
    bool operator!=(const ElderfierServiceModifier& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Service access proof structure
 * 
 * This proves sufficient stake exists for Elderfier service access
 * without revealing the exact balance amount.
 */
struct ElderfierServiceProof {
    // Core proof data
    Crypto::Hash proof_hash;                    // Hash of this proof
    Crypto::Hash service_modifier_hash;         // Hash of current service modifier
    Crypto::Hash fee_address_hash;              // Hash of fee address (privacy)
    uint64_t minimum_stake_atomic;             // Required stake: 800 XFG
    uint64_t proof_timestamp;                   // When proof was generated
    uint64_t proof_sequence;                    // Incremental sequence number
    
    // Cryptographic proof
    Crypto::Signature proof_signature;          // Signature proving stake
    Crypto::PublicKey proof_public_key;        // Public key for verification
    
    // Service metadata
    uint8_t proof_version;                      // Proof format version
    uint8_t service_type;                       // Service access type
    uint16_t reserved;                          // Reserved for future use
    
    // Default constructor
    ElderfierServiceProof() 
        : proof_hash(CryptoNote::NULL_HASH)
        , service_modifier_hash(CryptoNote::NULL_HASH)
        , fee_address_hash(CryptoNote::NULL_HASH)
        , minimum_stake_atomic(0)
        , proof_timestamp(0)
        , proof_sequence(0)
        , proof_signature(Crypto::NULL_SIGNATURE)
        , proof_public_key(Crypto::NULL_PUBLIC_KEY)
        , proof_version(ELDERFIER_PROOF_VERSION_1)
        , service_type(ELDERFIER_SERVICE_TYPE_BASIC)
        , reserved(0) {}
    
    // Equality comparison
    bool operator==(const ElderfierServiceProof& other) const {
        return proof_hash == other.proof_hash &&
               service_modifier_hash == other.service_modifier_hash &&
               fee_address_hash == other.fee_address_hash &&
               minimum_stake_atomic == other.minimum_stake_atomic &&
               proof_timestamp == other.proof_timestamp &&
               proof_sequence == other.proof_sequence &&
               proof_signature == other.proof_signature &&
               proof_public_key == other.proof_public_key &&
               proof_version == other.proof_version &&
               service_type == other.service_type;
    }
    
    // Inequality comparison
    bool operator!=(const ElderfierServiceProof& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Service access kernel for proof generation
 * 
 * This contains the data needed to generate a service access proof.
 */
struct ElderfierServiceKernel {
    ElderfierServiceModifier service_modifier;  // Current service modifier
    uint64_t kernel_timestamp;                  // Kernel creation timestamp
    crypto::hash fee_address_hash;              // Hash of fee address
    uint64_t minimum_stake_atomic;              // Required stake amount
    crypto::key_image stake_key_image;          // Unique key image for this stake
    
    // Default constructor
    ElderfierServiceKernel() 
        : service_modifier()
        , kernel_timestamp(0)
        , fee_address_hash(crypto::null_hash)
        , minimum_stake_atomic(0)
        , stake_key_image(crypto::null_key_image) {}
    
    // Equality comparison
    bool operator==(const ElderfierServiceKernel& other) const {
        return service_modifier == other.service_modifier &&
               kernel_timestamp == other.kernel_timestamp &&
               fee_address_hash == other.fee_address_hash &&
               minimum_stake_atomic == other.minimum_stake_atomic &&
               stake_key_image == other.stake_key_image;
    }
    
    // Inequality comparison
    bool operator!=(const ElderfierServiceKernel& other) const {
        return !(*this == other);
    }
};

#pragma pack(pop)

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

class core;
class ElderfierServiceModifierBuilder;
class ElderfierServiceKernelBuilder;
class ElderfierServiceProofGenerator;
class ElderfierServiceProofVerifier;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Convert service modifier to string for logging
 */
std::string serviceModifierToString(const ElderfierServiceModifier& modifier);

/**
 * @brief Convert service proof to string for logging
 */
std::string serviceProofToString(const ElderfierServiceProof& proof);

/**
 * @brief Convert service kernel to string for logging
 */
std::string serviceKernelToString(const ElderfierServiceKernel& kernel);

} // namespace CryptoNote
