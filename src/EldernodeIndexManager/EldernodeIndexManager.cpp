#include "EldernodeIndexManager.h"
#include "Common/StringTools.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "Logging/LoggerRef.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

using namespace Logging;

namespace CryptoNote {

EldernodeIndexManager::EldernodeIndexManager(Logging::ILogger& log)
    : logger(log, "EldernodeIndexManager")
    , m_consensusThresholds(ConsensusThresholds::getDefault())
    , m_elderfierConfig(ElderfierServiceConfig::getDefault())
    , m_lastUpdate(std::chrono::system_clock::now()) {
}

bool EldernodeIndexManager::addEldernode(const ENindexEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateEldernodeEntry(entry)) {
        logger(ERROR) << "Invalid Eldernode entry for public key: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    auto it = m_eldernodes.find(entry.eldernodePublicKey);
    if (it != m_eldernodes.end()) {
        logger(WARNING) << "Eldernode already exists: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    // Check for service ID conflicts for Elderfier nodes
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (!validateElderfierServiceId(entry.serviceId)) {
            logger(ERROR) << "Invalid Elderfier service ID for public key: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        if (hasServiceIdConflict(entry.serviceId, entry.eldernodePublicKey)) {
            logger(ERROR) << "Service ID conflict for Elderfier node: " << entry.serviceId.toString();
            return false;
        }
        
        // Verify that linked address matches fee address for all Elderfier types
        if (entry.serviceId.linkedAddress != entry.feeAddress) {
            logger(ERROR) << "Linked address mismatch for Elderfier node: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        // Verify that hashed address is present for all Elderfier nodes
        if (entry.serviceId.hashedAddress.empty()) {
            logger(ERROR) << "Missing hashed address for Elderfier node: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
    }
    
    m_eldernodes[entry.eldernodePublicKey] = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (entry.tier == EldernodeTier::BASIC) ? "Basic" : "Elderfier";
    logger(INFO) << "Added " << tierName << " Eldernode: " << Common::podToHex(entry.eldernodePublicKey) 
                << " with stake: " << entry.stakeAmount;
    
    if (entry.tier == EldernodeTier::ELDERFIER) {
        logger(INFO) << "Elderfier service ID: " << entry.serviceId.toString();
    }
    
    return true;
}

bool EldernodeIndexManager::removeEldernode(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(WARNING) << "Eldernode not found for removal: " << Common::podToHex(publicKey);
        return false;
    }
    
    std::string tierName = (it->second.tier == EldernodeTier::BASIC) ? "Basic" : "Elderfier";
    logger(INFO) << "Removing " << tierName << " Eldernode: " << Common::podToHex(publicKey);
    
    m_eldernodes.erase(it);
    m_stakeProofs.erase(publicKey);
    m_consensusParticipants.erase(publicKey);
    m_lastUpdate = std::chrono::system_clock::now();
    
    return true;
}

bool EldernodeIndexManager::updateEldernode(const ENindexEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateEldernodeEntry(entry)) {
        logger(ERROR) << "Invalid Eldernode entry for update: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    auto it = m_eldernodes.find(entry.eldernodePublicKey);
    if (it == m_eldernodes.end()) {
        logger(WARNING) << "Eldernode not found for update: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    // Check for service ID conflicts for Elderfier nodes (excluding self)
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (!validateElderfierServiceId(entry.serviceId)) {
            logger(ERROR) << "Invalid Elderfier service ID for update: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        if (hasServiceIdConflict(entry.serviceId, entry.eldernodePublicKey)) {
            logger(ERROR) << "Service ID conflict for Elderfier update: " << entry.serviceId.toString();
            return false;
        }
        
        // Verify that linked address matches fee address for all Elderfier types
        if (entry.serviceId.linkedAddress != entry.feeAddress) {
            logger(ERROR) << "Linked address mismatch for Elderfier update: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        // Verify that hashed address is present for all Elderfier nodes
        if (entry.serviceId.hashedAddress.empty()) {
            logger(ERROR) << "Missing hashed address for Elderfier update: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
    }
    
    it->second = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (entry.tier == EldernodeTier::BASIC) ? "Basic" : "Elderfier";
    logger(INFO) << "Updated " << tierName << " Eldernode: " << Common::podToHex(entry.eldernodePublicKey);
    
    return true;
}

std::optional<ENindexEntry> EldernodeIndexManager::getEldernode(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::vector<ENindexEntry> EldernodeIndexManager::getAllEldernodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ENindexEntry> result;
    result.reserve(m_eldernodes.size());
    
    for (const auto& pair : m_eldernodes) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<ENindexEntry> EldernodeIndexManager::getActiveEldernodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ENindexEntry> result;
    result.reserve(m_eldernodes.size());
    
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<ENindexEntry> EldernodeIndexManager::getElderfierNodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ENindexEntry> result;
    result.reserve(m_eldernodes.size());
    
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::optional<ENindexEntry> EldernodeIndexManager::getEldernodeByServiceId(const ElderfierServiceId& serviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && 
            pair.second.serviceId.identifier == serviceId.identifier) {
            return pair.second;
        }
    }
    
    return std::nullopt;
}

bool EldernodeIndexManager::addStakeProof(const EldernodeStakeProof& proof) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateStakeProof(proof)) {
        logger(ERROR) << "Invalid stake proof for public key: " << Common::podToHex(proof.eldernodePublicKey);
        return false;
    }
    
    auto& proofs = m_stakeProofs[proof.eldernodePublicKey];
    proofs.push_back(proof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (proof.tier == EldernodeTier::BASIC) ? "Basic" : "Elderfier";
    logger(INFO) << "Added stake proof for " << tierName << " Eldernode: " << Common::podToHex(proof.eldernodePublicKey)
                << " amount: " << proof.stakeAmount;
    
    if (proof.tier == EldernodeTier::ELDERFIER) {
        logger(INFO) << "Elderfier service ID: " << proof.serviceId.toString();
    }
    
    return true;
}

bool EldernodeIndexManager::verifyStakeProof(const EldernodeStakeProof& proof) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return validateStakeProof(proof);
}

std::vector<EldernodeStakeProof> EldernodeIndexManager::getStakeProofs(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_stakeProofs.find(publicKey);
    if (it == m_stakeProofs.end()) {
        return {};
    }
    
    return it->second;
}

bool EldernodeIndexManager::addConsensusParticipant(const EldernodeConsensusParticipant& participant) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_consensusParticipants[participant.publicKey] = participant;
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (participant.tier == EldernodeTier::BASIC) ? "Basic" : "Elderfier";
    logger(INFO) << "Added " << tierName << " consensus participant: " << Common::podToHex(participant.publicKey);
    
