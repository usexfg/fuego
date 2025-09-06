// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2014-2016 The XDN developers
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

#include "DepositCommitment.h"
#include "crypto/randomize.h"
#include "crypto/hash.h"
#include "crypto/keccak.h"
#include "Common/StringTools.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteConfig.h"

namespace CryptoNote {

DepositCommitment DepositCommitmentGenerator::generateHeatCommitment(
    uint64_t xfgAmount,
    const std::vector<uint8_t>& metadata) {
    
    // Generate secret for HEAT commitment
    Crypto::SecretKey secret;
    Randomize::randomBytes(sizeof(secret.data), secret.data);
    
    // 🔥 ADD: Include network ID in HEAT commitment metadata
    std::vector<uint8_t> enhancedMetadata = metadata;
    std::string networkId = "93385046440755750514194170694064996624";
    enhancedMetadata.insert(enhancedMetadata.end(), networkId.begin(), networkId.end());
    
    // Calculate HEAT commitment components (match xfgwinter)
    std::vector<uint8_t> heatData;
    
    // Add secret
    heatData.insert(heatData.end(), secret.data, secret.data + sizeof(secret.data));
    
    // Add "commitment" string (match xfgwinter algorithm)
    heatData.insert(heatData.end(), (uint8_t*)"commitment", (uint8_t*)"commitment" + 10);
    
    // Calculate final HEAT commitment (match xfgwinter)
    Crypto::Hash heatCommitment;
    keccak(heatData.data(), heatData.size(), heatCommitment.data, sizeof(heatCommitment.data));
    
    return DepositCommitment(CommitmentType::HEAT, heatCommitment, enhancedMetadata);
}

std::pair<DepositCommitment, Crypto::SecretKey> DepositCommitmentGenerator::generateHeatCommitmentWithSecret(
    uint64_t xfgAmount,
    const std::vector<uint8_t>& metadata) {
    
    // Generate secret for HEAT commitment
    Crypto::SecretKey secret;
    Randomize::randomBytes(sizeof(secret.data), secret.data);
    
    // 🔥 ADD: Include network ID in HEAT commitment metadata
    std::vector<uint8_t> enhancedMetadata = metadata;
    std::string networkId = "93385046440755750514194170694064996624";
    enhancedMetadata.insert(enhancedMetadata.end(), networkId.begin(), networkId.end());
    
    // Calculate HEAT commitment components (match xfgwinter)
    std::vector<uint8_t> heatData;
    
    // Add secret
    heatData.insert(heatData.end(), secret.data, secret.data + sizeof(secret.data));
    
    // Add "commitment" string (match xfgwinter algorithm)
    heatData.insert(heatData.end(), (uint8_t*)"commitment", (uint8_t*)"commitment" + 10);
    
    // Calculate final HEAT commitment (match xfgwinter)
    Crypto::Hash heatCommitment;
    keccak(heatData.data(), heatData.size(), heatCommitment.data, sizeof(heatCommitment.data));
    
    DepositCommitment commitment(CommitmentType::HEAT, heatCommitment, enhancedMetadata);
    return std::make_pair(commitment, secret);
}

DepositCommitment DepositCommitmentGenerator::generateYieldCommitment(
    uint64_t term,
    uint64_t amount,
    const std::vector<uint8_t>& metadata) {
    
    // Generate secret for YIELD commitment
    Crypto::SecretKey secret;
    Randomize::randomBytes(sizeof(secret.data), secret.data);
    
    // Calculate YIELD commitment components
    std::vector<uint8_t> yieldData;
    
    // Add secret
    yieldData.insert(yieldData.end(), secret.data, secret.data + sizeof(secret.data));
    
    // Add term
    yieldData.insert(yieldData.end(),
        reinterpret_cast<const uint8_t*>(&term),
        reinterpret_cast<const uint8_t*>(&term) + sizeof(term));
    
    // Add amount
    yieldData.insert(yieldData.end(),
        reinterpret_cast<const uint8_t*>(&amount),
        reinterpret_cast<const uint8_t*>(&amount) + sizeof(amount));
    
    // Add metadata
    yieldData.insert(yieldData.end(), metadata.begin(), metadata.end());
    
    // Calculate final YIELD commitment
    Crypto::Hash yieldCommitment;
    keccak(yieldData.data(), yieldData.size(), yieldCommitment.data, sizeof(yieldCommitment.data));
    
    return DepositCommitment(CommitmentType::YIELD, yieldCommitment, metadata);
}

DepositCommitment DepositCommitmentGenerator::generateCommitment(
    uint64_t term,
    uint64_t amount,
    const std::vector<uint8_t>& metadata) {
    
    // FOREVER term = HEAT commitment (burn deposit)
    if (term == parameters::DEPOSIT_TERM_FOREVER) {
        return generateHeatCommitment(amount, metadata);
    }
    
    // Any other term = YIELD commitment (interest deposit)
    return generateYieldCommitment(term, amount, metadata);
}

bool DepositCommitmentGenerator::validateCommitment(const DepositCommitment& commitment) {
    // Basic validation
    if (commitment.commitment == NULL_HASH) {
        return false;
    }
    
    // Type-specific validation
    switch (commitment.type) {
        case CommitmentType::HEAT:
            // HEAT commitments should have recipient address in metadata
            return !commitment.metadata.empty();
            
        case CommitmentType::YIELD:
            // YIELD commitments should have term info in metadata
            return commitment.metadata.size() >= sizeof(uint64_t);
            
        default:
            return false;
    }
}

uint64_t DepositCommitmentGenerator::convertXfgToHeat(uint64_t xfgAmount) {
    // 0.8 XFG = 8M HEAT
    return (xfgAmount * 10000000) / 800000000;
}

uint64_t DepositCommitmentGenerator::convertHeatToXfg(uint64_t heatAmount) {
    // 8M HEAT = 0.8 XFG
    return (heatAmount * 800000000) / 10000000;
}

} // namespace CryptoNote
