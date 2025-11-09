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

#include "StagedUnlock.h"
#include "CryptoNoteSerialization.h"
#include "Serialization/SerializationOverloads.h"
#include <algorithm>
#include <sstream>

namespace CryptoNote {

void StagedUnlock::initializeFromDeposit(const Deposit& deposit) {
    // Copy basic fields
    amount = deposit.amount;
    term = deposit.term;
    interest = deposit.interest;
    height = deposit.height;
    unlockHeight = deposit.unlockHeight;
    locked = deposit.locked;
    creatingTransactionId = deposit.creatingTransactionId;
    spendingTransactionId = deposit.spendingTransactionId;
    
    // Initialize staged unlock if applicable
    useStagedUnlock = StagedUnlockManager::shouldUseStagedUnlock(term);
    if (useStagedUnlock) {
        stagedUnlock.initializeStagedUnlock(amount, interest, height);
        totalUnlockedAmount = 0;
        remainingLockedAmount = amount + interest;
    } else {
        totalUnlockedAmount = 0;
        remainingLockedAmount = amount + interest;
    }
}

bool StagedUnlock::canUnlock(uint32_t currentHeight) const {
    if (useStagedUnlock) {
        // Check if any stage can be unlocked
        return !stagedUnlock.getNextUnlockStage(currentHeight).isUnlocked;
    } else {
        // Traditional unlock
        return currentHeight >= unlockHeight && locked;
    }
}

uint64_t StagedUnlock::getUnlockableAmount(uint32_t currentHeight) const {
    if (useStagedUnlock) {
        // Get amount from next unlockable stage
        auto nextStage = stagedUnlock.getNextUnlockStage(currentHeight);
        if (nextStage.isUnlocked || currentHeight < nextStage.unlockHeight) {
            return 0;
        }
        return nextStage.principalAmount + nextStage.interestAmount;
    } else {
        // Traditional unlock - all or nothing
        if (currentHeight >= unlockHeight && locked) {
            return amount + interest;
        }
        return 0;
    }
}

std::vector<UnlockStage> StagedUnlock::processUnlock(uint32_t currentHeight) {
    std::vector<UnlockStage> newlyUnlocked;
    
    if (useStagedUnlock) {
        // Process staged unlock
        newlyUnlocked = stagedUnlock.checkUnlockStages(currentHeight);
        
        // Update totals
        totalUnlockedAmount = stagedUnlock.getTotalUnlockedAmount();
        remainingLockedAmount = stagedUnlock.getRemainingLockedAmount();
        
        // Mark as fully unlocked if all stages are done
        if (stagedUnlock.isFullyUnlocked()) {
            locked = false;
        }
    } else {
        // Traditional unlock
        if (currentHeight >= unlockHeight && locked) {
            totalUnlockedAmount = amount + interest;
            remainingLockedAmount = 0;
            locked = false;
        }
    }
    
    return newlyUnlocked;
}

bool StagedUnlock::isFullyUnlocked() const {
    if (useStagedUnlock) {
        return stagedUnlock.isFullyUnlocked();
    } else {
        return !locked;
    }
}

UnlockStage StagedUnlock::getNextUnlockInfo(uint32_t currentHeight) const {
    if (useStagedUnlock) {
        return stagedUnlock.getNextUnlockStage(currentHeight);
    } else {
        // Return traditional unlock info
        UnlockStage stage;
        stage.stageNumber = 1;
        stage.unlockHeight = unlockHeight;
        stage.principalAmount = amount;
        stage.interestAmount = interest;
        stage.isUnlocked = currentHeight >= unlockHeight;
        return stage;
    }
}

void StagedUnlock::serialize(ISerializer& s) {
    // Serialize basic fields
    s(amount, "amount");
    s(term, "term");
    s(interest, "interest");
    s(height, "height");
    s(unlockHeight, "unlockHeight");
    s(locked, "locked");
    s(creatingTransactionId, "creatingTransactionId");
    s(spendingTransactionId, "spendingTransactionId");
    
    // Serialize staged unlock fields
    s(useStagedUnlock, "useStagedUnlock");
    s(stagedUnlock, "stagedUnlock");
    s(totalUnlockedAmount, "totalUnlockedAmount");
    s(remainingLockedAmount, "remainingLockedAmount");
}

// StagedUnlockManager implementation
std::vector<StagedUnlock> StagedUnlockManager::convertDeposits(const std::vector<Deposit>& deposits) {
    std::vector<StagedUnlock> stagedUnlocks;
    stagedUnlocks.reserve(deposits.size());
    
    for (const auto& deposit : deposits) {
        StagedUnlock stagedUnlock;
        stagedUnlock.initializeFromDeposit(deposit);
        stagedUnlocks.push_back(stagedUnlock);
    }
    
    return stagedUnlocks;
}

std::vector<DepositId> StagedUnlockManager::processAllUnlocks(uint32_t currentHeight, 
                                                            const std::vector<StagedUnlock>& deposits) {
    std::vector<DepositId> newlyUnlockedDeposits;
    
    for (size_t i = 0; i < deposits.size(); ++i) {
        const auto& deposit = deposits[i];
        
        if (deposit.canUnlock(currentHeight)) {
            // Process unlock
            auto newlyUnlocked = const_cast<StagedUnlock&>(deposit).processUnlock(currentHeight);
            
            if (!newlyUnlocked.empty()) {
                newlyUnlockedDeposits.push_back(static_cast<DepositId>(i));
            }
        }
    }
    
    return newlyUnlockedDeposits;
}

std::string StagedUnlockManager::getUnlockStatus(const StagedUnlock& deposit, uint32_t currentHeight) {
    std::ostringstream oss;
    
    if (deposit.useStagedUnlock) {
        oss << "Staged Unlock - ";
        auto nextStage = deposit.getNextUnlockInfo(currentHeight);
        
        if (deposit.isFullyUnlocked()) {
            oss << "Fully Unlocked";
        } else if (nextStage.isUnlocked) {
            oss << "Stage " << nextStage.stageNumber << " Ready";
        } else {
            oss << "Stage " << nextStage.stageNumber << " in " 
                << (nextStage.unlockHeight - currentHeight) << " blocks";
        }
        
        oss << " (Unlocked: " << deposit.totalUnlockedAmount 
            << ", Remaining: " << deposit.remainingLockedAmount << ")";
    } else {
        oss << "Traditional Unlock - ";
        if (deposit.isFullyUnlocked()) {
            oss << "Fully Unlocked";
        } else if (currentHeight >= deposit.unlockHeight) {
            oss << "Ready to Unlock";
        } else {
            oss << "Unlocks in " << (deposit.unlockHeight - currentHeight) << " blocks";
        }
    }
    
    return oss.str();
}

uint64_t StagedUnlockManager::getTotalUnlockedAmount(const std::vector<StagedUnlock>& deposits) {
    uint64_t total = 0;
    for (const auto& deposit : deposits) {
        total += deposit.totalUnlockedAmount;
    }
    return total;
}

uint64_t StagedUnlockManager::getTotalRemainingLockedAmount(const std::vector<StagedUnlock>& deposits) {
    uint64_t total = 0;
    for (const auto& deposit : deposits) {
        total += deposit.remainingLockedAmount;
    }
    return total;
}

} // namespace CryptoNote