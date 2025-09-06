#include <iostream>
#include <cassert>
#include <cstdint>

// Simple test for DynamicMoneySupply without complex dependencies
class SimpleDynamicSupplyTest {
private:
    // Test constants
    static const uint64_t BASE_MONEY_SUPPLY = 80000088000008ULL; // 8,000,008.8000008 XFG
    static const uint64_t TEST_BURN_AMOUNT = 1000000000000ULL;   // 1,000,000.0000000 XFG
    static const uint64_t SMALL_BURN_AMOUNT = 100000000ULL;      // 100.0000000 XFG
    
    // Simple DynamicMoneySupply implementation for testing
    class SimpleDynamicSupply {
    private:
        uint64_t m_baseMoneySupply;
        uint64_t m_totalBurnedXfg;
        uint64_t m_totalRebornXfg;
        uint64_t m_totalSupply;
        uint64_t m_circulatingSupply;
        uint64_t m_blockRewardSupply;
        
    public:
        SimpleDynamicSupply() {
            reset();
        }
        
        void reset() {
            m_baseMoneySupply = BASE_MONEY_SUPPLY;
            m_totalBurnedXfg = 0;
            m_totalRebornXfg = 0;
            m_totalSupply = BASE_MONEY_SUPPLY;
            m_circulatingSupply = BASE_MONEY_SUPPLY;
            m_blockRewardSupply = BASE_MONEY_SUPPLY;
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
        
        // Getters
        uint64_t getBaseMoneySupply() const { return m_baseMoneySupply; }
        uint64_t getTotalBurnedXfg() const { return m_totalBurnedXfg; }
        uint64_t getTotalRebornXfg() const { return m_totalRebornXfg; }
        uint64_t getTotalSupply() const { return m_totalSupply; }
        uint64_t getCirculatingSupply() const { return m_circulatingSupply; }
        uint64_t getBlockRewardSupply() const { return m_blockRewardSupply; }
        
        double getBurnPercentage() const {
            if (m_baseMoneySupply == 0) return 0.0;
            return (static_cast<double>(m_totalBurnedXfg) / m_baseMoneySupply) * 100.0;
        }
        
        double getRebornPercentage() const {
            if (m_baseMoneySupply == 0) return 0.0;
            return (static_cast<double>(m_totalRebornXfg) / m_baseMoneySupply) * 100.0;
        }
    };
    
    SimpleDynamicSupply m_dynamicSupply;
    
    void assertEqual(uint64_t expected, uint64_t actual, const std::string& testName) {
        if (expected != actual) {
            std::cout << "❌ FAIL: " << testName << " - Expected: " << expected << ", Got: " << actual << std::endl;
            exit(1);
        }
        std::cout << "✅ PASS: " << testName << std::endl;
    }
    
