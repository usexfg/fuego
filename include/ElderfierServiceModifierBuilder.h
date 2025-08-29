// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <memory>
#include "ElderfierServiceTypesSimple.h"

// Forward declarations
namespace CryptoNote {
    class core;
}

namespace CryptoNote {

/**
 * @brief Service access modifier builder to prevent proof grinding attacks
 * 
 * This class builds service access modifiers that prevent nodes from
 * manipulating proofs to gain unauthorized access to the Elderfier service.
 * 
 * NOT for blockchain consensus - only for service access control.
 */
class ElderfierServiceModifierBuilder {
private:
    CryptoNote::core& m_core;
    mutable std::mutex m_modifierMutex;
    ElderfierServiceModifier m_currentModifier;
    uint64_t m_lastUpdateHeight;
    
public:
    /**
     * @brief Constructor
     * @param core Reference to the CryptoNote core
     */
    explicit ElderfierServiceModifierBuilder(CryptoNote::core& core);
    
    /**
     * @brief Destructor
     */
    ~ElderfierServiceModifierBuilder() = default;
    
    /**
     * @brief Build service access modifier
     * 
     * Creates a new service modifier that includes:
     * - Last PoW block hash and height
     * - Timestamp and sequence number
     * - Prevents proof grinding attacks
     * 
     * @param sm Output service modifier
     * @param currentHeight Current blockchain height
     * @return true if successful, false otherwise
     */
    bool buildServiceModifier(ElderfierServiceModifier& sm, uint64_t currentHeight);
    
    /**
     * @brief Get current service modifier
     * 
     * @return Reference to current service modifier
     */
    const ElderfierServiceModifier& getCurrentModifier() const;
    
    /**
     * @brief Check if modifier needs update
     * 
     * @param currentHeight Current blockchain height
     * @return true if modifier should be updated
     */
    bool needsUpdate(uint64_t currentHeight) const;
    
    /**
     * @brief Force update of service modifier
     * 
     * @param currentHeight Current blockchain height
     * @return true if successful, false otherwise
     */
    bool forceUpdate(uint64_t currentHeight);
    
private:
    /**
     * @brief Initialize service modifier from genesis
     * 
     * @param sm Service modifier to initialize
     * @return true if successful, false otherwise
     */
    bool initializeFromGenesis(ElderfierServiceModifier& sm);
    
    /**
     * @brief Update service modifier from blockchain
     * 
     * @param sm Service modifier to update
     * @param currentHeight Current blockchain height
     * @return true if successful, false otherwise
     */
    bool updateFromBlockchain(ElderfierServiceModifier& sm, uint64_t currentHeight);
    
    /**
     * @brief Calculate hash of service modifier
     * 
     * @param modifier Service modifier to hash
     * @return Hash of the modifier
     */
    std::array<uint8_t, 32> calculateModifierHash(const ElderfierServiceModifier& modifier) const;
};

} // namespace CryptoNote
