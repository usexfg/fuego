#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>

class DynamicSupplySimulation {
private:
    // Simulation parameters
    static const uint64_t INITIAL_BASE_SUPPLY = 80000088000008ULL;
    static const uint64_t TOTAL_BURN_AMOUNT = 1000000000000ULL; // 1 million XFG
    static const uint32_t SIMULATION_DAYS = 180; // 6 months
    static const uint32_t BLOCKS_PER_DAY = 1440; // Assuming 1-minute blocks
    
    // State variables
    uint64_t m_baseMoneySupply;
    uint64_t m_totalBurnedXfg;
    uint64_t m_totalRebornXfg;
    uint64_t m_totalSupply;
    uint64_t m_circulatingSupply;
    uint64_t m_blockRewardSupply;
    
    // Tracking data
    struct SimulationState {
        uint32_t day;
        uint32_t block;
        uint64_t baseSupply;
        uint64_t totalSupply;
        uint64_t burnedXfg;
        uint64_t rebornXfg;
        uint64_t blockRewardSupply;
        uint64_t circulatingSupply;
        uint64_t dailyBurnAmount;
        uint64_t cumulativeBurnAmount;
        double burnPercentage;
        double rebornPercentage;
    };
    
    std::vector<SimulationState> m_states;
    
public:
    DynamicSupplySimulation() {
        reset();
    }
    
    void reset() {
        m_baseMoneySupply = INITIAL_BASE_SUPPLY;
        m_totalBurnedXfg = 0;
        m_totalRebornXfg = 0;
        m_totalSupply = INITIAL_BASE_SUPPLY;
        m_circulatingSupply = INITIAL_BASE_SUPPLY;
        m_blockRewardSupply = INITIAL_BASE_SUPPLY;
        m_states.clear();
    }
    
    void addBurnedXfg(uint64_t amount) {
        if (amount == 0) return;
        
        m_totalBurnedXfg += amount;
        addRebornXfg(amount); // Automatically add as reborn XFG
        
        // Increase base money supply by the burned amount
        m_baseMoneySupply += amount;
        
        // Recalculate supplies
        recalculateSupply();
    }
    
    void addRebornXfg(uint64_t amount) {
        if (amount == 0) return;
        
        m_totalRebornXfg += amount;
        
        // Recalculate supplies
        recalculateSupply();
    }
    
    void recalculateSupply() {
        // Total supply = Base money supply - Burned XFG
        m_totalSupply = m_baseMoneySupply - m_totalBurnedXfg;
        
        // Block reward supply = Base money supply (includes all reborn amounts)
        m_blockRewardSupply = m_baseMoneySupply;
        
        // Circulating supply = Total supply (simplified, excluding locked deposits)
        m_circulatingSupply = m_totalSupply;
    }
    
    void runSimulation() {
        std::cout << "🔥 Fuego Dynamic Supply Simulation 🔥" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Initial Base Supply: " << formatAmount(INITIAL_BASE_SUPPLY) << " XFG" << std::endl;
        std::cout << "Total Burn Amount: " << formatAmount(TOTAL_BURN_AMOUNT) << " XFG" << std::endl;
        std::cout << "Simulation Period: " << SIMULATION_DAYS << " days (" << (SIMULATION_DAYS * BLOCKS_PER_DAY) << " blocks)" << std::endl;
        std::cout << std::endl;
        
        // Calculate daily burn amount (distributed evenly)
        uint64_t dailyBurnAmount = TOTAL_BURN_AMOUNT / SIMULATION_DAYS;
        uint64_t remainingBurn = TOTAL_BURN_AMOUNT;
        
        uint64_t cumulativeBurnAmount = 0;
        
        for (uint32_t day = 1; day <= SIMULATION_DAYS; day++) {
            // Calculate today's burn amount
            uint64_t todayBurn = (day == SIMULATION_DAYS) ? remainingBurn : dailyBurnAmount;
            remainingBurn -= todayBurn;
            cumulativeBurnAmount += todayBurn;
            
            // Simulate daily burns
            for (uint32_t block = 1; block <= BLOCKS_PER_DAY; block++) {
                uint32_t currentBlock = (day - 1) * BLOCKS_PER_DAY + block;
                
                // Add burn amount (distributed across blocks)
                uint64_t blockBurnAmount = todayBurn / BLOCKS_PER_DAY;
                if (block == BLOCKS_PER_DAY) {
                    blockBurnAmount += todayBurn % BLOCKS_PER_DAY; // Handle remainder
                }
                
                if (blockBurnAmount > 0) {
                    addBurnedXfg(blockBurnAmount);
                }
                
                // Record state every 30 days or on significant events
                if (day % 30 == 0 || block == BLOCKS_PER_DAY || day == 1) {
                    recordState(day, currentBlock, todayBurn, cumulativeBurnAmount);
                }
            }
            
            // Verify system stability
            if (!verifySystemStability()) {
                std::cout << "❌ SYSTEM INSTABILITY DETECTED at day " << day << std::endl;
                return;
            }
        }
        
        // Final analysis
        analyzeResults();
    }
    
private:
    void recordState(uint32_t day, uint32_t block, uint64_t dailyBurn, uint64_t cumulativeBurn) {
        SimulationState state;
        state.day = day;
        state.block = block;
        state.baseSupply = m_baseMoneySupply;
        state.totalSupply = m_totalSupply;
        state.burnedXfg = m_totalBurnedXfg;
        state.rebornXfg = m_totalRebornXfg;
        state.blockRewardSupply = m_blockRewardSupply;
        state.circulatingSupply = m_circulatingSupply;
        state.dailyBurnAmount = dailyBurn;
        state.cumulativeBurnAmount = cumulativeBurn;
        state.burnPercentage = getBurnPercentage();
        state.rebornPercentage = getRebornPercentage();
        
        m_states.push_back(state);
    }
    