    return true;
}

bool EldernodeIndexManager::removeConsensusParticipant(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_consensusParticipants.find(publicKey);
    if (it == m_consensusParticipants.end()) {
        return false;
    }
    
    m_consensusParticipants.erase(it);
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Removed consensus participant: " << Common::podToHex(publicKey);
    return true;
}

std::vector<EldernodeConsensusParticipant> EldernodeIndexManager::getConsensusParticipants() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<EldernodeConsensusParticipant> result;
    result.reserve(m_consensusParticipants.size());
    
    for (const auto& pair : m_consensusParticipants) {
        result.push_back(pair.second);
    }
    
    return result;
}

EldernodeConsensusResult EldernodeIndexManager::reachConsensus(const std::vector<uint8_t>& data, const ConsensusThresholds& thresholds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    EldernodeConsensusResult result;
    result.consensusReached = false;
    result.requiredThreshold = thresholds.requiredAgreement;
    result.actualVotes = 0;
    result.consensusTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Get active participants (prioritize Elderfier nodes)
    std::vector<EldernodeConsensusParticipant> activeParticipants;
    for (const auto& pair : m_consensusParticipants) {
        if (pair.second.isActive) {
            activeParticipants.push_back(pair.second);
        }
    }
    
    // Sort by tier (Elderfier first) and stake amount
    std::sort(activeParticipants.begin(), activeParticipants.end());
    
    if (activeParticipants.size() < thresholds.minimumEldernodes) {
        logger(WARNING) << "Insufficient active Eldernodes for consensus: " 
                       << activeParticipants.size() << "/" << thresholds.minimumEldernodes;
        return result;
    }
    
    // Simulate consensus voting (prioritize Elderfier nodes)
    std::vector<std::vector<uint8_t>> signatures;
    for (const auto& participant : activeParticipants) {
        // Simulate signature generation
        Crypto::Hash dataHash;
        Crypto::cn_fast_hash(data.data(), data.size(), dataHash);
        
        // For now, we'll simulate a simple agreement
        // In real implementation, each Eldernode would sign the data
        std::vector<uint8_t> signature(64, 0); // Placeholder signature
        signatures.push_back(signature);
        result.participatingEldernodes.push_back(participant.publicKey);
    }
    
    result.actualVotes = static_cast<uint32_t>(signatures.size());
    
    // Check if consensus threshold is met (4/5 instead of 3/5 as requested)
    if (result.actualVotes >= thresholds.requiredAgreement) {
        result.consensusReached = true;
        result.aggregatedSignature = aggregateSignatures(signatures);
        logger(INFO) << "Consensus reached: " << result.actualVotes << "/" << thresholds.requiredAgreement;
    } else {
        logger(WARNING) << "Consensus failed: " << result.actualVotes << "/" << thresholds.requiredAgreement;
    }
    
    return result;
}

