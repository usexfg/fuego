// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/ElderfierServiceKernelBuilder.h"
#include "../../include/ElderfierServiceTypesSimple.h"
#include <cstring>
#include <functional>

namespace CryptoNote {

ElderfierServiceKernelBuilder::ElderfierServiceKernelBuilder(ElderfierServiceModifierBuilder& modifierBuilder)
    : m_modifierBuilder(modifierBuilder) {
}

bool ElderfierServiceKernelBuilder::buildKernel(const std::string& feeAddress,
                                                uint64_t minimumStake,
                                                uint64_t currentHeight,
                                                ElderfierServiceKernel& kernel) {
    try {
        // Validate parameters first
        if (!validateKernelParameters(feeAddress, minimumStake)) {
            return false;
        }
        
        // Get current service modifier
        ElderfierServiceModifier stakeModifier;
        if (!m_modifierBuilder.buildServiceModifier(stakeModifier, currentHeight)) {
            return false;
        }
        
        // Build kernel
        kernel = ElderfierServiceKernel();
        kernel.service_modifier = stakeModifier;
        kernel.kernel_timestamp = static_cast<uint64_t>(std::time(nullptr));
        kernel.fee_address_hash = hashFeeAddress(feeAddress);
        kernel.minimum_stake_atomic = minimumStake;
        
        // Generate unique key image for this stake
        kernel.stake_key_image = generateStakeKeyImage(feeAddress, stakeModifier);
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool ElderfierServiceKernelBuilder::validateKernelParameters(const std::string& feeAddress, uint64_t minimumStake) const {
    // Check fee address format
    if (!isValidFeeAddress(feeAddress)) {
        return false;
    }
    
    // Check minimum stake requirement
    if (minimumStake == 0) {
        return false;
    }
    
    // Check if minimum stake meets Elderfier requirement (800 XFG)
    uint64_t requiredStake = 800000000000; // 800 XFG in atomic units (7 decimal places)
    if (minimumStake < requiredStake) {
        return false;
    }
    
    return true;
}

std::array<uint8_t, 32> ElderfierServiceKernelBuilder::generateStakeKeyImage(const std::string& feeAddress,
                                                                             const ElderfierServiceModifier& modifier) const {
    // Create deterministic key image from address and modifier
    std::string keyImageData = feeAddress + 
                               std::to_string(modifier.last_pow_block_height) + 
                               std::to_string(modifier.modifier_sequence);
    
    // Hash the data to create key image
    std::hash<std::string> hasher;
    size_t hashValue = hasher(keyImageData);
    
    std::array<uint8_t, 32> keyImage;
    std::fill(keyImage.begin(), keyImage.end(), 0);
    std::memcpy(keyImage.data(), &hashValue, sizeof(hashValue));
    
    return keyImage;
}

std::array<uint8_t, 32> ElderfierServiceKernelBuilder::hashFeeAddress(const std::string& feeAddress) const {
    // Hash fee address for privacy
    std::hash<std::string> hasher;
    size_t hashValue = hasher(feeAddress);
    
    std::array<uint8_t, 32> addressHash;
    std::fill(addressHash.begin(), addressHash.end(), 0);
    std::memcpy(addressHash.data(), &hashValue, sizeof(hashValue));
    
    return addressHash;
}

bool ElderfierServiceKernelBuilder::isValidFeeAddress(const std::string& feeAddress) const {
    // Basic validation - check if address is not empty and has reasonable length
    if (feeAddress.empty()) {
        return false;
    }
    
    // Check if address has reasonable length (CryptoNote addresses are typically 95+ characters)
    if (feeAddress.length() < 10) {
        return false;
    }
    
    // Check if address contains only valid characters (basic check)
    for (char c : feeAddress) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    
    return true;
}

} // namespace CryptoNote
