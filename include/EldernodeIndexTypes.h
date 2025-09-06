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
    ELDERFIER = 1,       // Elderfier service node (800 XFG stake required)
    ELDARADO = 2         // Eldarado validator (8000 XFG stake required)
};

// Elderfier deposit data structure
struct ElderfierDepositData {
    Crypto::Hash depositHash;
    Crypto::PublicKey elderfierPublicKey;
    uint64_t depositAmount;
    uint64_t depositTimestamp;
    uint64_t lastSeenTimestamp;
    uint64_t totalUptimeSeconds;
    uint32_t selectionMultiplier;
    std::string elderfierAddress;
    ElderfierServiceId serviceId;
    bool isActive;
    bool isSlashable;
    
    // Methods
    bool isValid() const;
    bool isOnline() const;
    uint32_t calculateSelectionMultiplier() const;
    void updateUptime(uint64_t currentTimestamp);
    void markOffline(uint64_t currentTimestamp);
    std::string toString() const;
};

// Fee structure constants
namespace EldernodeFees {
    static const uint64_t LARGE_BURN_FEE = 8000000;      // 0.8 XFG for large burns (800 XFG+)
    static const uint64_t DEFAULT_BURN_FEE = 80000;       // 0.008 XFG for default burns
    static const uint64_t ELDERFIER_STAKE_AMOUNT = 8000000000;  // 800 XFG stake required
    static const uint64_t ELDARADO_STAKE_AMOUNT = 80000000000;  // 8000 XFG stake required
}

// Selection multiplier mapping based on uptime duration
namespace SelectionMultipliers {
    static const uint64_t MONTH_1_SECONDS = 2592000;    // 30 days
    static const uint64_t MONTH_3_SECONDS = 7776000;   // 90 days  
    static const uint64_t MONTH_6_SECONDS = 15552000;   // 180 days
    static const uint64_t YEAR_1_SECONDS = 31536000;    // 365 days
    static const uint64_t YEAR_2_SECONDS = 63072000;    // 730 days
    
    static const uint32_t UPTIME_1_MONTH_MULTIPLIER = 1;   // 1x (0-1 month)
    static const uint32_t UPTIME_3_MONTH_MULTIPLIER = 2;   // 2x (1-3 months)
    static const uint32_t UPTIME_6_MONTH_MULTIPLIER = 4;   // 4x (3-6 months)
    static const uint32_t UPTIME_1_YEAR_MULTIPLIER = 8;    // 8x (6-12 months)
    static const uint32_t UPTIME_2_YEAR_MULTIPLIER = 16;   // 16x (1-2 years)
    static const uint32_t MAX_MULTIPLIER = 16;             // Cap at 2 years
}

// Eldernode consensus participant
struct EldernodeConsensusParticipant {
    Crypto::PublicKey publicKey;
    std::string address;
    uint64_t stakeAmount;
    uint32_t selectionMultiplier;  // Selection probability multiplier
    bool isActive;
    std::chrono::system_clock::time_point lastSeen;
    EldernodeTier tier;
    ElderfierServiceId serviceId;  // Only used for ELDERFIER tier
    
    bool operator==(const EldernodeConsensusParticipant& other) const;
    bool operator<(const EldernodeConsensusParticipant& other) const;
};

// Random selection result for Elderfier verification
struct ElderfierSelectionResult {
    std::vector<EldernodeConsensusParticipant> selectedElderfiers;  // Exactly 2 Elderfiers
    Crypto::Hash selectionHash;  // Provably fair random seed
    uint64_t blockHeight;        // Block height used for selection
    uint64_t totalWeight;        // Sum of all selection multipliers
    std::vector<uint32_t> selectionWeights;  // Individual weights used in selection
    
    bool isValid() const;        // Verify exactly 2 Elderfiers selected
    std::string toString() const;
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

// Deposit validation result
struct DepositValidationResult {
    bool isValid;
    std::string errorMessage;
    uint64_t validatedAmount;
    Crypto::Hash validatedDepositHash;
    
    static DepositValidationResult success(uint64_t amount, const Crypto::Hash& hash);
    static DepositValidationResult failure(const std::string& error);
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

// Elderfier service configuration
struct ElderfierServiceConfig {
    uint64_t minimumStakeAmount;      // 800 XFG minimum for Elderfier
    uint64_t customNameLength;        // Exactly 8 letters for custom names
    bool allowHashedAddresses;        // Whether to allow hashed addresses
    std::vector<std::string> reservedNames; // Reserved custom names
    SlashingConfig slashingConfig;    // Slashing configuration
    
    static ElderfierServiceConfig getDefault();
    bool isValid() const;
    bool isCustomNameReserved(const std::string& name) const;
    bool isValidCustomName(const std::string& name) const;
};

} // namespace CryptoNote
