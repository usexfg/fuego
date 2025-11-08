#include "CryptoNoteCore/DynamicMoneySupply.h"
#include "CryptoNoteCore/Currency.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <iostream>

using namespace CryptoNote;

class DynamicMoneySupplyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize with default parameters
        m_dynamicSupply = DynamicMoneySupply();
        m_currency = CurrencyBuilder()
            .baseMoneySupply(80000088000008ULL) // 8,000,008.8000008 XFG
            .totalBurnedXfg(0)
            .totalRebornXfg(0)
            .totalSupply(80000088000008ULL)
            .circulatingSupply(80000088000008ULL)
            .build();
    }

    DynamicMoneySupply m_dynamicSupply;
    Currency m_currency;
    
    // Test constants
    static const uint64_t BASE_MONEY_SUPPLY = 80000088000008ULL; // 8,000,008.8000008 XFG
    static const uint64_t TEST_BURN_AMOUNT = 1000000000000ULL;   // 1,000,000.0000000 XFG
    static const uint64_t SMALL_BURN_AMOUNT = 100000000ULL;      // 100.0000000 XFG
};

// Test 1: Initial State Verification
TEST_F(DynamicMoneySupplyTest, InitialState) {
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), 0);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), 0);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_dynamicSupply.getCirculatingSupply(), BASE_MONEY_SUPPLY);
}

// Test 2: Single Burn Operation
TEST_F(DynamicMoneySupplyTest, SingleBurn) {
    uint64_t burnAmount = SMALL_BURN_AMOUNT;
    
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    // Verify burned amount is recorded
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), burnAmount);
    
    // Verify reborn amount equals burned amount
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), burnAmount);
    
    // Verify base supply increases by burn amount
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + burnAmount);
    
    // Verify total supply remains constant (base - burned = original)
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    
    // Verify block reward supply equals base supply
    EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), BASE_MONEY_SUPPLY + burnAmount);
    
    // Verify circulating supply equals total supply
    EXPECT_EQ(m_dynamicSupply.getCirculatingSupply(), BASE_MONEY_SUPPLY);
}

// Test 3: Multiple Burns
TEST_F(DynamicMoneySupplyTest, MultipleBurns) {
    uint64_t burn1 = SMALL_BURN_AMOUNT;
    uint64_t burn2 = SMALL_BURN_AMOUNT * 2;
    uint64_t burn3 = SMALL_BURN_AMOUNT * 3;
    uint64_t totalBurned = burn1 + burn2 + burn3;
    
    m_dynamicSupply.addBurnedXfg(burn1);
    m_dynamicSupply.addBurnedXfg(burn2);
    m_dynamicSupply.addBurnedXfg(burn3);
    
    // Verify total burned amount
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), totalBurned);
    
    // Verify reborn equals burned
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), totalBurned);
    
    // Verify base supply increases by total burned
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + totalBurned);
    
    // Verify total supply remains constant
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    
    // Verify block reward supply equals base supply
    EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), BASE_MONEY_SUPPLY + totalBurned);
}

// Test 4: Large Burn Amount
TEST_F(DynamicMoneySupplyTest, LargeBurnAmount) {
    uint64_t largeBurn = TEST_BURN_AMOUNT; // 1 million XFG
    
    m_dynamicSupply.addBurnedXfg(largeBurn);
    
    // Verify all calculations work with large amounts
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), largeBurn);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), largeBurn);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + largeBurn);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), BASE_MONEY_SUPPLY + largeBurn);
}

// Test 5: Zero Burn Amount
TEST_F(DynamicMoneySupplyTest, ZeroBurnAmount) {
    uint64_t initialBurned = m_dynamicSupply.getTotalBurnedXfg();
    uint64_t initialBase = m_dynamicSupply.getBaseMoneySupply();
    
    m_dynamicSupply.addBurnedXfg(0);
    
    // Verify nothing changes with zero burn
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), initialBurned);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), initialBase);
}

// Test 6: Percentage Calculations
TEST_F(DynamicMoneySupplyTest, PercentageCalculations) {
    uint64_t burnAmount = BASE_MONEY_SUPPLY / 10; // 10% of base supply
    
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    double burnPercentage = m_dynamicSupply.getBurnPercentage();
    double rebornPercentage = m_dynamicSupply.getRebornPercentage();
    
    // Verify percentages are approximately 10%
    EXPECT_NEAR(burnPercentage, 10.0, 0.1);
    EXPECT_NEAR(rebornPercentage, 10.0, 0.1);
    
    // Verify burn and reborn percentages are equal
    EXPECT_EQ(burnPercentage, rebornPercentage);
}

