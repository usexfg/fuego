#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include "crypto/hash.h"
#include "crypto/crypto.h"

namespace CryptoNote {

// Eldernode stake proof structure
struct EldernodeStakeProof {
    Crypto::Hash stakeHash;
    Crypto::PublicKey eldernodePublicKey;
    uint64_t stakeAmount;
    uint64_t timestamp;
    std::vector<uint8_t> proofSignature;
    std::string feeAddress;
    
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
    
    bool operator==(const ENindexEntry& other) const;
    bool operator<(const ENindexEntry& other) const;
};

// Consensus thresholds configuration
struct ConsensusThresholds {
    uint32_t minimumEldernodes;
    uint32_t requiredAgreement; // e.g., 4 out of 5 = 4/5
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

} // namespace CryptoNote
