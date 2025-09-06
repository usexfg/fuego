#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include "crypto/hash.h"
#include "crypto/crypto.h"

namespace CryptoNote {

// Service ID types for Elderfier nodes
enum class ServiceIdType : uint8_t {
    STANDARD_ADDRESS = 0,    // Standard fee address (like basic Eldernodes)
    CUSTOM_NAME = 1,         // Custom name (exactly 8 letters, all caps) - links to hashed address
    HASHED_ADDRESS = 2       // Hashed public fee address (for privacy)
};

// Service ID structure for Elderfier nodes
struct ElderfierServiceId {
    ServiceIdType type;
    std::string identifier;  // Raw identifier (address, name, or hash)
    std::string displayName; // Human-readable display name
    std::string linkedAddress; // Actual wallet address
    std::string hashedAddress; // SHA256 hash of the wallet address (for all Elderfier nodes)
    
    bool isValid() const;
    std::string toString() const;
    static ElderfierServiceId createStandardAddress(const std::string& address);
    static ElderfierServiceId createCustomName(const std::string& name, const std::string& walletAddress);
    static ElderfierServiceId createHashedAddress(const std::string& address);
};

// Eldernode tier levels
enum class EldernodeTier : uint8_t {
    BASIC = 0,           // Basic Eldernode (no stake required)
    ELDERFIER = 1        // Elderfier service node (800 XFG stake required)
};

// Constant stake proof types for cross-chain validation
enum class ConstantStakeProofType : uint8_t {
    NONE = 0,                    // No constant stake proof
    ELDERADO_C0DL3_VALIDATOR = 1 // Elderado validator stake for C0DL3 (zkSync) - 8000 XFG
};

// Eldernode stake proof structure
struct EldernodeStakeProof {
    Crypto::Hash stakeHash;
    Crypto::PublicKey eldernodePublicKey;
    uint64_t stakeAmount;
    uint64_t timestamp;
    std::vector<uint8_t> proofSignature;
    std::string feeAddress;
    EldernodeTier tier;
    ElderfierServiceId serviceId;  // Only used for ELDERFIER tier
    ConstantStakeProofType constantProofType; // Constant stake proof type for cross-chain validation
    std::string crossChainAddress; // Address on target chain (e.g., C0DL3/zkSync)
    uint64_t constantStakeAmount;  // Amount locked for constant proof (e.g., 8000 XFG for Elderado)
    uint64_t constantProofExpiry;  // Expiry timestamp for constant proof (0 = never expires)
    
    bool isValid() const;
    std::string toString() const;
    bool isConstantProof() const;
    bool isConstantProofExpired() const;
};

// Eldernode consensus participant
struct EldernodeConsensusParticipant {
    Crypto::PublicKey publicKey;
    std::string address;
    uint64_t stakeAmount;
    bool isActive;
    std::chrono::system_clock::time_point lastSeen;
    EldernodeTier tier;
    ElderfierServiceId serviceId;  // Only used for ELDERFIER tier
    
    bool operator==(const EldernodeConsensusParticipant& other) const;
    bool operator<(const EldernodeConsensusParticipant& other) const;
};

// Consensus result structure
struct EldernodeConsensusResult {
    bool consensusReached;
    uint32_t requiredThreshold;
    uint32_t actualVotes;
    std::vector<Crypto::PublicKey> participatingEldernodes;
    std::vector<uint8_t> aggregatedSignature;
    uint64_t consensusTimestamp;
    
    bool isValid() const;
    std::string toString() const;
};

// ENindex entry structure
struct ENindexEntry {
    Crypto::PublicKey eldernodePublicKey;
    std::string feeAddress;
    uint64_t stakeAmount;
    uint64_t registrationTimestamp;
    bool isActive;
    uint32_t consensusParticipationCount;
    std::chrono::system_clock::time_point lastActivity;
    EldernodeTier tier;
    ElderfierServiceId serviceId;  // Only used for ELDERFIER tier
    ConstantStakeProofType constantProofType; // Constant stake proof type for cross-chain validation
    std::string crossChainAddress; // Address on target chain (e.g., C0DL3/zkSync)
    uint64_t constantStakeAmount;  // Amount locked for constant proof (e.g., 8000 XFG for Elderado)
    uint64_t constantProofExpiry;  // Expiry timestamp for constant proof (0 = never expires)
    
    bool operator==(const ENindexEntry& other) const;
    bool operator<(const ENindexEntry& other) const;
    bool hasConstantProof() const;
    bool isConstantProofExpired() const;
};

// Consensus thresholds configuration
struct ConsensusThresholds {
    uint32_t minimumEldernodes;
    uint32_t requiredAgreement; // e.g., 4/5 instead of 3/5
    uint32_t timeoutSeconds;
    uint32_t retryAttempts;
    
    static ConsensusThresholds getDefault();
    bool isValid() const;
};

// Stake verification result
struct StakeVerificationResult {
    bool isValid;
    std::string errorMessage;
    uint64_t verifiedAmount;
    Crypto::Hash verifiedStakeHash;
    
    static StakeVerificationResult success(uint64_t amount, const Crypto::Hash& hash);
    static StakeVerificationResult failure(const std::string& error);
};

// Slashing configuration
enum class SlashingDestination : uint8_t {
    BURN = 0,           // Burn slashed stakes (remove from circulation)
    TREASURY = 1,       // Send to network treasury address
    REDISTRIBUTE = 2,   // Redistribute to other Eldernodes
    CHARITY = 3         // Send to charity/community fund address
};

struct SlashingConfig {
    SlashingDestination destination;
    std::string destinationAddress; // Address for treasury/charity
    uint64_t slashingPercentage;    // Percentage of stake to slash (e.g., 50 = 50%)
    bool enableSlashing;            // Whether slashing is enabled
    
    static SlashingConfig getDefault();
    bool isValid() const;
};

// Constant stake proof configuration
struct ConstantStakeProofConfig {
    bool enableElderadoC0DL3Validator; // Enable Elderado validator stake for C0DL3
    uint64_t elderadoC0DL3StakeAmount; // Required stake amount for Elderado validator (8000 XFG)
    uint64_t constantProofValidityPeriod; // Validity period for constant proofs (0 = never expires)
    std::string c0dl3NetworkId; // C0DL3 network identifier
    std::string c0dl3ContractAddress; // C0DL3 validator contract address
    bool allowConstantProofRenewal; // Allow renewal of constant proofs
    
    static ConstantStakeProofConfig getDefault();
    bool isValid() const;
    uint64_t getRequiredStakeAmount(ConstantStakeProofType type) const;
};

// Elderfier service configuration
struct ElderfierServiceConfig {
    uint64_t minimumStakeAmount;      // 800 XFG minimum for Elderfier
    uint64_t customNameLength;        // Exactly 8 letters for custom names
    bool allowHashedAddresses;        // Whether to allow hashed addresses
    std::vector<std::string> reservedNames; // Reserved custom names
    SlashingConfig slashingConfig;    // Slashing configuration
    ConstantStakeProofConfig constantProofConfig; // Constant stake proof configuration
    
    static ElderfierServiceConfig getDefault();
    bool isValid() const;
    bool isCustomNameReserved(const std::string& name) const;
    bool isValidCustomName(const std::string& name) const;
};

} // namespace CryptoNote
