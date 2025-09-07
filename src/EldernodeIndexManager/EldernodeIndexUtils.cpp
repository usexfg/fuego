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
            // Must be exactly 8 letters, all caps
            if (identifier.length() != 8) {
                return false;
            }
            if (!std::all_of(identifier.begin(), identifier.end(), ::isupper)) {
                return false;
            }
            if (!std::all_of(identifier.begin(), identifier.end(), ::isalpha)) {
                return false;
            }
            // Must have linked address and hashed address for custom names
            return !linkedAddress.empty() && !hashedAddress.empty();
            
        case ServiceIdType::HASHED_ADDRESS:
            return identifier.length() == 64 && // SHA256 hash length
                   std::all_of(identifier.begin(), identifier.end(), 
                              [](char c) { return std::isxdigit(c); }) &&
                   !linkedAddress.empty() && !hashedAddress.empty();
            
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
        << ", displayName=\"" << displayName << "\""
        << ", linkedAddress=\"" << linkedAddress << "\""
        << ", hashedAddress=\"" << hashedAddress << "\"}";
    return oss.str();
}

ElderfierServiceId ElderfierServiceId::createStandardAddress(const std::string& address) {
    ElderfierServiceId serviceId;
    serviceId.type = ServiceIdType::STANDARD_ADDRESS;
    serviceId.identifier = address;
    serviceId.displayName = address;
    serviceId.linkedAddress = address; // Same as identifier for standard addresses
    
    // Generate hashed address for all Elderfier nodes
    Crypto::Hash hash;
    Crypto::cn_fast_hash(address.data(), address.size(), hash);
    serviceId.hashedAddress = Common::podToHex(hash);
    
    return serviceId;
}

ElderfierServiceId ElderfierServiceId::createCustomName(const std::string& name, const std::string& walletAddress) {
    ElderfierServiceId serviceId;
    serviceId.type = ServiceIdType::CUSTOM_NAME;
    
    // Convert to uppercase and ensure exactly 8 letters
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    // Pad or truncate to exactly 8 characters
    if (upperName.length() < 8) {
        upperName.resize(8, 'X'); // Pad with X
    } else if (upperName.length() > 8) {
        upperName = upperName.substr(0, 8); // Truncate to 8
    }
    
    serviceId.identifier = upperName;
    serviceId.displayName = upperName;
    serviceId.linkedAddress = walletAddress; // Link to actual wallet address
    
    // Generate hashed address for all Elderfier nodes
    Crypto::Hash hash;
    Crypto::cn_fast_hash(walletAddress.data(), walletAddress.size(), hash);
    serviceId.hashedAddress = Common::podToHex(hash);
    
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
    
    serviceId.linkedAddress = address; // Link to actual wallet address
    serviceId.hashedAddress = serviceId.identifier; // Same as identifier for hashed type
    
    return serviceId;
}

// EldernodeStakeProof implementations
bool EldernodeStakeProof::isValid() const {
    return !feeAddress.empty() && 
           !proofSignature.empty() &&
           timestamp > 0 &&
           (tier == EldernodeTier::ELDERFIER || serviceId.isValid());
}

bool EldernodeStakeProof::isConstantProof() const {
    return constantProofType != ConstantStakeProofType::NONE;
}

bool EldernodeStakeProof::isConstantProofExpired() const {
    if (!isConstantProof() || constantProofExpiry == 0) {
        return false; // No expiry for non-constant proofs or never-expiring proofs
    }
    
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return currentTime > constantProofExpiry;
}

