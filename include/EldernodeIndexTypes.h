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
    CUSTOM_NAME = 1,         // Custom name (up to 8 characters)
    HASHED_ADDRESS = 2       // Hashed public fee address (for privacy)
};

// Service ID structure for Elderfier nodes
struct ElderfierServiceId {
    ServiceIdType type;
    std::string identifier;  // Raw identifier (address, name, or hash)
    std::string displayName; // Human-readable display name
    
    bool isValid() const;
    std::string toString() const;
    static ElderfierServiceId createStandardAddress(const std::string& address);
    static ElderfierServiceId createCustomName(const std::string& name);
    static ElderfierServiceId createHashedAddress(const std::string& address);
};

// Eldernode tier levels
enum class EldernodeTier : uint8_t {
    BASIC = 0,           // Basic Eldernode
    ELDERFIER = 1        // Elderfier service node (higher tier)
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
    
    bool isValid() const;
    std::string toString() const;
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
    
    bool operator==(const ENindexEntry& other) const;
    bool operator<(const ENindexEntry& other) const;
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

// Elderfier service configuration
struct ElderfierServiceConfig {
    uint64_t minimumStakeAmount;      // Higher stake requirement for Elderfier
    uint64_t maximumCustomNameLength; // Max length for custom names
    bool allowHashedAddresses;        // Whether to allow hashed addresses
    std::vector<std::string> reservedNames; // Reserved custom names
    
    static ElderfierServiceConfig getDefault();
    bool isValid() const;
    bool isCustomNameReserved(const std::string& name) const;
};

} // namespace CryptoNote
