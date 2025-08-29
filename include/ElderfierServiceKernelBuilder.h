// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <memory>
#include "ElderfierServiceModifierBuilder.h"

namespace CryptoNote {

/**
 * @brief Service access kernel builder for proof generation
 * 
 * This class builds service access kernels that contain the data
 * needed to generate service access proofs.
 * 
 * NOT for blockchain consensus - only for service access control.
 */
class ElderfierServiceKernelBuilder {
private:
    ElderfierServiceModifierBuilder& m_modifierBuilder;
    
public:
    /**
     * @brief Constructor
     * @param modifierBuilder Reference to the modifier builder
     */
    explicit ElderfierServiceKernelBuilder(ElderfierServiceModifierBuilder& modifierBuilder);
    
    /**
     * @brief Destructor
     */
    ~ElderfierServiceKernelBuilder() = default;
    
    /**
     * @brief Build service access kernel
     * 
     * Creates a kernel that includes:
     * - Current service modifier
     * - Fee address hash
     * - Minimum stake requirement
     * - Unique key image
     * 
     * @param feeAddress Fee address for service access
     * @param minimumStake Minimum stake required in atomic units
     * @param currentHeight Current blockchain height
     * @param kernel Output kernel
     * @return true if successful, false otherwise
     */
    bool buildKernel(const std::string& feeAddress,
                     uint64_t minimumStake,
                     uint64_t currentHeight,
                     ElderfierServiceKernel& kernel);
    
    /**
     * @brief Validate kernel parameters
     * 
     * @param feeAddress Fee address to validate
     * @param minimumStake Minimum stake to validate
     * @return true if valid, false otherwise
     */
    bool validateKernelParameters(const std::string& feeAddress, uint64_t minimumStake) const;
    
private:
    /**
     * @brief Generate unique key image for stake
     * 
     * @param feeAddress Fee address
     * @param modifier Current service modifier
     * @return Unique key image
     */
    std::array<uint8_t, 32> generateStakeKeyImage(const std::string& feeAddress,
                                                  const ElderfierServiceModifier& modifier) const;
    
    /**
     * @brief Hash fee address for privacy
     * 
     * @param feeAddress Fee address to hash
     * @return Hash of fee address
     */
    std::array<uint8_t, 32> hashFeeAddress(const std::string& feeAddress) const;
    
    /**
     * @brief Validate fee address format
     * 
     * @param feeAddress Fee address to validate
     * @return true if valid format, false otherwise
     */
    bool isValidFeeAddress(const std::string& feeAddress) const;
};

} // namespace CryptoNote