uint32_t EldernodeIndexManager::getTotalEldernodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_eldernodes.size());
}

uint32_t EldernodeIndexManager::getActiveEldernodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint32_t count = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            count++;
        }
    }
    
    return count;
}

uint32_t EldernodeIndexManager::getElderfierNodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint32_t count = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            count++;
        }
    }
    
    return count;
}

uint64_t EldernodeIndexManager::getTotalStakeAmount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint64_t total = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            total += pair.second.stakeAmount;
        }
    }
    
    return total;
}

std::chrono::system_clock::time_point EldernodeIndexManager::getLastUpdate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastUpdate;
}

bool EldernodeIndexManager::saveToStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ofstream file("eldernode_index.dat", std::ios::binary);
        if (!file) {
            logger(ERROR) << "Failed to open storage file for writing";
            return false;
        }
        
        // Write Eldernodes
        uint32_t eldernodeCount = static_cast<uint32_t>(m_eldernodes.size());
        file.write(reinterpret_cast<const char*>(&eldernodeCount), sizeof(eldernodeCount));
        
        for (const auto& pair : m_eldernodes) {
            const auto& entry = pair.second;
            file.write(reinterpret_cast<const char*>(&entry.eldernodePublicKey), sizeof(entry.eldernodePublicKey));
            uint32_t feeAddressSize = static_cast<uint32_t>(entry.feeAddress.size());
            file.write(reinterpret_cast<const char*>(&feeAddressSize), sizeof(feeAddressSize));
            file.write(entry.feeAddress.c_str(), feeAddressSize);
            file.write(reinterpret_cast<const char*>(&entry.stakeAmount), sizeof(entry.stakeAmount));
            file.write(reinterpret_cast<const char*>(&entry.registrationTimestamp), sizeof(entry.registrationTimestamp));
            file.write(reinterpret_cast<const char*>(&entry.isActive), sizeof(entry.isActive));
            file.write(reinterpret_cast<const char*>(&entry.tier), sizeof(entry.tier));
            
            // Write Elderfier service ID if applicable
            if (entry.tier == EldernodeTier::ELDERFIER) {
                file.write(reinterpret_cast<const char*>(&entry.serviceId.type), sizeof(entry.serviceId.type));
                uint32_t identifierSize = static_cast<uint32_t>(entry.serviceId.identifier.size());
                file.write(reinterpret_cast<const char*>(&identifierSize), sizeof(identifierSize));
                file.write(entry.serviceId.identifier.c_str(), identifierSize);
                uint32_t displayNameSize = static_cast<uint32_t>(entry.serviceId.displayName.size());
                file.write(reinterpret_cast<const char*>(&displayNameSize), sizeof(displayNameSize));
                file.write(entry.serviceId.displayName.c_str(), displayNameSize);
                uint32_t linkedAddressSize = static_cast<uint32_t>(entry.serviceId.linkedAddress.size());
                file.write(reinterpret_cast<const char*>(&linkedAddressSize), sizeof(linkedAddressSize));
                file.write(entry.serviceId.linkedAddress.c_str(), linkedAddressSize);
                uint32_t hashedAddressSize = static_cast<uint32_t>(entry.serviceId.hashedAddress.size());
                file.write(reinterpret_cast<const char*>(&hashedAddressSize), sizeof(hashedAddressSize));
                file.write(entry.serviceId.hashedAddress.c_str(), hashedAddressSize);
            }
            
            // Write constant stake proof data
            file.write(reinterpret_cast<const char*>(&entry.constantProofType), sizeof(entry.constantProofType));
            uint32_t crossChainAddressSize = static_cast<uint32_t>(entry.crossChainAddress.size());
            file.write(reinterpret_cast<const char*>(&crossChainAddressSize), sizeof(crossChainAddressSize));
            file.write(entry.crossChainAddress.c_str(), crossChainAddressSize);
            file.write(reinterpret_cast<const char*>(&entry.constantStakeAmount), sizeof(entry.constantStakeAmount));
            file.write(reinterpret_cast<const char*>(&entry.constantProofExpiry), sizeof(entry.constantProofExpiry));
        }
        
        logger(INFO) << "Saved " << eldernodeCount << " Eldernodes to storage";
        return true;
    } catch (const std::exception& e) {
        logger(ERROR) << "Exception during save: " << e.what();
        return false;
    }
}

