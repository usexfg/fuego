// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/ElderfierServiceProofVerifier.h"
#include "../../include/ElderfierServiceTypesSimple.h"
#include <cstring>
#include <functional>
#include <ctime>

namespace CryptoNote {

ElderfierServiceProofVerifier::ElderfierServiceProofVerifier(ElderfierServiceModifierBuilder& modifierBuilder)
    : m_modifierBuilder(modifierBuilder) {
}

bool ElderfierServiceProofVerifier::verifyServiceAccessProof(const ElderfierServiceProof& proof,
                                                            uint64_t currentHeight,
                                                            const std::string& expectedFeeAddress) {
    try {
        // Validate proof structure first
        if (!validateProofStructure(proof)) {
            return false;
        }
        
        // Check for replay attacks
        std::string proofHashStr(reinterpret_cast<const char*>(proof.proof_hash.data()), proof.proof_hash.size());
        if (checkReplayAttack(proofHashStr)) {
            return false;
        }
        
        // Verify service modifier
        if (!verifyServiceModifier(proof, currentHeight)) {
            return false;
        }
        
        // Verify proof timestamp
        if (!verifyProofTimestamp(proof)) {
            return false;
        }
        
        // Verify fee address hash
        if (!verifyFeeAddressHash(proof, expectedFeeAddress)) {
            return false;
        }
        
        // Verify proof signature
        if (!verifyProofSignature(proof)) {
            return false;
        }
        
        // Mark proof as used to prevent replay
        markProofAsUsed(proofHashStr);
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool ElderfierServiceProofVerifier::isProofAlreadyUsed(const std::string& proofHash) const {
    std::lock_guard<std::mutex> lock(m_usedProofsMutex);
    return m_usedProofHashes.find(proofHash) != m_usedProofHashes.end();
}

void ElderfierServiceProofVerifier::clearUsedProofs() {
    std::lock_guard<std::mutex> lock(m_usedProofsMutex);
    m_usedProofHashes.clear();
}

size_t ElderfierServiceProofVerifier::getUsedProofCount() const {
    std::lock_guard<std::mutex> lock(m_usedProofsMutex);
    return m_usedProofHashes.size();
}

bool ElderfierServiceProofVerifier::validateProofStructure(const ElderfierServiceProof& proof) const {
    // Check proof version
    if (proof.proof_version != ELDERFIER_PROOF_VERSION_1) {
        return false;
    }
    
    // Check service type
    if (proof.service_type != ELDERFIER_SERVICE_TYPE_BASIC) {
        return false;
    }
    
    // Check minimum stake requirement
    uint64_t requiredStake = 800000000000; // 800 XFG in atomic units
    if (proof.minimum_stake_atomic < requiredStake) {
        return false;
    }
    
    // Check that proof hash is not all zeros
    bool allZeros = true;
    for (uint8_t byte : proof.proof_hash) {
        if (byte != 0) {
            allZeros = false;
            break;
        }
    }
    if (allZeros) {
        return false;
    }
    
    return true;
}

bool ElderfierServiceProofVerifier::checkReplayAttack(const std::string& proofHash) {
    std::lock_guard<std::mutex> lock(m_usedProofsMutex);
    
    if (m_usedProofHashes.find(proofHash) != m_usedProofHashes.end()) {
        return true; // Proof already used
    }
    
    return false;
}

bool ElderfierServiceProofVerifier::verifyServiceModifier(const ElderfierServiceProof& proof, uint64_t currentHeight) {
    // Get current service modifier from builder
    ElderfierServiceModifier currentModifier;
    if (!m_modifierBuilder.buildServiceModifier(currentModifier, currentHeight)) {
        return false;
    }
    
    // Calculate hash of current modifier
    std::string modifierData = 
        std::to_string(currentModifier.last_pow_block_height) + 
        std::to_string(currentModifier.modifier_timestamp) + 
        std::to_string(currentModifier.modifier_sequence);
    
    std::hash<std::string> hasher;
    size_t expectedHash = hasher(modifierData);
    
    // Compare with proof's modifier hash (simplified comparison)
    size_t proofHash = 0;
    std::memcpy(&proofHash, proof.service_modifier_hash.data(), sizeof(proofHash));
    
    return expectedHash == proofHash;
}

bool ElderfierServiceProofVerifier::verifyProofTimestamp(const ElderfierServiceProof& proof) const {
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    
    // Check if proof is too old
    if (currentTime - proof.proof_timestamp > ELDERFIER_SERVICE_PROOF_WINDOW) {
        return false;
    }
    
    // Check if proof is from the future (with small tolerance)
    if (proof.proof_timestamp > currentTime + 300) { // 5 minutes tolerance
        return false;
    }
    
    return true;
}

bool ElderfierServiceProofVerifier::verifyFeeAddressHash(const ElderfierServiceProof& proof, const std::string& expectedFeeAddress) const {
    // Hash the expected fee address
    std::array<uint8_t, 32> expectedHash = hashFeeAddress(expectedFeeAddress);
    
    // Compare hashes
    return proof.fee_address_hash == expectedHash;
}

bool ElderfierServiceProofVerifier::verifyProofSignature(const ElderfierServiceProof& proof) const {
    // Create the payload that was signed
    std::string payload = 
        std::to_string(proof.minimum_stake_atomic) + 
        std::to_string(proof.proof_timestamp) + 
        std::to_string(proof.proof_sequence);
    
    // Verify signature (simplified for now)
    return verifySignature(payload, proof.proof_public_key, proof.proof_signature);
}

void ElderfierServiceProofVerifier::markProofAsUsed(const std::string& proofHash) {
    std::lock_guard<std::mutex> lock(m_usedProofsMutex);
    m_usedProofHashes.insert(proofHash);
    
    // Clean up old proofs periodically
    if (m_usedProofHashes.size() > 1000) {
        cleanupOldUsedProofs();
    }
}

void ElderfierServiceProofVerifier::cleanupOldUsedProofs() {
    // Simple cleanup - remove oldest proofs if we have too many
    // In production, this could be more sophisticated with timestamp tracking
    if (m_usedProofHashes.size() > 500) {
        // Remove half of the proofs (simplified cleanup)
        auto it = m_usedProofHashes.begin();
        for (size_t i = 0; i < m_usedProofHashes.size() / 2; ++i) {
            it = m_usedProofHashes.erase(it);
        }
    }
}

std::array<uint8_t, 32> ElderfierServiceProofVerifier::hashFeeAddress(const std::string& feeAddress) const {
    // Hash fee address for comparison
    std::hash<std::string> hasher;
    size_t hashValue = hasher(feeAddress);
    
    std::array<uint8_t, 32> addressHash;
    std::fill(addressHash.begin(), addressHash.end(), 0);
    std::memcpy(addressHash.data(), &hashValue, sizeof(hashValue));
    
    return addressHash;
}

bool ElderfierServiceProofVerifier::verifySignature(const std::string& payload,
                                                   const std::array<uint8_t, 32>& publicKey,
                                                   const std::array<uint8_t, 64>& signature) const {
    // Simplified signature verification for now
    // In production, this should use proper cryptographic verification
    
    // Create expected signature from payload and public key
    std::string signatureData = payload + "public_key";
    std::hash<std::string> hasher;
    size_t expectedHash = hasher(signatureData);
    
    // Extract actual signature hash
    size_t actualHash = 0;
    std::memcpy(&actualHash, signature.data(), sizeof(actualHash));
    
    return expectedHash == actualHash;
}

} // namespace CryptoNote
