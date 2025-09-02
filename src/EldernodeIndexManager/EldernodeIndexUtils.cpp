#include "EldernodeIndexTypes.h"
#include "Common/StringTools.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace CryptoNote {

// ElderfierServiceId implementations
bool ElderfierServiceId::isValid() const {
    if (identifier.empty()) {
        return false;
    }
    
    switch (type) {
        case ServiceIdType::STANDARD_ADDRESS:
            return identifier.length() >= 10 && identifier.length() <= 100;
            
        case ServiceIdType::CUSTOM_NAME:
            return identifier.length() >= 1 && identifier.length() <= 8 &&
                   std::all_of(identifier.begin(), identifier.end(), 
                              [](char c) { return std::isalnum(c) || c == '_' || c == '-'; });
            
        case ServiceIdType::HASHED_ADDRESS:
            return identifier.length() == 64 && // SHA256 hash length
                   std::all_of(identifier.begin(), identifier.end(), 
                              [](char c) { return std::isxdigit(c); });
            
        default:
            return false;
    }
}

std::string ElderfierServiceId::toString() const {
    std::ostringstream oss;
    oss << "ElderfierServiceId{"
        << "type=";
    
    switch (type) {
        case ServiceIdType::STANDARD_ADDRESS:
            oss << "STANDARD_ADDRESS";
            break;
        case ServiceIdType::CUSTOM_NAME:
            oss << "CUSTOM_NAME";
            break;
        case ServiceIdType::HASHED_ADDRESS:
            oss << "HASHED_ADDRESS";
            break;
    }
    
    oss << ", identifier=\"" << identifier << "\""
        << ", displayName=\"" << displayName << "\"}";
    return oss.str();
}

ElderfierServiceId ElderfierServiceId::createStandardAddress(const std::string& address) {
    ElderfierServiceId serviceId;
    serviceId.type = ServiceIdType::STANDARD_ADDRESS;
    serviceId.identifier = address;
    serviceId.displayName = address;
    return serviceId;
}

ElderfierServiceId ElderfierServiceId::createCustomName(const std::string& name) {
    ElderfierServiceId serviceId;
    serviceId.type = ServiceIdType::CUSTOM_NAME;
    serviceId.identifier = name;
    serviceId.displayName = name;
    return serviceId;
}

ElderfierServiceId ElderfierServiceId::createHashedAddress(const std::string& address) {
    ElderfierServiceId serviceId;
    serviceId.type = ServiceIdType::HASHED_ADDRESS;
    
    // Hash the address using SHA256
    Crypto::Hash hash;
    Crypto::cn_fast_hash(address.data(), address.size(), hash);
    serviceId.identifier = Common::podToHex(hash);
    
    // Create a masked display name for privacy
    if (address.length() >= 8) {
        serviceId.displayName = address.substr(0, 4) + "..." + address.substr(address.length() - 4);
    } else {
        serviceId.displayName = "***" + address.substr(address.length() - 2);
    }
    
    return serviceId;
}

// EldernodeStakeProof implementations
bool EldernodeStakeProof::isValid() const {
    return stakeAmount > 0 && 
           !feeAddress.empty() && 
           !proofSignature.empty() &&
           timestamp > 0 &&
           (tier == EldernodeTier::BASIC || serviceId.isValid());
}

std::string EldernodeStakeProof::toString() const {
    std::ostringstream oss;
    oss << "EldernodeStakeProof{"
        << "stakeHash=" << Common::podToHex(stakeHash) << ", "
        << "publicKey=" << Common::podToHex(eldernodePublicKey) << ", "
        << "amount=" << stakeAmount << ", "
        << "timestamp=" << timestamp << ", "
        << "feeAddress=" << feeAddress << ", "
        << "tier=" << (tier == EldernodeTier::BASIC ? "BASIC" : "ELDERFIER") << ", "
        << "signatureSize=" << proofSignature.size();
    
    if (tier == EldernodeTier::ELDERFIER) {
        oss << ", serviceId=" << serviceId.toString();
    }
    
    oss << "}";
    return oss.str();
}

// EldernodeConsensusParticipant implementations
bool EldernodeConsensusParticipant::operator==(const EldernodeConsensusParticipant& other) const {
    return publicKey == other.publicKey && 
           address == other.address && 
           stakeAmount == other.stakeAmount &&
           isActive == other.isActive &&
           tier == other.tier &&
           serviceId.identifier == other.serviceId.identifier;
}

bool EldernodeConsensusParticipant::operator<(const EldernodeConsensusParticipant& other) const {
    // Elderfier nodes have higher priority
    if (tier != other.tier) {
        return tier > other.tier; // ELDERFIER > BASIC
    }
    
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
           isActive == other.isActive &&
           tier == other.tier &&
           serviceId.identifier == other.serviceId.identifier;
}

bool ENindexEntry::operator<(const ENindexEntry& other) const {
    // Elderfier nodes have higher priority
    if (tier != other.tier) {
        return tier > other.tier; // ELDERFIER > BASIC
    }
    
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

// ElderfierServiceConfig implementations
ElderfierServiceConfig ElderfierServiceConfig::getDefault() {
    ElderfierServiceConfig config;
    config.minimumStakeAmount = 5000000;      // 5 FUEGO minimum for Elderfier (vs 1 for basic)
    config.maximumCustomNameLength = 8;        // Max 8 characters for custom names
    config.allowHashedAddresses = true;        // Allow hashed addresses for privacy
    config.reservedNames = {
        "admin", "root", "system", "fuego", "elder", "node", "test", "dev", "main", "prod"
    };
    return config;
}

bool ElderfierServiceConfig::isValid() const {
    return minimumStakeAmount > 0 && 
           maximumCustomNameLength > 0 && 
           maximumCustomNameLength <= 16; // Reasonable upper limit
}

bool ElderfierServiceConfig::isCustomNameReserved(const std::string& name) const {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return std::find(reservedNames.begin(), reservedNames.end(), lowerName) != reservedNames.end();
}

} // namespace CryptoNote
