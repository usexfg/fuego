// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#pragma once

#include <vector>
#include <string>
#include <optional>
#include "crypto/crypto.h"

namespace CryptoNote {

struct EldernodeInfo {
    Crypto::PublicKey publicKey;
    std::string address;
    uint64_t depositAmount;
    uint32_t registrationHeight;
    bool isActive;
};

class IEldernodeIndexManager {
public:
    virtual ~IEldernodeIndexManager() = default;

    // Get all active Eldernodes
    virtual std::vector<EldernodeInfo> getActiveEldernodes() = 0;

    // Get Eldernode by public key
    virtual std::optional<EldernodeInfo> getEldernode(const Crypto::PublicKey& publicKey) = 0;

    // Check if Eldernode is active
    virtual bool isEldernodeActive(const Crypto::PublicKey& publicKey) = 0;

    // Get total count of active Eldernodes
    virtual size_t getActiveEldernodeCount() = 0;

    // Update Eldernode status (called when deposits change)
    virtual void updateEldernodeStatus(const Crypto::PublicKey& publicKey, bool isActive) = 0;
};

} // namespace CryptoNote
