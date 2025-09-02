#include "EldernodeIndexTypes.h"
#include "Common/StringTools.h"
#include <sstream>
#include <iomanip>

namespace CryptoNote {

// EldernodeStakeProof implementations
bool EldernodeStakeProof::isValid() const {
    return stakeAmount > 0 && 
           !feeAddress.empty() && 
           !proofSignature.empty() &&
           timestamp > 0;
}

std::string EldernodeStakeProof::toString() const {
    std::ostringstream oss;
    oss << "EldernodeStakeProof{"
        << "stakeHash=" << Common::podToHex(stakeHash) << ", "
        << "publicKey=" << Common::podToHex(eldernodePublicKey) << ", "
        << "amount=" << stakeAmount << ", "
        << "timestamp=" << timestamp << ", "
        << "feeAddress=" << feeAddress << ", "
        << "signatureSize=" << proofSignature.size() << "}";
    return oss.str();
}

// EldernodeConsensusParticipant implementations
bool EldernodeConsensusParticipant::operator==(const EldernodeConsensusParticipant& other) const {
    return publicKey == other.publicKey && 
           address == other.address && 
           stakeAmount == other.stakeAmount &&
           isActive == other.isActive;
}

bool EldernodeConsensusParticipant::operator<(const EldernodeConsensusParticipant& other) const {
    if (stakeAmount != other.stakeAmount) {
        return stakeAmount > other.stakeAmount; // Higher stake first
    }
    return publicKey < other.publicKey;
}

// EldernodeConsensusResult implementations
bool EldernodeConsensusResult::isValid() const {
    return consensusTimestamp > 0 && 
           (consensusReached ? actualVotes >= requiredThreshold : true);
}

std::string EldernodeConsensusResult::toString() const {
    std::ostringstream oss;
    oss << "EldernodeConsensusResult{"
        << "reached=" << (consensusReached ? "true" : "false") << ", "
        << "votes=" << actualVotes << "/" << requiredThreshold << ", "
        << "participants=" << participatingEldernodes.size() << ", "
        << "timestamp=" << consensusTimestamp << ", "
        << "signatureSize=" << aggregatedSignature.size() << "}";
    return oss.str();
}

// ENindexEntry implementations
bool ENindexEntry::operator==(const ENindexEntry& other) const {
    return eldernodePublicKey == other.eldernodePublicKey && 
           feeAddress == other.feeAddress && 
           stakeAmount == other.stakeAmount &&
           registrationTimestamp == other.registrationTimestamp &&
           isActive == other.isActive;
}

bool ENindexEntry::operator<(const ENindexEntry& other) const {
    if (stakeAmount != other.stakeAmount) {
        return stakeAmount > other.stakeAmount; // Higher stake first
    }
    if (registrationTimestamp != other.registrationTimestamp) {
        return registrationTimestamp < other.registrationTimestamp; // Older first
    }
    return eldernodePublicKey < other.eldernodePublicKey;
}

// ConsensusThresholds implementations
ConsensusThresholds ConsensusThresholds::getDefault() {
    ConsensusThresholds thresholds;
    thresholds.minimumEldernodes = 5;      // Minimum 5 Eldernodes required
    thresholds.requiredAgreement = 4;     // 4 out of 5 agreement (4/5 instead of 3/5)
    thresholds.timeoutSeconds = 30;       // 30 second timeout
    thresholds.retryAttempts = 3;        // 3 retry attempts
    return thresholds;
}

bool ConsensusThresholds::isValid() const {
    return minimumEldernodes > 0 && 
           requiredAgreement > 0 && 
           requiredAgreement <= minimumEldernodes &&
           timeoutSeconds > 0 && 
           retryAttempts >= 0;
}

// StakeVerificationResult implementations
StakeVerificationResult StakeVerificationResult::success(uint64_t amount, const Crypto::Hash& hash) {
    StakeVerificationResult result;
    result.isValid = true;
    result.errorMessage = "";
    result.verifiedAmount = amount;
    result.verifiedStakeHash = hash;
    return result;
}

StakeVerificationResult StakeVerificationResult::failure(const std::string& error) {
    StakeVerificationResult result;
    result.isValid = false;
    result.errorMessage = error;
    result.verifiedAmount = 0;
    result.verifiedStakeHash = Crypto::Hash();
    return result;
}

} // namespace CryptoNote
