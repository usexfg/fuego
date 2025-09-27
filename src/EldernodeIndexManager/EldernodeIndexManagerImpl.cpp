// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#include "EldernodeIndexManagerImpl.h"
#include "crypto/crypto.h"

namespace CryptoNote {

EldernodeIndexManagerImpl::EldernodeIndexManagerImpl() {
    // Initialize with empty Eldernode list
}

std::vector<EldernodeInfo> EldernodeIndexManagerImpl::getActiveEldernodes() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<EldernodeInfo> active;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            active.push_back(pair.second);
        }
    }

    return active;
}

std::optional<EldernodeInfo> EldernodeIndexManagerImpl::getEldernode(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_eldernodes.find(publicKey);
    if (it != m_eldernodes.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool EldernodeIndexManagerImpl::isEldernodeActive(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_eldernodes.find(publicKey);
    return it != m_eldernodes.end() && it->second.isActive;
}

size_t EldernodeIndexManagerImpl::getActiveEldernodeCount() {
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t count = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            ++count;
        }
    }

    return count;
}

void EldernodeIndexManagerImpl::updateEldernodeStatus(const Crypto::PublicKey& publicKey, bool isActive) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_eldernodes.find(publicKey);
    if (it != m_eldernodes.end()) {
        it->second.isActive = isActive;
    }
}

bool EldernodeIndexManagerImpl::addEldernode(const Crypto::PublicKey& publicKey,
                                           const std::string& address,
                                           uint64_t depositAmount,
                                           uint32_t registrationHeight) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if already exists
    if (m_eldernodes.find(publicKey) != m_eldernodes.end()) {
        return false;
    }

    EldernodeInfo info;
    info.publicKey = publicKey;
    info.address = address;
    info.depositAmount = depositAmount;
    info.registrationHeight = registrationHeight;
    info.isActive = true; // Assume active when added

    m_eldernodes[publicKey] = info;
    return true;
}

bool EldernodeIndexManagerImpl::removeEldernode(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        return false;
    }

    m_eldernodes.erase(it);
    return true;
}

void EldernodeIndexManagerImpl::updateEldernodeDeposit(const Crypto::PublicKey& publicKey, uint64_t newAmount) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_eldernodes.find(publicKey);
    if (it != m_eldernodes.end()) {
        it->second.depositAmount = newAmount;
        // TODO: Check if deposit amount meets minimum requirements and update active status
    }
}

} // namespace CryptoNote
