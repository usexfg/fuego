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
    
    bool isConstantProof() const;
    bool isConstantProofExpired() const;
};

// Security window configuration
namespace SecurityWindow {
    static const uint64_t DEFAULT_DURATION_SECONDS = 28800;      // 8 hours
    static const uint64_t MINIMUM_SIGNATURE_INTERVAL = 3600;     // 1 hour minimum between signatures
    static const uint64_t GRACE_PERIOD_SECONDS = 300;            // 5 minute grace period
    static const uint64_t MAX_OFFLINE_TIME = 86400;              // 24 hours max offline
}

// Mempool buffer security window for spending transactions
struct MempoolSecurityWindow {
    Crypto::Hash transactionHash;        // Hash of spending transaction
    Crypto::PublicKey elderfierPublicKey; // Elderfier attempting to spend
    uint64_t timestamp;                  // When transaction entered buffer
    uint64_t securityWindowEnd;          // When security window ends
    bool signatureValidated;             // Whether last signature was valid
    bool elderCouncilVoteRequired;       // Whether Elder Council vote is needed
    std::vector<Crypto::PublicKey> votes; // Elder Council votes (for/against)
    uint32_t requiredVotes;              // Required votes for quorum
    uint32_t currentVotes;               // Current vote count
    
    bool isSecurityWindowActive() const;
    bool hasQuorumReached() const;
    bool canReleaseTransaction() const;
    void addVote(const Crypto::PublicKey& voter);
    std::string toString() const;
    bool isConstantProof() const;
    bool isConstantProofExpired() const;
};

// Elder Council voting system
struct ElderCouncilVote {
    Crypto::PublicKey voterPublicKey;    // Elderfier who voted
    Crypto::PublicKey targetPublicKey;   // Elderfier being voted on
    bool voteFor;                        // true = allow spending, false = deny
    uint64_t timestamp;                  // Vote timestamp
    Crypto::Hash voteHash;               // Hash of vote data
    std::vector<uint8_t> signature;      // Vote signature
    
    bool isValid() const;
    Crypto::Hash calculateVoteHash() const;
    std::string toString() const;
};

// Elder Council voting message (like email inbox)
struct ElderCouncilVotingMessage {
    Crypto::Hash messageId;              // Unique message ID
    Crypto::PublicKey targetElderfier;   // Elderfier being voted on
    std::string subject;                  // Subject line
    std::string description;              // Detailed description of situation
    uint64_t timestamp;                  // When message was created
    uint64_t votingDeadline;             // When voting closes
    bool isRead;                         // Whether Elderfier has read the message
    bool hasVoted;                       // Whether Elderfier has voted on this
    bool hasConfirmedVote;               // Whether Elderfier has confirmed their vote
    ElderCouncilVoteType pendingVoteType; // Pending vote type (before confirmation)
    ElderCouncilVoteType confirmedVoteType; // Confirmed vote type (after confirmation)
    std::vector<ElderCouncilVote> votes;  // Votes cast so far
    uint32_t requiredVotes;              // Required votes for quorum
    uint32_t currentVotes;               // Current vote count
    
    bool isVotingActive() const;
    bool hasQuorumReached() const;
    std::string getVotingStatus() const;
    std::string toString() const;
};

// Vote types for Elder Council decisions
enum class ElderCouncilVoteType : uint8_t {
    SLASH_ALL = 1,      // Slash/burn ALL of Elderfier's stake
    SLASH_HALF = 2,      // Slash/burn HALF of Elderfier's stake  
    SLASH_NONE = 3       // Slash/burn NONE of Elderfier's stake
};

// Misbehavior evidence for Elder Council voting
struct MisbehaviorEvidence {
    Crypto::PublicKey elderfierPublicKey; // Elderfier who misbehaved
    uint32_t invalidSignatures;           // Number of invalid signatures
    uint32_t totalAttempts;              // Total signature attempts
    uint64_t firstInvalidSignature;      // Timestamp of first invalid signature
    uint64_t lastInvalidSignature;      // Timestamp of last invalid signature
    std::vector<Crypto::Hash> invalidSignatureHashes; // Hashes of invalid signatures
    std::string misbehaviorType;        // Type of misbehavior (e.g., "Invalid Signatures")
    std::string evidenceDescription;     // Detailed description of evidence
    
    bool isValid() const;
    std::string getSummary() const;
    std::string toString() const;
};

// Monitoring configuration
struct ElderfierMonitoringConfig {
    bool enableBlockBasedMonitoring;      // Monitor each block for 0x06 spending transactions
    bool enableMempoolBuffer;            // Enable mempool security window buffer
    bool enableElderCouncilVoting;        // Enable Elder Council voting system
    uint64_t mempoolBufferDuration;      // How long to hold transactions in buffer (default: 8 hours)
    uint32_t elderCouncilQuorumSize;     // Required votes for Elder Council quorum (default: 5)
    uint64_t votingWindowDuration;      // How long voting window stays open (default: 24 hours)
    
    static ElderfierMonitoringConfig getDefault() {
        ElderfierMonitoringConfig config;
        config.enableBlockBasedMonitoring = true;  // Monitor each block
        config.enableMempoolBuffer = true;          // Enable mempool buffer
        config.enableElderCouncilVoting = true;     // Enable Elder Council voting
        config.mempoolBufferDuration = 28800;        // 8 hours buffer
        config.elderCouncilQuorumSize = 5;           // 5 votes for quorum
        config.votingWindowDuration = 86400;         // 24 hours voting window
        return config;
    }
    
    bool isValid() const {
        return elderCouncilQuorumSize > 0 && elderCouncilQuorumSize <= 20; // Max 20 votes
    }
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
    bool isUnlocked;                 // Can be unlocked after security window
    bool isSpent;                    // True if deposit funds have been spent
    
    // Security window fields
    uint64_t lastSignatureTimestamp; // Last signature timestamp
    uint64_t securityWindowEnd;      // When security window ends
    uint64_t securityWindowDuration; // Duration of security window
    bool isInSecurityWindow;         // Currently in security window
    bool unlockRequested;            // Elderfier requested to unlock
    uint64_t unlockRequestTimestamp; // When unlock was requested
    
    // Methods
    bool isValid() const;
    bool isOnline() const;
    bool isDepositValid() const;     // Check if deposit is still valid (not spent)
    bool canUnlock() const;          // Check if deposit can be unlocked (outside security window)
    uint64_t getSecurityWindowRemaining() const; // Get remaining time in security window
    uint32_t calculateSelectionMultiplier() const;
    void updateUptime(uint64_t currentTimestamp);
    void markOffline(uint64_t currentTimestamp);
    void markSpent();                // Mark deposit as spent (invalidates Elderfier status)
    void updateLastSignature(uint64_t timestamp); // Update last signature timestamp
    void requestUnlock(uint64_t timestamp);       // Request to unlock deposit
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
