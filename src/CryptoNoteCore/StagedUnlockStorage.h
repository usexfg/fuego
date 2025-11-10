#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include "CryptoNote.h"

namespace CryptoNote {

// Storage class for managing staged unlock preferences per deposit
class StagedUnlockStorage {
public:
    StagedUnlockStorage();
    ~StagedUnlockStorage();

    // Set whether a deposit should use staged unlock
    void setStagedUnlockPreference(const std::string& transactionHash, bool useStagedUnlock);
    
    // Get staged unlock preference for a deposit
    bool getStagedUnlockPreference(const std::string& transactionHash) const;
    
    // Check if a deposit has a staged unlock preference stored
    bool hasStagedUnlockPreference(const std::string& transactionHash) const;
    
    // Remove staged unlock preference for a deposit
    void removeStagedUnlockPreference(const std::string& transactionHash);
    
    // Get all deposits that have staged unlock enabled
    std::vector<std::string> getStagedUnlockDeposits() const;
    
    // Clear all stored preferences
    void clearAllPreferences();
    
    // Get the total number of staged unlock deposits
    size_t getStagedUnlockCount() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, bool> m_stagedUnlockPreferences;
};

} // namespace CryptoNote