// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "ElderfierServiceKernelBuilder.h"

namespace CryptoNote {

/**
 * @brief Service access proof generator
 * 
 * This class generates cryptographic proofs that prove sufficient
 * stake exists for Elderfier service access without revealing
 * the exact balance amount.
 * 
 * NOT for blockchain consensus - only for service access control.
 */
class ElderfierServiceProofGenerator {
private:
    ElderfierServiceKernelBuilder& m_kernelBuilder;
    std::unordered_map<std::string, ElderfierServiceProof> m_proofCache;
    mutable std::mutex m_cacheMutex;
    std::atomic<uint64_t> m_proofSequenceCounter;
    
public:
    /**
     * @brief Constructor
     * @param kernelBuilder Reference to the kernel builder
     */
    explicit ElderfierServiceProofGenerator(ElderfierServiceKernelBuilder& kernelBuilder);
    
    /**
     * @brief Destructor
     */
    ~ElderfierServiceProofGenerator() = default;
    
    /**
     * @brief Generate service access proof
     * 
     * Creates a cryptographic proof that proves sufficient stake exists
     * for Elderfier service access without revealing exact balance.
     * 
     * @param feeAddress Fee address for service access
     * @param minimumStake Minimum stake required in atomic units
     * @param currentHeight Current blockchain height
     * @param proof Output proof
     * @return true if successful, false otherwise
     */
    bool generateStakeProof(const std::string& feeAddress,
                           uint64_t minimumStake,
                           uint64_t currentHeight,
                           ElderfierServiceProof& proof);
    
    /**
     * @brief Get cached proof if valid
     * 
     * @param feeAddress Fee address to get proof for
     * @param currentHeight Current blockchain height
     * @param proof Output proof if found and valid
     * @return true if valid cached proof found, false otherwise
     */
    bool getCachedProof(const std::string& feeAddress,
                       uint64_t currentHeight,
                       ElderfierServiceProof& proof);
    
    /**
     * @brief Clear proof cache
     */
    void clearCache();
    
    /**
     * @brief Get cache statistics
     * 
     * @return Number of cached proofs
     */
    size_t getCacheSize() const;
    
private:
    /**
     * @brief Generate proof from kernel
     * 
     * @param kernel Service access kernel
     * @param feeAddress Fee address
     * @param proof Output proof
     * @return true if successful, false otherwise
     */
    bool generateProofFromKernel(const ElderfierServiceKernel& kernel,
                                const std::string& feeAddress,
                                ElderfierServiceProof& proof);
    
    /**
     * @brief Generate proof signature
     * 
     * @param kernel Service access kernel
     * @param feeAddress Fee address
     * @param proof Proof to sign
     * @return true if successful, false otherwise
     */
    bool generateProofSignature(const ElderfierServiceKernel& kernel,
                               const std::string& feeAddress,
                               ElderfierServiceProof& proof);
    
    /**
     * @brief Calculate proof hash
     * 
     * @param proof Proof to hash
     * @return Hash of the proof
     */
    std::array<uint8_t, 32> calculateProofHash(const ElderfierServiceProof& proof) const;
    
    /**
     * @brief Get next proof sequence number
     * 
     * @return Next sequence number
     */
    uint64_t getNextProofSequence();
    
    /**
     * @brief Check if proof is still valid
     * 
     * @param proof Proof to check
     * @param currentHeight Current blockchain height
     * @return true if valid, false otherwise
     */
    bool isProofValid(const ElderfierServiceProof& proof, uint64_t currentHeight) const;
    
    /**
     * @brief Generate proof key pair
     * 
     * @param feeAddress Fee address
     * @param kernel Service access kernel
     * @param privateKey Output private key
     * @param publicKey Output public key
     * @return true if successful, false otherwise
     */
    bool generateProofKeyPair(const std::string& feeAddress,
                              const ElderfierServiceKernel& kernel,
                              std::array<uint8_t, 32>& privateKey,
                              std::array<uint8_t, 32>& publicKey) const;
    
    /**
     * @brief Sign proof payload
     * 
     * @param payload Payload to sign
     * @param privateKey Private key for signing
     * @param signature Output signature
     * @return true if successful, false otherwise
     */
    bool signProofPayload(const std::string& payload,
                         const std::array<uint8_t, 32>& privateKey,
                         std::array<uint8_t, 64>& signature) const;
};

} // namespace CryptoNote
