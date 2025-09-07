#pragma once

#include "EldernodeIndexTypes.h"
#include "CryptoNoteCore/TransactionExtra.h"
#include "crypto/hash.h"
#include "crypto/crypto.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace CryptoNote {

// Slashing request structure
struct SlashingRequest {
    Crypto::Hash depositHash;
    Crypto::PublicKey elderfierPublicKey;
    std::string reason;
    uint64_t timestamp;
    std::vector<uint8_t> evidence;
    
    bool isValid() const;
};

// Slashing result structure
struct SlashingResult {
    bool success;
    std::string message;
    uint64_t slashedAmount;
    
    static SlashingResult success(const std::string& message, uint64_t amount = 0);
    static SlashingResult failure(const std::string& message);
};

// Elderfier Deposit Manager
class ElderfierDepositManager {
public:
    ElderfierDepositManager();
    ~ElderfierDepositManager();
    
    // Deposit management
    bool addElderfierDeposit(const TransactionExtraElderfierDeposit& deposit);
    bool removeElderfierDeposit(const Crypto::Hash& depositHash);
    bool updateElderfierDeposit(const Crypto::Hash& depositHash, const TransactionExtraElderfierDeposit& updatedDeposit);
    
    // Deposit queries
    bool hasElderfierDeposit(const Crypto::Hash& depositHash) const;
    TransactionExtraElderfierDeposit getElderfierDeposit(const Crypto::Hash& depositHash) const;
    std::vector<TransactionExtraElderfierDeposit> getAllElderfierDeposits() const;
    
    // Deposit validation
    bool isValidElderfierDeposit(const TransactionExtraElderfierDeposit& deposit) const;
    bool isElderfierSlashable(const Crypto::Hash& depositHash) const;
    
    // Spending detection
    bool checkIfDepositOutputsSpent(const Crypto::Hash& depositHash) const;
    
    // Slashing operations
    SlashingResult processSlashingRequest(const SlashingRequest& request) const;
    
    // Statistics
    size_t getDepositCount() const;
    uint64_t getTotalDepositAmount() const;
    
private:
    std::unordered_map<Crypto::Hash, TransactionExtraElderfierDeposit> m_deposits;
    mutable std::mutex m_mutex;
    
    // Helper methods
    bool validateDepositAmount(uint64_t amount) const;
    bool validateDepositSignature(const TransactionExtraElderfierDeposit& deposit) const;
};

} // namespace CryptoNote
