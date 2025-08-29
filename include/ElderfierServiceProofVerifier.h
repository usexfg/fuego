// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <unordered_set>
#include <mutex>
#include "ElderfierServiceModifierBuilder.h"

namespace CryptoNote {

/**
 * @brief Service access proof verifier
 * 
 * This class verifies cryptographic proofs that prove sufficient
 * stake exists for Elderfier service access without accessing
 * private balance information.
 * 
 * NOT for blockchain consensus - only for service access control.
 */
class ElderfierServiceProofVerifier {
private:
    ElderfierServiceModifierBuilder& m_modifierBuilder;
    std::unordered_set<std::string> m_usedProofHashes;
    mutable std::mutex m_usedProofsMutex;
    
public:
    /**
     * @brief Constructor
     * @param modifierBuilder Reference to the modifier builder
     */
    explicit ElderfierServiceProofVerifier(ElderfierServiceModifierBuilder& modifierBuilder);
    
    /**
     * @brief Destructor
     */
    ~ElderfierServiceProofVerifier() = default;
    
    /**
     * @brief Verify service access proof
     * 
     * Verifies a cryptographic proof that proves sufficient stake exists
     * for Elderfier service access without revealing exact balance.
     * 
     * @param proof Proof to verify
     * @param currentHeight Current blockchain height
     * @param expectedFeeAddress Expected fee address
     * @return true if proof is valid, false otherwise
     */
    bool verifyServiceAccessProof(const ElderfierServiceProof& proof,
                                 uint64_t currentHeight,
                                 const std::string& expectedFeeAddress);
    
    /**
     * @brief Check if proof has been used before
     * 
     * @param proofHash Hash of proof to check
     * @return true if proof already used, false otherwise
     */
    bool isProofAlreadyUsed(const std::string& proofHash) const;
    
    /**
     * @brief Clear used proof tracking
     */
    void clearUsedProofs();
    
    /**
     * @brief Get used proof statistics
     * 
     * @return Number of tracked used proofs
     */
    size_t getUsedProofCount() const;
    
private:
    /**
     * @brief Validate proof structure
     * 
     * @param proof Proof to validate
     * @return true if structure is valid, false otherwise
     */
    bool validateProofStructure(const ElderfierServiceProof& proof) const;
    
    /**
     * @brief Check for replay attacks
     * 
     * @param proofHash Hash of proof to check
     * @return true if proof already used, false otherwise
     */
    bool checkReplayAttack(const std::string& proofHash);
    
    /**
     * @brief Verify service modifier
     * 
     * @param proof Proof to verify
     * @param currentHeight Current blockchain height
     * @return true if modifier is valid, false otherwise
     */
    bool verifyServiceModifier(const ElderfierServiceProof& proof, uint64_t currentHeight);
    
    /**
     * @brief Verify proof timestamp
     * 
     * @param proof Proof to verify
     * @return true if timestamp is valid, false otherwise
     */
    bool verifyProofTimestamp(const ElderfierServiceProof& proof) const;
    
    /**
     * @brief Verify fee address hash
     * 
     * @param proof Proof to verify
     * @param expectedFeeAddress Expected fee address
     * @return true if address hash matches, false otherwise
     */
    bool verifyFeeAddressHash(const ElderfierServiceProof& proof, const std::string& expectedFeeAddress) const;
    
    /**
     * @brief Verify proof signature
     * 
     * @param proof Proof to verify
     * @return true if signature is valid, false otherwise
     */
    bool verifyProofSignature(const ElderfierServiceProof& proof) const;
    
    /**
     * @brief Mark proof as used
     * 
     * @param proofHash Hash of proof to mark as used
     */
    void markProofAsUsed(const std::string& proofHash);
    
    /**
     * @brief Clean up old used proofs
     */
    void cleanupOldUsedProofs();
    
    /**
     * @brief Hash fee address for comparison
     * 
     * @param feeAddress Fee address to hash
     * @return Hash of fee address
     */
    std::array<uint8_t, 32> hashFeeAddress(const std::string& feeAddress) const;
    
    /**
     * @brief Verify signature against payload
     * 
     * @param payload Payload that was signed
     * @param publicKey Public key for verification
     * @param signature Signature to verify
     * @return true if signature is valid, false otherwise
     */
    bool verifySignature(const std::string& payload,
                        const std::array<uint8_t, 32>& publicKey,
                        const std::array<uint8_t, 64>& signature) const;
};

} // namespace CryptoNote
