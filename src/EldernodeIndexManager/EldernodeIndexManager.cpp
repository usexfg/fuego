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

namespace {
    LoggerRef logger(Logging::getLogger("EldernodeIndexManager"));
}

EldernodeIndexManager::EldernodeIndexManager() 
    : m_consensusThresholds(ConsensusThresholds::getDefault())
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
    
    m_eldernodes[entry.eldernodePublicKey] = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Added Eldernode: " << Common::podToHex(entry.eldernodePublicKey) 
                << " with stake: " << entry.stakeAmount;
    return true;
}

bool EldernodeIndexManager::removeEldernode(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(WARNING) << "Eldernode not found for removal: " << Common::podToHex(publicKey);
        return false;
    }
    
    m_eldernodes.erase(it);
    m_stakeProofs.erase(publicKey);
    m_consensusParticipants.erase(publicKey);
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Removed Eldernode: " << Common::podToHex(publicKey);
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
    
    it->second = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Updated Eldernode: " << Common::podToHex(entry.eldernodePublicKey);
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

bool EldernodeIndexManager::addStakeProof(const EldernodeStakeProof& proof) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateStakeProof(proof)) {
        logger(ERROR) << "Invalid stake proof for public key: " << Common::podToHex(proof.eldernodePublicKey);
        return false;
    }
    
    auto& proofs = m_stakeProofs[proof.eldernodePublicKey];
    proofs.push_back(proof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Added stake proof for Eldernode: " << Common::podToHex(proof.eldernodePublicKey)
                << " amount: " << proof.stakeAmount;
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
    
    logger(INFO) << "Added consensus participant: " << Common::podToHex(participant.publicKey);
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
    
    // Get active participants
    std::vector<EldernodeConsensusParticipant> activeParticipants;
    for (const auto& pair : m_consensusParticipants) {
        if (pair.second.isActive) {
            activeParticipants.push_back(pair.second);
        }
    }
    
    if (activeParticipants.size() < thresholds.minimumEldernodes) {
        logger(WARNING) << "Insufficient active Eldernodes for consensus: " 
                       << activeParticipants.size() << "/" << thresholds.minimumEldernodes;
        return result;
    }
    
    // Simulate consensus voting (in real implementation, this would involve actual communication)
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
    proof.stakeHash = calculateStakeHash(publicKey, entry.stakeAmount, timestamp);
    
    // Generate signature (placeholder for now)
    proof.proofSignature.resize(64, 0);
    
    m_stakeProofs[publicKey].push_back(proof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Generated fresh proof for Eldernode: " << Common::podToHex(publicKey);
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

// Private helper methods

bool EldernodeIndexManager::validateEldernodeEntry(const ENindexEntry& entry) const {
    if (entry.stakeAmount == 0) {
        return false;
    }
    
    if (entry.feeAddress.empty()) {
        return false;
    }
    
    return true;
}

bool EldernodeIndexManager::validateStakeProof(const EldernodeStakeProof& proof) const {
    if (proof.stakeAmount == 0) {
        return false;
    }
    
    if (proof.feeAddress.empty()) {
        return false;
    }
    
    if (proof.proofSignature.empty()) {
        return false;
    }
    
    // Verify stake hash
    Crypto::Hash expectedHash = calculateStakeHash(proof.eldernodePublicKey, proof.stakeAmount, proof.timestamp);
    if (proof.stakeHash != expectedHash) {
        return false;
    }
    
    return true;
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

} // namespace CryptoNote