bool EldernodeIndexManager::loadFromStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ifstream file("eldernode_index.dat", std::ios::binary);
        if (!file) {
            logger(INFO) << "No existing storage file found";
            return true; // Not an error if file doesn't exist
        }
        
        // Read Eldernodes
        uint32_t eldernodeCount;
        file.read(reinterpret_cast<char*>(&eldernodeCount), sizeof(eldernodeCount));
        
        for (uint32_t i = 0; i < eldernodeCount; ++i) {
            ENindexEntry entry;
            file.read(reinterpret_cast<char*>(&entry.eldernodePublicKey), sizeof(entry.eldernodePublicKey));
            
            uint32_t feeAddressSize;
            file.read(reinterpret_cast<char*>(&feeAddressSize), sizeof(feeAddressSize));
            entry.feeAddress.resize(feeAddressSize);
            file.read(&entry.feeAddress[0], feeAddressSize);
            
            file.read(reinterpret_cast<char*>(&entry.stakeAmount), sizeof(entry.stakeAmount));
            file.read(reinterpret_cast<char*>(&entry.registrationTimestamp), sizeof(entry.registrationTimestamp));
            file.read(reinterpret_cast<char*>(&entry.isActive), sizeof(entry.isActive));
            file.read(reinterpret_cast<char*>(&entry.tier), sizeof(entry.tier));
            
            // Read Elderfier service ID if applicable
            if (entry.tier == EldernodeTier::ELDERFIER) {
                file.read(reinterpret_cast<char*>(&entry.serviceId.type), sizeof(entry.serviceId.type));
                uint32_t identifierSize;
                file.read(reinterpret_cast<char*>(&identifierSize), sizeof(identifierSize));
                entry.serviceId.identifier.resize(identifierSize);
                file.read(&entry.serviceId.identifier[0], identifierSize);
                uint32_t displayNameSize;
                file.read(reinterpret_cast<char*>(&displayNameSize), sizeof(displayNameSize));
                entry.serviceId.displayName.resize(displayNameSize);
                file.read(&entry.serviceId.displayName[0], displayNameSize);
                uint32_t linkedAddressSize;
                file.read(reinterpret_cast<char*>(&linkedAddressSize), sizeof(linkedAddressSize));
                entry.serviceId.linkedAddress.resize(linkedAddressSize);
                file.read(&entry.serviceId.linkedAddress[0], linkedAddressSize);
                uint32_t hashedAddressSize;
                file.read(reinterpret_cast<char*>(&hashedAddressSize), sizeof(hashedAddressSize));
                entry.serviceId.hashedAddress.resize(hashedAddressSize);
                file.read(&entry.serviceId.hashedAddress[0], hashedAddressSize);
            }
            
            // Read constant stake proof data
            file.read(reinterpret_cast<char*>(&entry.constantProofType), sizeof(entry.constantProofType));
            uint32_t crossChainAddressSize;
            file.read(reinterpret_cast<char*>(&crossChainAddressSize), sizeof(crossChainAddressSize));
            entry.crossChainAddress.resize(crossChainAddressSize);
            file.read(&entry.crossChainAddress[0], crossChainAddressSize);
            file.read(reinterpret_cast<char*>(&entry.constantStakeAmount), sizeof(entry.constantStakeAmount));
            file.read(reinterpret_cast<char*>(&entry.constantProofExpiry), sizeof(entry.constantProofExpiry));
            
            m_eldernodes[entry.eldernodePublicKey] = entry;
        }
        
        logger(INFO) << "Loaded " << eldernodeCount << " Eldernodes from storage";
        return true;
    } catch (const std::exception& e) {
        logger(ERROR) << "Exception during load: " << e.what();
        return false;
    }
}

