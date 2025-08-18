// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2014-2017 The XDN developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope
// that it will be useful, but WITHOUT ANY WARRANTY; without even
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <vector>
#include <cstdint>
#include "CryptoNote.h"

namespace CryptoNote {

enum class CommitmentType : uint8_t {
    HEAT = 0,   // For FOREVER deposits (burn deposits)
    YIELD = 1   // For interest-bearing deposits
};

struct DepositCommitment {
    CommitmentType type;
    Crypto::Hash commitment;
    std::vector<uint8_t> metadata;
    
    DepositCommitment() : type(CommitmentType::YIELD) {}
    DepositCommitment(CommitmentType t, const Crypto::Hash& c, const std::vector<uint8_t>& m = {})
        : type(t), commitment(c), metadata(m) {}
};

// Commitment generation functions
class DepositCommitmentGenerator {
public:
    // Generate HEAT commitment for FOREVER deposits (pure, no recipient)
    static DepositCommitment generateHeatCommitment(
        uint64_t xfgAmount,
        const std::vector<uint8_t>& metadata = {});
    
    // Generate HEAT commitment and return secret for local storage
    static std::pair<DepositCommitment, Crypto::SecretKey> generateHeatCommitmentWithSecret(
        uint64_t xfgAmount,
        const std::vector<uint8_t>& metadata = {});
    
    // Generate YIELD commitment for interest-bearing deposits
    static DepositCommitment generateYieldCommitment(
        uint64_t term,
        uint64_t amount,
        const std::vector<uint8_t>& metadata = {});
    
    // Generate appropriate commitment based on deposit term
    static DepositCommitment generateCommitment(
        uint64_t term,
        uint64_t amount,
        const std::vector<uint8_t>& metadata = {});
    
    // Validate commitment data
    static bool validateCommitment(const DepositCommitment& commitment);
    
    // Convert XFG to HEAT (0.8 XFG = 8M HEAT)
    static uint64_t convertXfgToHeat(uint64_t xfgAmount);
    
    // Convert HEAT to XFG
    static uint64_t convertHeatToXfg(uint64_t heatAmount);
};

} // namespace CryptoNote
