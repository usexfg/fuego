// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/ElderfierServiceModifierBuilder.h"
#include "../../include/ElderfierServiceTypesSimple.h"
#include "../CryptoNoteCore/Core.h"
#include "../CryptoNoteCore/Blockchain.h"
#include "../CryptoNoteCore/BlockchainMessages.h"
#include "../Logging/LoggerRef.h"
#include <ctime>
#include <cstring>

namespace CryptoNote {

ElderfierServiceModifierBuilder::ElderfierServiceModifierBuilder(CryptoNote::core& core)
    : m_core(core)
    , m_lastUpdateHeight(0) {
    
    // Initialize with genesis modifier
    initializeFromGenesis(m_currentModifier);
}

bool ElderfierServiceModifierBuilder::buildServiceModifier(ElderfierServiceModifier& sm, uint64_t currentHeight) {
    std::lock_guard<std::mutex> lock(m_modifierMutex);
    
    // Check if we need to update modifier
    if (currentHeight - m_lastUpdateHeight < ELDERFIER_SERVICE_MODIFIER_INTERVAL) {
        sm = m_currentModifier;
        return true;
    }
    
    // Build new service modifier
    if (!updateFromBlockchain(sm, currentHeight)) {
        return false;
    }
    
    // Update current modifier
    m_currentModifier = sm;
    m_lastUpdateHeight = currentHeight;
    
    return true;
}

const ElderfierServiceModifier& ElderfierServiceModifierBuilder::getCurrentModifier() const {
    std::lock_guard<std::mutex> lock(m_modifierMutex);
    return m_currentModifier;
}

bool ElderfierServiceModifierBuilder::needsUpdate(uint64_t currentHeight) const {
    std::lock_guard<std::mutex> lock(m_modifierMutex);
    return (currentHeight - m_lastUpdateHeight) >= ELDERFIER_SERVICE_MODIFIER_INTERVAL;
}

bool ElderfierServiceModifierBuilder::forceUpdate(uint64_t currentHeight) {
    std::lock_guard<std::mutex> lock(m_modifierMutex);
    
    ElderfierServiceModifier newModifier;
    if (!updateFromBlockchain(newModifier, currentHeight)) {
        return false;
    }
    
    m_currentModifier = newModifier;
    m_lastUpdateHeight = currentHeight;
    
    return true;
}

bool ElderfierServiceModifierBuilder::initializeFromGenesis(ElderfierServiceModifier& sm) {
    // Initialize with genesis values
    sm = ElderfierServiceModifier();
    sm.last_pow_block_height = 0;
    sm.modifier_timestamp = static_cast<uint64_t>(std::time(nullptr));
    sm.modifier_sequence = 0;
    
    // Set genesis block hash (all zeros for now)
    std::fill(sm.last_pow_block_hash.begin(), sm.last_pow_block_hash.end(), 0);
    
    return true;
}

bool ElderfierServiceModifierBuilder::updateFromBlockchain(ElderfierServiceModifier& sm, uint64_t currentHeight) {
    try {
        // Get last PoW block from core
        auto lastBlock = m_core.getTopBlock();
        if (!lastBlock) {
            return false;
        }
        
        // Initialize new modifier
        sm = ElderfierServiceModifier();
        
        // Set PoW block data
        sm.last_pow_block_height = currentHeight;
        sm.modifier_timestamp = static_cast<uint64_t>(std::time(nullptr));
        sm.modifier_sequence = m_currentModifier.modifier_sequence + 1;
        
        // Calculate hash of last block (simplified for now)
        // In production, this should use proper block hashing
        std::string blockData = std::to_string(currentHeight) + std::to_string(sm.modifier_timestamp);
        std::hash<std::string> hasher;
        size_t hashValue = hasher(blockData);
        
        // Convert to byte array
        std::memcpy(sm.last_pow_block_hash.data(), &hashValue, sizeof(hashValue));
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

std::array<uint8_t, 32> ElderfierServiceModifierBuilder::calculateModifierHash(const ElderfierServiceModifier& modifier) const {
    // Calculate hash of modifier (simplified for now)
    // In production, this should use proper cryptographic hashing
    std::string modifierData = 
        std::to_string(modifier.last_pow_block_height) + 
        std::to_string(modifier.modifier_timestamp) + 
        std::to_string(modifier.modifier_sequence);
    
    std::hash<std::string> hasher;
    size_t hashValue = hasher(modifierData);
    
    std::array<uint8_t, 32> hash;
    std::fill(hash.begin(), hash.end(), 0);
    std::memcpy(hash.data(), &hashValue, sizeof(hashValue));
    
    return hash;
}

} // namespace CryptoNote