bool EldernodeIndexManager::clearStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_eldernodes.clear();
    m_stakeProofs.clear();
    m_consensusParticipants.clear();
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Cleared all Eldernode data";
    return true;
}

void EldernodeIndexManager::setConsensusThresholds(const ConsensusThresholds& thresholds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consensusThresholds = thresholds;
}

ConsensusThresholds EldernodeIndexManager::getConsensusThresholds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_consensusThresholds;
}

void EldernodeIndexManager::setElderfierConfig(const ElderfierServiceConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_elderfierConfig = config;
}

ElderfierServiceConfig EldernodeIndexManager::getElderfierConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_elderfierConfig;
}

bool EldernodeIndexManager::generateFreshProof(const Crypto::PublicKey& publicKey, const std::string& feeAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for proof generation: " << Common::podToHex(publicKey);
        return false;
    }
    
    const auto& entry = it->second;
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    EldernodeStakeProof proof;
    proof.eldernodePublicKey = publicKey;
    proof.stakeAmount = entry.stakeAmount;
    proof.timestamp = timestamp;
    proof.feeAddress = feeAddress;
    proof.tier = entry.tier;
    proof.serviceId = entry.serviceId;
    proof.stakeHash = calculateStakeHash(publicKey, entry.stakeAmount, timestamp);
    
    // Generate signature (placeholder for now)
    proof.proofSignature.resize(64, 0);
    
    m_stakeProofs[publicKey].push_back(proof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (entry.tier == EldernodeTier::BASIC) ? "Basic" : "Elderfier";
    logger(INFO) << "Generated fresh proof for " << tierName << " Eldernode: " << Common::podToHex(publicKey);
    
    return true;
}

bool EldernodeIndexManager::regenerateAllProofs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    bool success = true;
    for (const auto& pair : m_eldernodes) {
        if (!generateFreshProof(pair.first, pair.second.feeAddress)) {
            success = false;
        }
    }
    
    logger(INFO) << "Regenerated proofs for all Eldernodes, success: " << success;
    return success;
}