    bool verifySystemStability() {
        // Check that reborn equals burned
        if (m_totalRebornXfg != m_totalBurnedXfg) {
            std::cout << "❌ Reborn != Burned: " << m_totalRebornXfg 
                      << " != " << m_totalBurnedXfg << std::endl;
            return false;
        }
        
        // Check that base supply never goes below initial
        if (m_baseMoneySupply < INITIAL_BASE_SUPPLY) {
            std::cout << "❌ Base supply below initial: " << m_baseMoneySupply 
                      << " < " << INITIAL_BASE_SUPPLY << std::endl;
            return false;
        }
        
        // Check that total supply is reasonable
        if (m_totalSupply > m_baseMoneySupply) {
            std::cout << "❌ Total supply exceeds base supply" << std::endl;
            return false;
        }
        
        // Check that block reward supply equals base supply
        if (m_blockRewardSupply != m_baseMoneySupply) {
            std::cout << "❌ Block reward supply != base supply" << std::endl;
            return false;
        }
        
        return true;
    }
    
    double getBurnPercentage() const {
        if (m_baseMoneySupply == 0) return 0.0;
        return (static_cast<double>(m_totalBurnedXfg) / m_baseMoneySupply) * 100.0;
    }
    
    double getRebornPercentage() const {
        if (m_baseMoneySupply == 0) return 0.0;
        return (static_cast<double>(m_totalRebornXfg) / m_baseMoneySupply) * 100.0;
    }
    
