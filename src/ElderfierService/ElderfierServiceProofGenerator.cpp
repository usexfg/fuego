// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/ElderfierServiceProofGenerator.h"
#include "../../include/ElderfierServiceTypesSimple.h"
#include <cstring>
#include <functional>
#include <ctime>

namespace CryptoNote {

ElderfierServiceProofGenerator::ElderfierServiceProofGenerator(ElderfierServiceKernelBuilder& kernelBuilder)
    : m_kernelBuilder(kernelBuilder)
    , m_proofSequenceCounter(0) {
}

bool ElderfierServiceProofGenerator::generateStakeProof(const std::string& feeAddress,
                                                       uint64_t minimumStake,
                                                       uint64_t currentHeight,
                                                       ElderfierServiceProof& proof) {
    try {
        // Check cache first
        if (getCachedProof(feeAddress, currentHeight, proof)) {
            return true;
        }
        
        // Build stake kernel
        ElderfierServiceKernel kernel;
        if (!m_kernelBuilder.buildKernel(feeAddress, minimumStake, currentHeight, kernel)) {
            return false;
        }
        
        // Generate proof from kernel
        if (!generateProofFromKernel(kernel, feeAddress, proof)) {
            return false;
        }
        
        // Cache the proof
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_proofCache[feeAddress] = proof;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool ElderfierServiceProofGenerator::getCachedProof(const std::string& feeAddress,
                                                   uint64_t currentHeight,
                                                   ElderfierServiceProof& proof) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    auto it = m_proofCache.find(feeAddress);
    if (it != m_proofCache.end()) {
        // Check if cached proof is still valid
        if (isProofValid(it->second, currentHeight)) {
            proof = it->second;
            return true;
        }
    }
    
    return false;
}

void ElderfierServiceProofGenerator::clearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_proofCache.clear();
}

size_t ElderfierServiceProofGenerator::getCacheSize() const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_proofCache.size();
}

bool ElderfierServiceProofGenerator::generateProofFromKernel(const ElderfierServiceKernel& kernel,
                                                            const std::string& feeAddress,
                                                            ElderfierServiceProof& proof) {
    // Initialize proof structure
    proof = ElderfierServiceProof();
    proof.proof_version = ELDERFIER_PROOF_VERSION_1;
    proof.service_type = ELDERFIER_SERVICE_TYPE_BASIC;
    proof.proof_timestamp = static_cast<uint64_t>(std::time(nullptr));
    proof.proof_sequence = getNextProofSequence();
    proof.minimum_stake_atomic = kernel.minimum_stake_atomic;
    
    // Set hashes (simplified for now)
    std::fill(proof.service_modifier_hash.begin(), proof.service_modifier_hash.end(), 0);
    proof.fee_address_hash = kernel.fee_address_hash;
    
    // Generate proof signature
    if (!generateProofSignature(kernel, feeAddress, proof)) {
        return false;
    }
    
    // Calculate final proof hash
    proof.proof_hash = calculateProofHash(proof);
    
    return true;
}

bool ElderfierServiceProofGenerator::generateProofSignature(const ElderfierServiceKernel& kernel,
                                                           const std::string& feeAddress,
                                                           ElderfierServiceProof& proof) {
    // Create proof payload
    std::string proofPayload = 
        std::to_string(proof.minimum_stake_atomic) + 
        std::to_string(proof.proof_timestamp) + 
        std::to_string(proof.proof_sequence);
    
    // Generate cryptographic proof key pair
    std::array<uint8_t, 32> privateKey;
    std::array<uint8_t, 32> publicKey;
    
    if (!generateProofKeyPair(feeAddress, kernel, privateKey, publicKey)) {
        return false;
    }
    
    // Sign the proof payload
    std::array<uint8_t, 64> signature;
    if (!signProofPayload(proofPayload, privateKey, signature)) {
        return false;
    }
    
    proof.proof_signature = signature;
    proof.proof_public_key = publicKey;
    
    return true;
}

std::array<uint8_t, 32> ElderfierServiceProofGenerator::calculateProofHash(const ElderfierServiceProof& proof) const {
    // Hash the entire proof structure (simplified for now)
    std::string proofData = 
        std::to_string(proof.proof_timestamp) + 
        std::to_string(proof.proof_sequence) + 
        std::to_string(proof.minimum_stake_atomic);
    
    std::hash<std::string> hasher;
    size_t hashValue = hasher(proofData);
    
    std::array<uint8_t, 32> hash;
    std::fill(hash.begin(), hash.end(), 0);
    std::memcpy(hash.data(), &hashValue, sizeof(hashValue));
    
    return hash;
}

uint64_t ElderfierServiceProofGenerator::getNextProofSequence() {
    return ++m_proofSequenceCounter;
}

bool ElderfierServiceProofGenerator::isProofValid(const ElderfierServiceProof& proof, uint64_t currentHeight) const {
    // Check if proof is still fresh
    uint64_t currentTime = static_cast<uint64_t>(std::time(nullptr));
    if (currentTime - proof.proof_timestamp > ELDERFIER_SERVICE_PROOF_WINDOW) {
        return false; // Proof too old
    }
    
    return true;
}

bool ElderfierServiceProofGenerator::generateProofKeyPair(const std::string& feeAddress,
                                                         const ElderfierServiceKernel& kernel,
                                                         std::array<uint8_t, 32>& privateKey,
                                                         std::array<uint8_t, 32>& publicKey) const {
    // Generate deterministic keys from address and kernel (simplified for now)
    std::string keyData = feeAddress + 
                          std::to_string(kernel.kernel_timestamp) + 
                          std::to_string(kernel.minimum_stake_atomic);
    
    std::hash<std::string> hasher;
    size_t keyHash = hasher(keyData);
    
    // Convert hash to key pair (simplified)
    std::fill(privateKey.begin(), privateKey.end(), 0);
    std::fill(publicKey.begin(), publicKey.end(), 0);
    
    std::memcpy(privateKey.data(), &keyHash, sizeof(keyHash));
    std::memcpy(publicKey.data(), &keyHash, sizeof(keyHash));
    
    return true;
}

bool ElderfierServiceProofGenerator::signProofPayload(const std::string& payload,
                                                      const std::array<uint8_t, 32>& privateKey,
                                                      std::array<uint8_t, 64>& signature) const {
    // Simplified signing for now - in production, use proper cryptographic signing
    std::string signatureData = payload + std::to_string(privateKey.data());
    
    std::hash<std::string> hasher;
    size_t signatureHash = hasher(signatureData);
    
    std::fill(signature.begin(), signature.end(), 0);
    std::memcpy(signature.data(), &signatureHash, sizeof(signatureHash));
    
    return true;
}



} // namespace CryptoNote