// Constant stake proof management for cross-chain validation
bool EldernodeIndexManager::createConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                                     ConstantStakeProofType proofType,
                                                     const std::string& crossChainAddress,
                                                     uint64_t stakeAmount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for constant proof creation: " << Common::podToHex(publicKey);
        return false;
    }
    
    const auto& entry = it->second;
    
    // Only Elderfier nodes can create constant stake proofs
    if (entry.tier != EldernodeTier::ELDERFIER) {
        logger(ERROR) << "Only Elderfier nodes can create constant stake proofs: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Validate constant proof type
    if (proofType == ConstantStakeProofType::NONE) {
        logger(ERROR) << "Invalid constant proof type: NONE";
        return false;
    }
    
    // Check if constant proof type is enabled
    if (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR && 
        !m_elderfierConfig.constantProofConfig.enableElderadoC0DL3Validator) {
        logger(ERROR) << "Elderado C0DL3 validator constant proofs are disabled";
        return false;
    }
    
    // Validate stake amount
    uint64_t requiredStake = m_elderfierConfig.constantProofConfig.getRequiredStakeAmount(proofType);
    if (stakeAmount < requiredStake) {
        logger(ERROR) << "Insufficient stake for constant proof: " << stakeAmount 
                     << " < " << requiredStake << " required";
        return false;
    }
    
    // Check if Eldernode has sufficient total stake
    if (entry.stakeAmount < stakeAmount) {
        logger(ERROR) << "Eldernode total stake insufficient for constant proof: " 
                     << entry.stakeAmount << " < " << stakeAmount;
        return false;
    }
    
    // Check for existing constant proof of same type
    auto existingProofs = getConstantStakeProofs(publicKey);
    for (const auto& existingProof : existingProofs) {
        if (existingProof.constantProofType == proofType && !existingProof.isConstantProofExpired()) {
            logger(ERROR) << "Constant proof of type " << static_cast<int>(proofType) 
                         << " already exists for Eldernode: " << Common::podToHex(publicKey);
            return false;
        }
    }
    
    // Create constant stake proof
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    EldernodeStakeProof constantProof;
    constantProof.eldernodePublicKey = publicKey;
    constantProof.stakeAmount = stakeAmount;
    constantProof.timestamp = timestamp;
    constantProof.feeAddress = entry.feeAddress;
    constantProof.tier = entry.tier;
    constantProof.serviceId = entry.serviceId;
    constantProof.constantProofType = proofType;
    constantProof.crossChainAddress = crossChainAddress;
    constantProof.constantStakeAmount = stakeAmount;
    
    // Set expiry based on configuration
    if (m_elderfierConfig.constantProofConfig.constantProofValidityPeriod > 0) {
        constantProof.constantProofExpiry = timestamp + m_elderfierConfig.constantProofConfig.constantProofValidityPeriod;
    } else {
        constantProof.constantProofExpiry = 0; // Never expires
    }
    
    constantProof.stakeHash = calculateStakeHash(publicKey, stakeAmount, timestamp);
    
    // Generate signature
    constantProof.proofSignature.resize(64, 0);
    
    // Add to stake proofs
    m_stakeProofs[publicKey].push_back(constantProof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string proofTypeName = (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) 
                               ? "Elderado C0DL3 Validator" : "Unknown";
    
    logger(INFO) << "Created constant stake proof for " << proofTypeName 
                << " - Eldernode: " << Common::podToHex(publicKey)
                << " Amount: " << stakeAmount << " XFG"
                << " Cross-chain address: " << crossChainAddress;
    
    return true;
}

bool EldernodeIndexManager::renewConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                                    ConstantStakeProofType proofType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_elderfierConfig.constantProofConfig.allowConstantProofRenewal) {
        logger(ERROR) << "Constant proof renewal is disabled";
        return false;
    }
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for constant proof renewal: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Find existing constant proof
    auto existingProofs = getConstantStakeProofs(publicKey);
    EldernodeStakeProof* existingProof = nullptr;
    
    for (auto& proof : m_stakeProofs[publicKey]) {
        if (proof.constantProofType == proofType) {
            existingProof = &proof;
            break;
        }
    }
    
    if (!existingProof) {
        logger(ERROR) << "No existing constant proof of type " << static_cast<int>(proofType) 
                     << " found for Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Update timestamp and expiry
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    existingProof->timestamp = timestamp;
    existingProof->stakeHash = calculateStakeHash(publicKey, existingProof->stakeAmount, timestamp);
    
    if (m_elderfierConfig.constantProofConfig.constantProofValidityPeriod > 0) {
        existingProof->constantProofExpiry = timestamp + m_elderfierConfig.constantProofConfig.constantProofValidityPeriod;
    }
    
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string proofTypeName = (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) 
                               ? "Elderado C0DL3 Validator" : "Unknown";
    
    logger(INFO) << "Renewed constant stake proof for " << proofTypeName 
                << " - Eldernode: " << Common::podToHex(publicKey);
    
    return true;
}

bool EldernodeIndexManager::revokeConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                                     ConstantStakeProofType proofType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_stakeProofs.find(publicKey);
    if (it == m_stakeProofs.end()) {
        logger(ERROR) << "No stake proofs found for Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Find and remove constant proof
    auto& proofs = it->second;
    auto proofIt = proofs.begin();
    bool found = false;
    
    while (proofIt != proofs.end()) {
        if (proofIt->constantProofType == proofType) {
            found = true;
            std::string proofTypeName = (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) 
                                       ? "Elderado C0DL3 Validator" : "Unknown";
            
            logger(INFO) << "Revoked constant stake proof for " << proofTypeName 
                        << " - Eldernode: " << Common::podToHex(publicKey);
            
            proofIt = proofs.erase(proofIt);
            break;
        } else {
            ++proofIt;
        }
    }
    
    if (!found) {
        logger(ERROR) << "No constant proof of type " << static_cast<int>(proofType) 
                     << " found for Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    m_lastUpdate = std::chrono::system_clock::now();
    return true;
}

std::vector<EldernodeStakeProof> EldernodeIndexManager::getConstantStakeProofs(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_stakeProofs.find(publicKey);
    if (it == m_stakeProofs.end()) {
        return {};
    }
    
    std::vector<EldernodeStakeProof> constantProofs;
    for (const auto& proof : it->second) {
        if (proof.isConstantProof()) {
            constantProofs.push_back(proof);
        }
    }
    
    return constantProofs;
}

std::vector<EldernodeStakeProof> EldernodeIndexManager::getConstantStakeProofsByType(ConstantStakeProofType proofType) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<EldernodeStakeProof> constantProofs;
    for (const auto& pair : m_stakeProofs) {
        for (const auto& proof : pair.second) {
            if (proof.constantProofType == proofType && proof.isConstantProof()) {
                constantProofs.push_back(proof);
            }
        }
    }
    
    return constantProofs;
}