// Test 7: State Serialization
TEST_F(DynamicMoneySupplyTest, StateSerialization) {
    uint64_t burnAmount = SMALL_BURN_AMOUNT;
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    // Serialize state
    std::string serialized = m_dynamicSupply.serialize();
    EXPECT_FALSE(serialized.empty());
    
    // Create new instance and deserialize
    DynamicMoneySupply newSupply;
    newSupply.deserialize(serialized);
    
    // Verify deserialized state matches original
    EXPECT_EQ(newSupply.getBaseMoneySupply(), m_dynamicSupply.getBaseMoneySupply());
    EXPECT_EQ(newSupply.getTotalBurnedXfg(), m_dynamicSupply.getTotalBurnedXfg());
    EXPECT_EQ(newSupply.getTotalRebornXfg(), m_dynamicSupply.getTotalRebornXfg());
    EXPECT_EQ(newSupply.getTotalSupply(), m_dynamicSupply.getTotalSupply());
    EXPECT_EQ(newSupply.getBlockRewardSupply(), m_dynamicSupply.getBlockRewardSupply());
}

// Test 8: State Validation
TEST_F(DynamicMoneySupplyTest, StateValidation) {
    uint64_t burnAmount = SMALL_BURN_AMOUNT;
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    // Verify state is valid
    EXPECT_TRUE(m_dynamicSupply.validateState());
    
    // Verify economic balance
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), m_dynamicSupply.getTotalRebornXfg());
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + burnAmount);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
}

// Test 9: Clear State
TEST_F(DynamicMoneySupplyTest, ClearState) {
    uint64_t burnAmount = SMALL_BURN_AMOUNT;
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    // Verify state changed
    EXPECT_GT(m_dynamicSupply.getTotalBurnedXfg(), 0);
    
    // Clear state
    m_dynamicSupply.clearState();
    
    // Verify state is reset to initial values
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), 0);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), 0);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), BASE_MONEY_SUPPLY);
}

// Test 10: Currency Integration
TEST_F(DynamicMoneySupplyTest, CurrencyIntegration) {
    uint64_t burnAmount = SMALL_BURN_AMOUNT;
    
    // Test Currency class integration
    m_currency.addBurnedXfg(burnAmount);
    
    // Verify Currency methods work correctly
    EXPECT_EQ(m_currency.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_currency.getBlockRewardSupply(), BASE_MONEY_SUPPLY + burnAmount);
    EXPECT_EQ(m_currency.getCirculatingSupply(), BASE_MONEY_SUPPLY);
}

// Test 11: Edge Case - Maximum Burn Amount
TEST_F(DynamicMoneySupplyTest, MaximumBurnAmount) {
    uint64_t maxBurn = UINT64_MAX - BASE_MONEY_SUPPLY;
    
    // This should not cause overflow
    m_dynamicSupply.addBurnedXfg(maxBurn);
    
    // Verify calculations are correct
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), maxBurn);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), maxBurn);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + maxBurn);
}

// Test 12: Stress Test - Many Small Burns
TEST_F(DynamicMoneySupplyTest, StressTestManySmallBurns) {
    const int numBurns = 1000;
    uint64_t smallBurn = 1000000ULL; // 0.1 XFG
    uint64_t expectedTotal = smallBurn * numBurns;
    
    for (int i = 0; i < numBurns; ++i) {
        m_dynamicSupply.addBurnedXfg(smallBurn);
    }
    
    // Verify total is correct
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
}

// Test 13: Block Reward Scaling
TEST_F(DynamicMoneySupplyTest, BlockRewardScaling) {
    uint64_t burnAmount = BASE_MONEY_SUPPLY / 4; // 25% of base supply
    
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    // Verify block reward supply scales with base supply
    uint64_t expectedBlockRewardSupply = BASE_MONEY_SUPPLY + burnAmount;
    EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), expectedBlockRewardSupply);
    
    // Verify the ratio is correct
    double ratio = static_cast<double>(m_dynamicSupply.getBlockRewardSupply()) / 
                   static_cast<double>(BASE_MONEY_SUPPLY);
    EXPECT_NEAR(ratio, 1.25, 0.01); // Should be 1.25 (25% increase)
}

// Test 14: Economic Balance Verification
TEST_F(DynamicMoneySupplyTest, EconomicBalanceVerification) {
    std::vector<uint64_t> burnAmounts = {
        SMALL_BURN_AMOUNT,
        SMALL_BURN_AMOUNT * 2,
        SMALL_BURN_AMOUNT * 5,
        TEST_BURN_AMOUNT
    };
    
    uint64_t totalBurned = 0;
    for (uint64_t amount : burnAmounts) {
        m_dynamicSupply.addBurnedXfg(amount);
        totalBurned += amount;
        
        // Verify economic balance after each burn
        EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), totalBurned);
        EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), totalBurned);
        EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + totalBurned);
        EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
        EXPECT_EQ(m_dynamicSupply.getBlockRewardSupply(), BASE_MONEY_SUPPLY + totalBurned);
    }
}

// Test 15: Supply Cap Enforcement
TEST_F(DynamicMoneySupplyTest, SupplyCapEnforcement) {
    // Test that base supply never goes below initial value
    uint64_t burnAmount = SMALL_BURN_AMOUNT;
    
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    // Verify base supply is at least the initial value
    EXPECT_GE(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY);
    
    // Verify total supply is never negative
    EXPECT_GE(m_dynamicSupply.getTotalSupply(), 0);
    
    // Verify block reward supply is never negative
    EXPECT_GE(m_dynamicSupply.getBlockRewardSupply(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
