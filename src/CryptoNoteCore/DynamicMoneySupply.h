// Copyright (c) 2017-2022 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2014-2017 The XDN developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CryptoNote {
class ISerializer;

class DynamicMoneySupply {
public:
    using BurnedAmount = uint64_t;
    using RebornAmount = uint64_t;
    
    struct MoneySupplyState {
        uint64_t baseMoneySupply;      // Increases with each burn
        BurnedAmount totalBurnedXfg;   // Total XFG burned through FOREVER deposits
        RebornAmount totalRebornXfg;   // Always equals totalBurnedXfg
        uint64_t totalSupply;          // baseMoneySupply - totalBurnedXfg
        uint64_t circulatingSupply;    // totalSupply - lockedDeposits (excluding burn deposits)
        uint64_t blockRewardSupply;    // baseMoneySupply (for mining rewards)
    };
    
    DynamicMoneySupply();
    ~DynamicMoneySupply() = default;
    
    // Core money supply management
    uint64_t getBaseMoneySupply() const;
    uint64_t getTotalSupply() const;
    uint64_t getCirculatingSupply() const;
    uint64_t getBlockRewardSupply() const;
    BurnedAmount getTotalBurnedXfg() const;
    RebornAmount getTotalRebornXfg() const;
    
    // Burned XFG management
    void addBurnedXfg(BurnedAmount amount);
    void removeBurnedXfg(BurnedAmount amount);
    
    // Reborn XFG management
    void addRebornXfg(RebornAmount amount);
    void removeRebornXfg(RebornAmount amount);
    
    // State management
    MoneySupplyState getCurrentState() const;
    void updateFromDepositIndex(const class DepositIndex& depositIndex);
    
    // Statistics and percentages
    double getBurnPercentage() const;
    double getRebornPercentage() const;
    double getSupplyIncreasePercentage() const;
    
    // Persistence
    void saveState(const std::string& filename) const;
    void loadState(const std::string& filename);
    void clearState();
    
    // Serialization
    void serialize(ISerializer& s);

private:
    MoneySupplyState m_state;
    static const uint64_t BASE_MONEY_SUPPLY = 80000088000008ULL;
    
    // Helper functions
    void recalculateSupply();
    void validateAmounts() const;
    void ensureSupplyCap();
};
}