// Slashing functionality
bool EldernodeIndexManager::slashEldernode(const Crypto::PublicKey& publicKey, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_elderfierConfig.slashingConfig.enableSlashing) {
        logger(WARNING) << "Slashing is disabled";
        return false;
    }
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for slashing: " << Common::podToHex(publicKey);
        return false;
    }
    
    const auto& entry = it->second;
    if (entry.tier != EldernodeTier::ELDERFIER) {
        logger(ERROR) << "Cannot slash Basic Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    uint64_t slashedAmount = (entry.stakeAmount * m_elderfierConfig.slashingConfig.slashingPercentage) / 100;
    
    // Update the Eldernode entry with reduced stake
    ENindexEntry updatedEntry = entry;
    updatedEntry.stakeAmount -= slashedAmount;
    
    // Handle slashed amount based on destination
    switch (m_elderfierConfig.slashingConfig.destination) {
        case SlashingDestination::BURN:
            logger(INFO) << "Burned " << slashedAmount << " XFG from Eldernode: " << Common::podToHex(publicKey);
            break;
            
        case SlashingDestination::TREASURY:
            logger(INFO) << "Sent " << slashedAmount << " XFG to treasury from Eldernode: " << Common::podToHex(publicKey);
            break;
            
        case SlashingDestination::REDISTRIBUTE:
            redistributeSlashedStake(slashedAmount);
            logger(INFO) << "Redistributed " << slashedAmount << " XFG to other Eldernodes from: " << Common::podToHex(publicKey);
            break;
            
        case SlashingDestination::CHARITY:
            logger(INFO) << "Sent " << slashedAmount << " XFG to charity from Eldernode: " << Common::podToHex(publicKey);
            break;
    }
    
    // Update the Eldernode
    it->second = updatedEntry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Slashed Eldernode: " << Common::podToHex(publicKey) 
                << " amount: " << slashedAmount << " reason: " << reason;
    
    return true;
}

// Private helper methods

bool EldernodeIndexManager::validateEldernodeEntry(const ENindexEntry& entry) const {
    if (entry.feeAddress.empty()) {
        return false;
    }
    
    // Check tier-specific requirements
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (entry.stakeAmount < m_elderfierConfig.minimumStakeAmount) {
            logger(ERROR) << "Elderfier node stake too low: " << entry.stakeAmount 
                         << " < " << m_elderfierConfig.minimumStakeAmount << " (800 XFG)";
            return false;
        }
    } else if (entry.tier == EldernodeTier::BASIC) {
        // Basic Eldernodes have no stake requirement (--set-fee-address only)
        if (entry.stakeAmount != 0) {
            logger(ERROR) << "Basic Eldernode should have no stake: " << entry.stakeAmount;
            return false;
        }
    }
    
    return true;
}

