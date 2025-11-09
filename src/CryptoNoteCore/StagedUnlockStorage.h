#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

namespace CryptoNote {

class StagedUnlockStorage {
public:
    void setStagedUnlockPreference(const std::string& transactionHash, bool useStagedUnlock);
    bool getStagedUnlockPreference(const std::string& transactionHash) const;
    bool hasStagedUnlockPreference(const std::string& transactionHash) const;
    void removeStagedUnlockPreference(const std::string& transactionHash);
    std::vector<std::string> getStagedUnlockDeposits() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, bool> m_stagedUnlockPreferences;
};

} // namespace CryptoNote