std::string EldernodeStakeProof::toString() const {
    std::ostringstream oss;
    oss << "EldernodeStakeProof{"
        << "stakeHash=" << Common::podToHex(stakeHash) << ", "
        << "publicKey=" << Common::podToHex(eldernodePublicKey) << ", "
        << "amount=" << stakeAmount << ", "
        << "timestamp=" << timestamp << ", "
        << "feeAddress=" << feeAddress << ", "
        << "tier=" << (tier == EldernodeTier::ELDERFIER ? "BASIC" : "ELDERFIER") << ", "
        << "signatureSize=" << proofSignature.size();
    
    if (tier == EldernodeTier::ELDERFIER) {
        oss << ", serviceId=" << serviceId.toString();
    }
    
    if (isConstantProof()) {
        oss << ", constantProofType=" << static_cast<int>(constantProofType)
            << ", crossChainAddress=" << crossChainAddress
            << ", constantStakeAmount=" << constantStakeAmount
            << ", constantProofExpiry=" << constantProofExpiry;
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
           serviceId.identifier == other.serviceId.identifier &&
           constantProofType == other.constantProofType &&
           crossChainAddress == other.crossChainAddress &&
           constantStakeAmount == other.constantStakeAmount &&
           constantProofExpiry == other.constantProofExpiry;
}

bool ENindexEntry::hasConstantProof() const {
    return constantProofType != ConstantStakeProofType::NONE;
}

bool ENindexEntry::isConstantProofExpired() const {
    if (!hasConstantProof() || constantProofExpiry == 0) {
        return false; // No expiry for non-constant proofs or never-expiring proofs
    }
    
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return currentTime > constantProofExpiry;
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
           timeoutSeconds > 0;
}

// DepositValidationResult implementations
DepositValidationResult DepositValidationResult::success(uint64_t amount, const Crypto::Hash& hash) {
    DepositValidationResult result;
    result.isValid = true;
    result.errorMessage = "";
    result.validatedAmount = amount;
    result.validatedDepositHash = hash;
    return result;
}

DepositValidationResult DepositValidationResult::failure(const std::string& error) {
    DepositValidationResult result;
    result.isValid = false;
    result.errorMessage = error;
    result.validatedAmount = 0;
    result.validatedDepositHash = Crypto::Hash();
    return result;
}

// SlashingConfig implementations
SlashingConfig SlashingConfig::getDefault() {
    SlashingConfig config;
    config.destination = SlashingDestination::TREASURY;
    config.destinationAddress = "FUEGOTREASURY123456789abcdef"; // Network treasury address
    config.slashingPercentage = 50; // 50% of stake slashed for misbehavior
    config.enableSlashing = true;
    return config;
}

bool SlashingConfig::isValid() const {
    return slashingPercentage > 0 && slashingPercentage <= 100 &&
           (destination == SlashingDestination::BURN || !destinationAddress.empty());
}

// ConstantStakeProofConfig implementations
ConstantStakeProofConfig ConstantStakeProofConfig::getDefault() {
    ConstantStakeProofConfig config;
    config.enableElderadoC0DL3Validator = true;  // Enable Elderado validator stake for C0DL3
    config.elderadoC0DL3StakeAmount = 8000000000; // 8000 XFG for Elderado validator (8000 * 1,000,000)
    config.constantProofValidityPeriod = 0;      // 0 = never expires (constant proof)
    config.c0dl3NetworkId = "C0DL3_MAINNET";     // C0DL3 network identifier
    config.c0dl3ContractAddress = "0x0000000000000000000000000000000000000000"; // Placeholder contract address
    config.allowConstantProofRenewal = true;      // Allow renewal of constant proofs
    return config;
}

bool ConstantStakeProofConfig::isValid() const {
    return elderadoC0DL3StakeAmount > 0 &&
           !c0dl3NetworkId.empty() &&
           !c0dl3ContractAddress.empty();
}

uint64_t ConstantStakeProofConfig::getRequiredStakeAmount(ConstantStakeProofType type) const {
    switch (type) {
        case ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR:
            return elderadoC0DL3StakeAmount;
        default:
            return 0;
    }
}

// ElderfierServiceConfig implementations
ElderfierServiceConfig ElderfierServiceConfig::getDefault() {
    ElderfierServiceConfig config;
    config.minimumStakeAmount = 800000000;      // 800 XFG minimum for Elderfier (800 * 1,000,000)
    config.customNameLength = 8;                 // Exactly 8 letters for custom names
    config.allowHashedAddresses = true;         // Allow hashed addresses for privacy
    config.reservedNames = {
        "ADMIN", "ROOT", "SYSTEM", "FUEGO", "ELDER", "NODE", "TEST", "DEV", "MAIN", "PROD",
        "SERVER", "CLIENT", "MASTER", "SLAVE", "BACKUP", "CACHE", "DB", "API", "WEB", "APP"
    };
    config.slashingConfig = SlashingConfig::getDefault();
    config.constantProofConfig = ConstantStakeProofConfig::getDefault();
    return config;
}

bool ElderfierServiceConfig::isValid() const {
    return minimumStakeAmount > 0 && 
           customNameLength == 8 && // Must be exactly 8
           slashingConfig.isValid() &&
           constantProofConfig.isValid();
}

bool ElderfierServiceConfig::isCustomNameReserved(const std::string& name) const {
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    return std::find(reservedNames.begin(), reservedNames.end(), upperName) != reservedNames.end();
}

bool ElderfierServiceConfig::isValidCustomName(const std::string& name) const {
    // Must be exactly 8 letters, all caps, alphabetic only
    if (name.length() != 8) {
        return false;
    }
    
    if (!std::all_of(name.begin(), name.end(), ::isupper)) {
        return false;
    }
    
    if (!std::all_of(name.begin(), name.end(), ::isalpha)) {
        return false;
    }
    
    // Must not be reserved
    if (isCustomNameReserved(name)) {
        return false;
    }
    
    return true;
}

} // namespace CryptoNote
