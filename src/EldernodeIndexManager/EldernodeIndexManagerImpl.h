// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#pragma once

#include "IEldernodeIndexManager.h"
#include <unordered_map>
#include <mutex>

namespace CryptoNote {

class EldernodeIndexManagerImpl : public IEldernodeIndexManager {
public:
    EldernodeIndexManagerImpl();
    ~EldernodeIndexManagerImpl() override = default;

    // IEldernodeIndexManager implementation
    std::vector<EldernodeInfo> getActiveEldernodes() override;
    std::optional<EldernodeInfo> getEldernode(const Crypto::PublicKey& publicKey) override;
    bool isEldernodeActive(const Crypto::PublicKey& publicKey) override;
    size_t getActiveEldernodeCount() override;
    void updateEldernodeStatus(const Crypto::PublicKey& publicKey, bool isActive) override;

    // Additional methods for managing Eldernodes
    bool addEldernode(const Crypto::PublicKey& publicKey, const std::string& address,
                     uint64_t depositAmount, uint32_t registrationHeight);
    bool removeEldernode(const Crypto::PublicKey& publicKey);
    void updateEldernodeDeposit(const Crypto::PublicKey& publicKey, uint64_t newAmount);

private:
    std::unordered_map<Crypto::PublicKey, EldernodeInfo> m_eldernodes;
    mutable std::mutex m_mutex;
};

} // namespace CryptoNote
