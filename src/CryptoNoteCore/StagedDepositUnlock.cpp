// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2014-2017 The XDN developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope
// that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "StagedDepositUnlock.h"
#include "CryptoNoteSerialization.h"
#include "Serialization/SerializationOverloads.h"
#include <algorithm>
#include <chrono>

namespace CryptoNote {

StagedDepositUnlock::StagedDepositUnlock() 
    : m_totalAmount(0), m_totalInterest(0), m_depositHeight(0), m_initialized(false) {
}

StagedDepositUnlock::StagedDepositUnlock(uint64_t totalAmount, uint64_t totalInterest, uint32_t depositHeight)
    : m_totalAmount(totalAmount), m_totalInterest(totalInterest), m_depositHeight(depositHeight), m_initialized(true) {
    calculateStages();
}

void StagedDepositUnlock::initializeStagedUnlock(uint64_t totalAmount, uint64_t totalInterest, uint32_t depositHeight) {
    m_totalAmount = totalAmount;
    m_totalInterest = totalInterest;
    m_depositHeight = depositHeight;
    m_initialized = true;
    calculateStages();
}

void StagedDepositUnlock::calculateStages() {
    m_stages.clear();
    
    if (!m_initialized || m_totalAmount == 0) {
        return;
    }
    
    // Calculate principal amounts for each stage (20% each)
    uint64_t stage1Principal = (m_totalAmount * StagedUnlockConfig::STAGE_1_UNLOCK_PERCENT) / 100;
    uint64_t stage2Principal = (m_totalAmount * StagedUnlockConfig::STAGE_2_UNLOCK_PERCENT) / 100;
    uint64_t stage3Principal = (m_totalAmount * StagedUnlockConfig::STAGE_3_UNLOCK_PERCENT) / 100;
    uint64_t stage4Principal = (m_totalAmount * StagedUnlockConfig::STAGE_4_UNLOCK_PERCENT) / 100;
    uint64_t stage5Principal = m_totalAmount - stage1Principal - stage2Principal - stage3Principal - stage4Principal; // Remaining principal
    
    // Interest is paid off-chain at deposit time, on-chain interest is always 0
    uint64_t stage1Interest = 0;
    
    // Calculate unlock heights (every 18 days)
    uint32_t stage1Height = m_depositHeight + StagedUnlockConfig::STAGE_INTERVAL_BLOCKS;
    uint32_t stage2Height = m_depositHeight + (2 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    uint32_t stage3Height = m_depositHeight + (3 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    uint32_t stage4Height = m_depositHeight + (4 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    uint32_t stage5Height = m_depositHeight + (5 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    
    // Create stages (interest always 0 as it's paid off-chain)
    m_stages.emplace_back(1, stage1Height, stage1Principal, stage1Interest);
    m_stages.emplace_back(2, stage2Height, stage2Principal, 0);
    m_stages.emplace_back(3, stage3Height, stage3Principal, 0);
    m_stages.emplace_back(4, stage4Height, stage4Principal, 0);
    m_stages.emplace_back(5, stage5Height, stage5Principal, 0);
}

std::vector<UnlockStage> StagedDepositUnlock::checkUnlockStages(uint32_t currentHeight) {
    std::vector<UnlockStage> newlyUnlocked;
    
    for (auto& stage : m_stages) {
        if (!stage.isUnlocked && currentHeight >= stage.unlockHeight) {
            stage.isUnlocked = true;
            stage.unlockTimestamp = static_cast<uint64_t>(std::time(nullptr));
            newlyUnlocked.push_back(stage);
        }
    }
    
    return newlyUnlocked;
}

uint64_t StagedDepositUnlock::getTotalUnlockedAmount() const {
    uint64_t total = 0;
    for (const auto& stage : m_stages) {
        if (stage.isUnlocked) {
            total += stage.principalAmount + stage.interestAmount;
        }
    }
    return total;
}

uint64_t StagedDepositUnlock::getRemainingLockedAmount() const {
    return m_totalAmount + m_totalInterest - getTotalUnlockedAmount();
}

UnlockStage StagedDepositUnlock::getNextUnlockStage(uint32_t currentHeight) const {
    for (const auto& stage : m_stages) {
        if (!stage.isUnlocked) {
            return stage;
        }
    }
    return UnlockStage(); // No more stages
}

bool StagedDepositUnlock::isFullyUnlocked() const {
    return std::all_of(m_stages.begin(), m_stages.end(), 
                      [](const UnlockStage& stage) { return stage.isUnlocked; });
}

void StagedDepositUnlock::serialize(ISerializer& s) {
    s(m_totalAmount, "totalAmount");
    s(m_totalInterest, "totalInterest");
    s(m_depositHeight, "depositHeight");
    s(m_initialized, "initialized");
    
    if (s.type() == ISerializer::INPUT) {
        readSequence<UnlockStage>(std::back_inserter(m_stages), "stages", s);
    } else {
        writeSequence<UnlockStage>(m_stages.begin(), m_stages.end(), "stages", s);
    }
}

void UnlockStage::serialize(ISerializer& s) {
    s(stageNumber, "stageNumber");
    s(unlockHeight, "unlockHeight");
    s(principalAmount, "principalAmount");
    s(interestAmount, "interestAmount");
    s(isUnlocked, "isUnlocked");
    s(unlockTimestamp, "unlockTimestamp");
}

// StagedUnlockManager implementation
std::vector<DepositId> StagedUnlockManager::processStagedUnlocks(uint32_t currentHeight, 
                                                                const std::vector<Deposit>& deposits) {
    std::vector<DepositId> newlyUnlockedDeposits;
    
    for (size_t i = 0; i < deposits.size(); ++i) {
        const auto& deposit = deposits[i];
        
        // Check if this deposit should use staged unlocking
        if (shouldUseStagedUnlock(deposit.term)) {
            // Create staged unlock manager for this deposit
            StagedDepositUnlock stagedUnlock(deposit.amount, deposit.interest, deposit.height);
            
            // Check for newly unlocked stages
            auto newlyUnlocked = stagedUnlock.checkUnlockStages(currentHeight);
            
            if (!newlyUnlocked.empty()) {
                newlyUnlockedDeposits.push_back(static_cast<DepositId>(i));
            }
        }
    }
    
    return newlyUnlockedDeposits;
}

StagedDepositUnlock StagedUnlockManager::createStagedUnlock(uint64_t amount, uint64_t interest, uint32_t height) {
    return StagedDepositUnlock(amount, interest, height);
}

bool StagedUnlockManager::shouldUseStagedUnlock(uint32_t term) {
    // Use staged unlocking for yield deposits (non-FOREVER terms)
    // FOREVER deposits (burn deposits) don't use staged unlocking
    return term != parameters::DEPOSIT_TERM_FOREVER;
}

std::vector<UnlockStage> StagedUnlockManager::getUnlockSchedule(uint64_t amount, uint64_t interest, uint32_t height) {
    StagedDepositUnlock stagedUnlock(amount, interest, height);
    return stagedUnlock.getStages();
}

} // namespace CryptoNote