    void analyzeResults() {
        std::cout << "\n📊 SIMULATION RESULTS 📊" << std::endl;
        std::cout << "=======================" << std::endl;
        
        if (m_states.empty()) {
            std::cout << "No data recorded" << std::endl;
            return;
        }
        
        // Initial state
        const auto& initial = m_states.front();
        std::cout << "Initial State:" << std::endl;
        printState(initial);
        
        // Final state
        const auto& final = m_states.back();
        std::cout << "\nFinal State:" << std::endl;
        printState(final);
        
        // Calculate block reward changes
        uint64_t initialRewardSupply = initial.blockRewardSupply;
        uint64_t finalRewardSupply = final.blockRewardSupply;
        uint64_t rewardIncrease = finalRewardSupply - initialRewardSupply;
        double rewardIncreasePercent = (static_cast<double>(rewardIncrease) / initialRewardSupply) * 100.0;
        
        std::cout << "\n💰 BLOCK REWARD ANALYSIS 💰" << std::endl;
        std::cout << "Initial Block Reward Supply: " << formatAmount(initialRewardSupply) << " XFG" << std::endl;
        std::cout << "Final Block Reward Supply: " << formatAmount(finalRewardSupply) << " XFG" << std::endl;
        std::cout << "Increase: " << formatAmount(rewardIncrease) << " XFG (" << std::fixed << std::setprecision(2) << rewardIncreasePercent << "%)" << std::endl;
        
        // Verify economic balance
        std::cout << "\n⚖️ ECONOMIC BALANCE VERIFICATION ⚖️" << std::endl;
        uint64_t totalBurned = final.burnedXfg;
        uint64_t totalReborn = final.rebornXfg;
        uint64_t baseSupplyIncrease = final.baseSupply - initial.baseSupply;
        
        std::cout << "Total Burned: " << formatAmount(totalBurned) << " XFG" << std::endl;
        std::cout << "Total Reborn: " << formatAmount(totalReborn) << " XFG" << std::endl;
        std::cout << "Base Supply Increase: " << formatAmount(baseSupplyIncrease) << " XFG" << std::endl;
        
        if (totalBurned == totalReborn && totalBurned == baseSupplyIncrease) {
            std::cout << "✅ ECONOMIC BALANCE MAINTAINED" << std::endl;
        } else {
            std::cout << "❌ ECONOMIC IMBALANCE DETECTED" << std::endl;
        }
        
        // Monthly progression
        std::cout << "\n📈 MONTHLY PROGRESSION 📈" << std::endl;
        std::cout << std::setw(8) << "Month" << std::setw(20) << "Base Supply" << std::setw(20) << "Total Supply" 
                  << std::setw(15) << "Burned" << std::setw(15) << "Reward Supply" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (const auto& state : m_states) {
            if (state.day % 30 == 0 || state.day == 1) {
                std::cout << std::setw(8) << (state.day / 30) << std::setw(20) << formatAmount(state.baseSupply)
                          << std::setw(20) << formatAmount(state.totalSupply) << std::setw(15) << formatAmount(state.burnedXfg)
                          << std::setw(15) << formatAmount(state.blockRewardSupply) << std::endl;
            }
        }
        
        // Block reward stability analysis
        std::cout << "\n🔍 BLOCK REWARD STABILITY ANALYSIS 🔍" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        uint64_t totalBlocks = SIMULATION_DAYS * BLOCKS_PER_DAY;
        uint64_t totalRewardPool = finalRewardSupply;
        uint64_t averageRewardPerBlock = totalRewardPool / totalBlocks;
        
        std::cout << "Total Blocks: " << totalBlocks << std::endl;
        std::cout << "Total Reward Pool: " << formatAmount(totalRewardPool) << " XFG" << std::endl;
        std::cout << "Average Reward per Block: " << formatAmount(averageRewardPerBlock) << " XFG" << std::endl;
        
        // Calculate what would happen if we used total supply instead
        uint64_t totalSupplyRewardPool = final.totalSupply;
        uint64_t totalSupplyAvgReward = totalSupplyRewardPool / totalBlocks;
        
        std::cout << "\nComparison with Total Supply:" << std::endl;
        std::cout << "Total Supply Reward Pool: " << formatAmount(totalSupplyRewardPool) << " XFG" << std::endl;
        std::cout << "Total Supply Avg Reward: " << formatAmount(totalSupplyAvgReward) << " XFG" << std::endl;
        std::cout << "Difference: " << formatAmount(averageRewardPerBlock - totalSupplyAvgReward) << " XFG per block" << std::endl;
    }
    
    void printState(const SimulationState& state) {
        std::cout << "  Day: " << state.day << " (Block: " << state.block << ")" << std::endl;
        std::cout << "  Base Supply: " << formatAmount(state.baseSupply) << " XFG" << std::endl;
        std::cout << "  Total Supply: " << formatAmount(state.totalSupply) << " XFG" << std::endl;
        std::cout << "  Burned XFG: " << formatAmount(state.burnedXfg) << " XFG" << std::endl;
        std::cout << "  Reborn XFG: " << formatAmount(state.rebornXfg) << " XFG" << std::endl;
        std::cout << "  Block Reward Supply: " << formatAmount(state.blockRewardSupply) << " XFG" << std::endl;
        std::cout << "  Circulating Supply: " << formatAmount(state.circulatingSupply) << " XFG" << std::endl;
        std::cout << "  Burn Percentage: " << std::fixed << std::setprecision(4) << state.burnPercentage << "%" << std::endl;
        std::cout << "  Reborn Percentage: " << std::fixed << std::setprecision(4) << state.rebornPercentage << "%" << std::endl;
    }
    
    std::string formatAmount(uint64_t amount) {
        std::stringstream ss;
        if (amount >= 1000000000000ULL) {
            ss << std::fixed << std::setprecision(2) << (amount / 1000000000000.0) << "T";
        } else if (amount >= 1000000000ULL) {
            ss << std::fixed << std::setprecision(2) << (amount / 1000000000.0) << "B";
        } else if (amount >= 1000000ULL) {
            ss << std::fixed << std::setprecision(2) << (amount / 1000000.0) << "M";
        } else if (amount >= 1000ULL) {
            ss << std::fixed << std::setprecision(2) << (amount / 1000.0) << "K";
        } else {
            ss << amount;
        }
        return ss.str();
    }
};

int main() {
    DynamicSupplySimulation simulation;
    simulation.runSimulation();
    return 0;
}
