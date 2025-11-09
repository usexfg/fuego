#include "StagedUnlockStorage.h"

namespace CryptoNote {

void StagedUnlockStorage::setStagedUnlockPreference(const std::string& transactionHash, bool useStagedUnlock) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stagedUnlockPreferences[transactionHash] = useStagedUnlock;
}

bool StagedUnlockStorage::getStagedUnlockPreference(const std::string& transactionHash) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_stagedUnlockPreferences.find(transactionHash);
    return (it != m_stagedUnlockPreferences.end()) ? it->second : false;
}

bool StagedUnlockStorage::hasStagedUnlockPreference(const std::string& transactionHash) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stagedUnlockPreferences.find(transactionHash) != m_stagedUnlockPreferences.end();
}

void StagedUnlockStorage::removeStagedUnlockPreference(const std::string& transactionHash) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stagedUnlockPreferences.erase(transactionHash);
}

std::vector<std::string> StagedUnlockStorage::getStagedUnlockDeposits() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> result;
    for (const auto& pair : m_stagedUnlockPreferences) {
        if (pair.second) {
            result.push_back(pair.first);
        }
    }
    return result;
}

} // namespace CryptoNote