    void assertTrue(bool condition, const std::string& testName) {
        if (!condition) {
            std::cout << "❌ FAIL: " << testName << std::endl;
            exit(1);
        }
        std::cout << "✅ PASS: " << testName << std::endl;
    }
    
public:
    void runAllTests() {
        std::cout << "🔥 Running Dynamic Supply Tests 🔥" << std::endl;
        std::cout << "=================================" << std::endl;
        
        testInitialState();
        testSingleBurn();
        testMultipleBurns();
        testZeroBurnAmount();
        testPercentageCalculations();
        testEconomicBalance();
        testBlockRewardScaling();
        testLargeBurnAmount();
        testStressTest();
        
        std::cout << "\n🎉 All tests passed! 🎉" << std::endl;
    }
    
private:
    void testInitialState() {
        std::cout << "\n--- Test 1: Initial State ---" << std::endl;
        m_dynamicSupply.reset();
        
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getBaseMoneySupply(), "Initial base money supply");
        assertEqual(0, m_dynamicSupply.getTotalBurnedXfg(), "Initial burned XFG");
        assertEqual(0, m_dynamicSupply.getTotalRebornXfg(), "Initial reborn XFG");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getTotalSupply(), "Initial total supply");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getBlockRewardSupply(), "Initial block reward supply");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getCirculatingSupply(), "Initial circulating supply");
    }
    
    void testSingleBurn() {
        std::cout << "\n--- Test 2: Single Burn ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t burnAmount = SMALL_BURN_AMOUNT;
        m_dynamicSupply.addBurnedXfg(burnAmount);
        
        assertEqual(burnAmount, m_dynamicSupply.getTotalBurnedXfg(), "Burned amount recorded");
        assertEqual(burnAmount, m_dynamicSupply.getTotalRebornXfg(), "Reborn equals burned");
        assertEqual(BASE_MONEY_SUPPLY + burnAmount, m_dynamicSupply.getBaseMoneySupply(), "Base supply increased");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getTotalSupply(), "Total supply unchanged");
        assertEqual(BASE_MONEY_SUPPLY + burnAmount, m_dynamicSupply.getBlockRewardSupply(), "Block reward supply increased");
    }
    
    void testMultipleBurns() {
        std::cout << "\n--- Test 3: Multiple Burns ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t burn1 = SMALL_BURN_AMOUNT;
        uint64_t burn2 = SMALL_BURN_AMOUNT * 2;
        uint64_t totalBurn = burn1 + burn2;
        
        m_dynamicSupply.addBurnedXfg(burn1);
        m_dynamicSupply.addBurnedXfg(burn2);
        
        assertEqual(totalBurn, m_dynamicSupply.getTotalBurnedXfg(), "Total burned amount");
        assertEqual(totalBurn, m_dynamicSupply.getTotalRebornXfg(), "Total reborn equals total burned");
        assertEqual(BASE_MONEY_SUPPLY + totalBurn, m_dynamicSupply.getBaseMoneySupply(), "Base supply increased by total burn");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getTotalSupply(), "Total supply unchanged");
        assertEqual(BASE_MONEY_SUPPLY + totalBurn, m_dynamicSupply.getBlockRewardSupply(), "Block reward supply increased");
    }
    
    void testZeroBurnAmount() {
        std::cout << "\n--- Test 4: Zero Burn Amount ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t initialBase = m_dynamicSupply.getBaseMoneySupply();
        uint64_t initialBurned = m_dynamicSupply.getTotalBurnedXfg();
        
        m_dynamicSupply.addBurnedXfg(0);
        
        assertEqual(initialBase, m_dynamicSupply.getBaseMoneySupply(), "Base supply unchanged with zero burn");
        assertEqual(initialBurned, m_dynamicSupply.getTotalBurnedXfg(), "Burned amount unchanged with zero burn");
    }
    
    void testPercentageCalculations() {
        std::cout << "\n--- Test 5: Percentage Calculations ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t burnAmount = TEST_BURN_AMOUNT;
        m_dynamicSupply.addBurnedXfg(burnAmount);
        
        double expectedBurnPercentage = (static_cast<double>(burnAmount) / (BASE_MONEY_SUPPLY + burnAmount)) * 100.0;
        double actualBurnPercentage = m_dynamicSupply.getBurnPercentage();
        
        assertTrue(std::abs(expectedBurnPercentage - actualBurnPercentage) < 0.001, "Burn percentage calculation");
        assertTrue(std::abs(expectedBurnPercentage - m_dynamicSupply.getRebornPercentage()) < 0.001, "Reborn percentage equals burn percentage");
    }
    
    void testEconomicBalance() {
        std::cout << "\n--- Test 6: Economic Balance ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t burnAmount = TEST_BURN_AMOUNT;
        m_dynamicSupply.addBurnedXfg(burnAmount);
        
        // Verify economic balance: reborn == burned, base supply increase == burned
        assertEqual(m_dynamicSupply.getTotalBurnedXfg(), m_dynamicSupply.getTotalRebornXfg(), "Reborn equals burned");
        assertEqual(m_dynamicSupply.getBaseMoneySupply() - BASE_MONEY_SUPPLY, burnAmount, "Base supply increase equals burn amount");
        assertEqual(m_dynamicSupply.getBlockRewardSupply(), m_dynamicSupply.getBaseMoneySupply(), "Block reward supply equals base supply");
    }
    
    void testBlockRewardScaling() {
        std::cout << "\n--- Test 7: Block Reward Scaling ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t burnAmount = TEST_BURN_AMOUNT;
        m_dynamicSupply.addBurnedXfg(burnAmount);
        
        // Block reward supply should increase with burns to maintain reward stability
        uint64_t initialRewardSupply = BASE_MONEY_SUPPLY;
        uint64_t finalRewardSupply = m_dynamicSupply.getBlockRewardSupply();
        
        assertEqual(initialRewardSupply + burnAmount, finalRewardSupply, "Block reward supply scales with burns");
        assertTrue(finalRewardSupply > initialRewardSupply, "Block reward supply increases");
    }
    
    void testLargeBurnAmount() {
        std::cout << "\n--- Test 8: Large Burn Amount ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t largeBurn = TEST_BURN_AMOUNT * 5; // 5 million XFG
        m_dynamicSupply.addBurnedXfg(largeBurn);
        
        assertEqual(largeBurn, m_dynamicSupply.getTotalBurnedXfg(), "Large burn amount recorded");
        assertEqual(largeBurn, m_dynamicSupply.getTotalRebornXfg(), "Large reborn equals large burned");
        assertEqual(BASE_MONEY_SUPPLY + largeBurn, m_dynamicSupply.getBaseMoneySupply(), "Base supply increased by large burn");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getTotalSupply(), "Total supply unchanged with large burn");
    }
    
    void testStressTest() {
        std::cout << "\n--- Test 9: Stress Test (Many Small Burns) ---" << std::endl;
        m_dynamicSupply.reset();
        
        uint64_t totalBurn = 0;
        uint64_t smallBurn = 1000000ULL; // 0.1 XFG
        
        // Perform 1000 small burns
        for (int i = 0; i < 1000; i++) {
            m_dynamicSupply.addBurnedXfg(smallBurn);
            totalBurn += smallBurn;
        }
        
        assertEqual(totalBurn, m_dynamicSupply.getTotalBurnedXfg(), "Stress test total burned");
        assertEqual(totalBurn, m_dynamicSupply.getTotalRebornXfg(), "Stress test total reborn");
        assertEqual(BASE_MONEY_SUPPLY + totalBurn, m_dynamicSupply.getBaseMoneySupply(), "Stress test base supply");
        assertEqual(BASE_MONEY_SUPPLY, m_dynamicSupply.getTotalSupply(), "Stress test total supply");
        
        // Verify system stability
        assertTrue(m_dynamicSupply.getTotalBurnedXfg() == m_dynamicSupply.getTotalRebornXfg(), "System stability: reborn == burned");
        assertTrue(m_dynamicSupply.getBlockRewardSupply() == m_dynamicSupply.getBaseMoneySupply(), "System stability: block reward supply == base supply");
    }
};

int main() {
    SimpleDynamicSupplyTest test;
    test.runAllTests();
    return 0;
}