bool EldernodeIndexManager::validateStakeProof(const EldernodeStakeProof& proof) const {
    if (proof.feeAddress.empty()) {
        return false;
    }
    
    if (proof.proofSignature.empty()) {
        return false;
    }
    
    // Check tier-specific requirements
    if (proof.tier == EldernodeTier::ELDERFIER) {
        if (proof.stakeAmount < m_elderfierConfig.minimumStakeAmount) {
            return false;
        }
        if (!proof.serviceId.isValid()) {
            return false;
        }
    } else if (proof.tier == EldernodeTier::BASIC) {
        // Basic Eldernodes have no stake requirement
        if (proof.stakeAmount != 0) {
            return false;
        }
    }
    
    // Validate constant proof if applicable
    if (proof.isConstantProof()) {
        // Check if constant proof type is enabled
        if (proof.constantProofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR && 
            !m_elderfierConfig.constantProofConfig.enableElderadoC0DL3Validator) {
            return false;
        }
        
        // Validate constant stake amount
        uint64_t requiredStake = m_elderfierConfig.constantProofConfig.getRequiredStakeAmount(proof.constantProofType);
        if (proof.constantStakeAmount < requiredStake) {
            return false;
        }
        
        // Validate cross-chain address
        if (proof.crossChainAddress.empty()) {
            return false;
        }
        
        // Check if constant proof is expired
        if (proof.isConstantProofExpired()) {
            return false;
        }
    }
    
    // Verify stake hash
    Crypto::Hash expectedHash = calculateStakeHash(proof.eldernodePublicKey, proof.stakeAmount, proof.timestamp);
    if (proof.stakeHash != expectedHash) {
        return false;
    }
    
    return true;
}

bool EldernodeIndexManager::validateElderfierServiceId(const ElderfierServiceId& serviceId) const {
    if (!serviceId.isValid()) {
        return false;
    }
    
    switch (serviceId.type) {
        case ServiceIdType::CUSTOM_NAME:
            if (!m_elderfierConfig.isValidCustomName(serviceId.identifier)) {
                logger(ERROR) << "Invalid custom name: " << serviceId.identifier;
                return false;
            }
            if (m_elderfierConfig.isCustomNameReserved(serviceId.identifier)) {
                logger(ERROR) << "Custom name is reserved: " << serviceId.identifier;
                return false;
            }
            break;
            
        case ServiceIdType::HASHED_ADDRESS:
            if (!m_elderfierConfig.allowHashedAddresses) {
                logger(ERROR) << "Hashed addresses not allowed";
                return false;
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

bool EldernodeIndexManager::hasServiceIdConflict(const ElderfierServiceId& serviceId, const Crypto::PublicKey& excludeKey) const {
    for (const auto& pair : m_eldernodes) {
        if (pair.first != excludeKey && 
            pair.second.tier == EldernodeTier::ELDERFIER &&
            pair.second.serviceId.identifier == serviceId.identifier) {
            return true;
        }
    }
    return false;
}

Crypto::Hash EldernodeIndexManager::calculateStakeHash(const Crypto::PublicKey& publicKey, uint64_t amount, uint64_t timestamp) const {
    std::string data = Common::podToHex(publicKey) + std::to_string(amount) + std::to_string(timestamp);
    Crypto::Hash hash;
    Crypto::cn_fast_hash(data.data(), data.size(), hash);
    return hash;
}

std::vector<uint8_t> EldernodeIndexManager::aggregateSignatures(const std::vector<std::vector<uint8_t>>& signatures) const {
    // Simple aggregation - concatenate all signatures
    std::vector<uint8_t> result;
    for (const auto& sig : signatures) {
        result.insert(result.end(), sig.begin(), sig.end());
    }
    return result;
}

void EldernodeIndexManager::redistributeSlashedStake(uint64_t slashedAmount) {
    // Simple redistribution to active Elderfier nodes
    std::vector<ENindexEntry> activeElderfierNodes;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            activeElderfierNodes.push_back(pair.second);
        }
    }
    
    if (activeElderfierNodes.empty()) {
        logger(WARNING) << "No active Elderfier nodes for stake redistribution";
        return;
    }
    
    uint64_t amountPerNode = slashedAmount / activeElderfierNodes.size();
    uint64_t remainder = slashedAmount % activeElderfierNodes.size();
    
    for (size_t i = 0; i < activeElderfierNodes.size(); ++i) {
        uint64_t bonus = (i < remainder) ? 1 : 0;
        uint64_t totalBonus = amountPerNode + bonus;
        
        // Update the Eldernode's stake
        auto it = m_eldernodes.find(activeElderfierNodes[i].eldernodePublicKey);
        if (it != m_eldernodes.end()) {
            it->second.stakeAmount += totalBonus;
        }
    }
    
    logger(INFO) << "Redistributed " << slashedAmount << " XFG to " << activeElderfierNodes.size() << " active Elderfier nodes";
}

} // namespace CryptoNote
