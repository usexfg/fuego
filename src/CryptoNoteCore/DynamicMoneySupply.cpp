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

#include "CryptoNoteCore/DynamicMoneySupply.h"
#include "CryptoNoteCore/DepositIndex.h"
#include "Serialization/ISerializer.h"
#include "Serialization/SerializationOverloads.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <cmath>

namespace CryptoNote {

DynamicMoneySupply::DynamicMoneySupply() {
    m_state.baseMoneySupply = BASE_MONEY_SUPPLY;
    m_state.totalBurnedXfg = 0;
    m_state.totalRebornXfg = 0;
    	m_state.totalSupply = BASE_MONEY_SUPPLY;
    m_state.circulatingSupply = BASE_MONEY_SUPPLY;
}

uint64_t DynamicMoneySupply::getBaseMoneySupply() const {
    return m_state.baseMoneySupply;
}

uint64_t DynamicMoneySupply::getTotalSupply() const {
	return m_state.totalSupply;
}

uint64_t DynamicMoneySupply::getCirculatingSupply() const {
    return m_state.circulatingSupply;
}

DynamicMoneySupply::BurnedAmount DynamicMoneySupply::getTotalBurnedXfg() const {
    return m_state.totalBurnedXfg;
}

DynamicMoneySupply::RebornAmount DynamicMoneySupply::getTotalRebornXfg() const {
    return m_state.totalRebornXfg;
}

void DynamicMoneySupply::addBurnedXfg(BurnedAmount amount) {
    if (amount == 0) return;
    
    m_state.totalBurnedXfg += amount;
    addRebornXfg(amount); // Automatically add as reborn XFG
    
    // Increase base money supply by the burned amount
    m_state.baseMoneySupply += amount;
    
    recalculateSupply();
    validateAmounts();
}

void DynamicMoneySupply::removeBurnedXfg(BurnedAmount amount) {
    if (amount == 0) return;
    
    if (amount > m_state.totalBurnedXfg) {
        m_state.totalBurnedXfg = 0;
    } else {
        m_state.totalBurnedXfg -= amount;
    }
    
    removeRebornXfg(amount); // Automatically remove from reborn XFG
    
    // Decrease base money supply by the removed amount
    if (amount <= m_state.baseMoneySupply - BASE_MONEY_SUPPLY) {
        m_state.baseMoneySupply -= amount;
    }
    
    recalculateSupply();
    validateAmounts();
}

void DynamicMoneySupply::addRebornXfg(RebornAmount amount) {
    if (amount == 0) return;
    
    m_state.totalRebornXfg += amount;
    recalculateSupply();
    validateAmounts();
}

void DynamicMoneySupply::removeRebornXfg(RebornAmount amount) {
    if (amount == 0) return;
    
    if (amount > m_state.totalRebornXfg) {
        m_state.totalRebornXfg = 0;
    } else {
        m_state.totalRebornXfg -= amount;
    }
    
    recalculateSupply();
    validateAmounts();
}

DynamicMoneySupply::MoneySupplyState DynamicMoneySupply::getCurrentState() const {
    return m_state;
}

void DynamicMoneySupply::updateFromDepositIndex(const DepositIndex& depositIndex) {
    BurnedAmount currentBurned = depositIndex.getBurnedXfgAmount();
    
    // Calculate the difference
    if (currentBurned > m_state.totalBurnedXfg) {
        BurnedAmount newBurned = currentBurned - m_state.totalBurnedXfg;
        addBurnedXfg(newBurned);
    } else if (currentBurned < m_state.totalBurnedXfg) {
        BurnedAmount removedBurned = m_state.totalBurnedXfg - currentBurned;
        removeBurnedXfg(removedBurned);
    }
}

double DynamicMoneySupply::getBurnPercentage() const {
    if (m_state.baseMoneySupply == 0) return 0.0;
    return (static_cast<double>(m_state.totalBurnedXfg) / m_state.baseMoneySupply) * 100.0;
}

double DynamicMoneySupply::getRebornPercentage() const {
    if (m_state.baseMoneySupply == 0) return 0.0;
    return (static_cast<double>(m_state.totalRebornXfg) / m_state.baseMoneySupply) * 100.0;
}

double DynamicMoneySupply::getSupplyIncreasePercentage() const {
    if (m_state.baseMoneySupply == 0) return 0.0;
    return (static_cast<double>(m_state.circulatingSupply - m_state.baseMoneySupply) / m_state.baseMoneySupply) * 100.0;
}

void DynamicMoneySupply::saveState(const std::string& filename) const {
    // TODO: Implement file-based persistence
    // For now, this is a placeholder
}

void DynamicMoneySupply::loadState(const std::string& filename) {
    // TODO: Implement file-based persistence
    // For now, this is a placeholder
}

void DynamicMoneySupply::clearState() {
    m_state.totalBurnedXfg = 0;
    m_state.totalRebornXfg = 0;
    m_state.baseMoneySupply = BASE_MONEY_SUPPLY;
    m_state.totalSupply = BASE_MONEY_SUPPLY;
    m_state.circulatingSupply = BASE_MONEY_SUPPLY;
    m_state.blockRewardSupply = BASE_MONEY_SUPPLY;
}

void DynamicMoneySupply::recalculateSupply() {
    // Total supply = Base money supply - Burned XFG
    m_state.totalSupply = m_state.baseMoneySupply - m_state.totalBurnedXfg;
    
    // Block reward supply = Base money supply (includes all reborn amounts)
    // This allows burned XFG to be redistributed through mining rewards
    m_state.blockRewardSupply = m_state.baseMoneySupply;
    
    // Circulating supply = Total supply - Locked deposits (excluding burn deposits)
    // Note: Locked deposits calculation is handled separately in DepositIndex
    m_state.circulatingSupply = m_state.totalSupply;
}

void DynamicMoneySupply::validateAmounts() const {
    // Ensure reborn XFG equals burned XFG
    if (m_state.totalRebornXfg != m_state.totalBurnedXfg) {
        throw std::runtime_error("Reborn XFG must equal burned XFG");
    }
    
    // Ensure base money supply never goes below original
    if (m_state.baseMoneySupply < BASE_MONEY_SUPPLY) {
        throw std::runtime_error("Base money supply cannot be less than original supply");
    }
    
    // Ensure total supply is reasonable
    if (m_state.totalSupply > m_state.baseMoneySupply) {
        throw std::runtime_error("Total supply cannot exceed base money supply");
    }
    
    // Ensure block reward supply equals base money supply
    if (m_state.blockRewardSupply != m_state.baseMoneySupply) {
        throw std::runtime_error("Block reward supply must equal base money supply");
    }
}

void DynamicMoneySupply::ensureSupplyCap() {
    // Ensure base money supply never goes below original
    if (m_state.baseMoneySupply < BASE_MONEY_SUPPLY) {
        m_state.baseMoneySupply = BASE_MONEY_SUPPLY;
    }
    
    // Recalculate all supplies
    recalculateSupply();
}

void DynamicMoneySupply::serialize(ISerializer& s) {
    s(m_state.baseMoneySupply, "baseMoneySupply");
    s(m_state.totalBurnedXfg, "totalBurnedXfg");
    s(m_state.totalRebornXfg, "totalRebornXfg");
    s(m_state.totalSupply, "totalSupply");
    s(m_state.circulatingSupply, "circulatingSupply");
    s(m_state.blockRewardSupply, "blockRewardSupply");
}

}
