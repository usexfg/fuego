#include "CryptoNoteCore/DynamicMoneySupply.h"
#include "CryptoNoteCore/Currency.h"
#include "CryptoNoteCore/DepositIndex.h"
#include "CryptoNoteCore/TransactionExtra.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <iostream>

using namespace CryptoNote;

class DynamicSupplyIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize components
        m_dynamicSupply = DynamicMoneySupply();
        m_depositIndex = DepositIndex();
        
        // Initialize currency with dynamic supply
        m_currency = CurrencyBuilder()
            .baseMoneySupply(80000088000008ULL) // 8,000,008.8000008 XFG
            .totalBurnedXfg(0)
            .totalRebornXfg(0)
            .totalSupply(80000088000008ULL)
            .circulatingSupply(80000088000008ULL)
            .build();
    }

    DynamicMoneySupply m_dynamicSupply;
    DepositIndex m_depositIndex;
    Currency m_currency;
    
    // Test constants
    static const uint64_t BASE_MONEY_SUPPLY = 80000088000008ULL;
    static const uint64_t TEST_DEPOSIT_AMOUNT = 1000000000000ULL; // 1,000,000.0000000 XFG
    static const uint32_t FOREVER_TERM = 4294967295; // DEPOSIT_TERM_FOREVER
};

// Test 1: FOREVER Deposit Creation and Burn Tracking
TEST_F(DynamicSupplyIntegrationTest, ForeverDepositCreation) {
    uint64_t depositAmount = TEST_DEPOSIT_AMOUNT;
    
    // Create a FOREVER deposit (burn deposit)
    Deposit deposit;
    deposit.amount = depositAmount;
    deposit.term = FOREVER_TERM;
    deposit.creatingTransactionHash = Crypto::Hash();
    deposit.spendingTransactionHash = Crypto::Hash();
    deposit.height = 1000;
    deposit.unlockHeight = 0; // Never unlocks
    
    // Add deposit to index
    m_depositIndex.addDeposit(deposit);
    
    // Verify deposit is tracked as burned
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), depositAmount);
    
    // Add burned amount to dynamic supply
    m_dynamicSupply.addBurnedXfg(depositAmount);
    
    // Verify dynamic supply reflects the burn
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), depositAmount);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), depositAmount);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + depositAmount);
}

// Test 2: Multiple FOREVER Deposits
TEST_F(DynamicSupplyIntegrationTest, MultipleForeverDeposits) {
    std::vector<uint64_t> depositAmounts = {
        100000000000ULL,  // 100,000.0000000 XFG
        200000000000ULL,  // 200,000.0000000 XFG
        500000000000ULL,  // 500,000.0000000 XFG
        200000000000ULL   // 200,000.0000000 XFG
    };
    
    uint64_t totalBurned = 0;
    
    for (size_t i = 0; i < depositAmounts.size(); ++i) {
        Deposit deposit;
        deposit.amount = depositAmounts[i];
        deposit.term = FOREVER_TERM;
        deposit.creatingTransactionHash = Crypto::Hash();
        deposit.spendingTransactionHash = Crypto::Hash();
        deposit.height = 1000 + i;
        deposit.unlockHeight = 0;
        
        m_depositIndex.addDeposit(deposit);
        totalBurned += depositAmounts[i];
        
        // Add to dynamic supply
        m_dynamicSupply.addBurnedXfg(depositAmounts[i]);
        
        // Verify cumulative tracking
        EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), totalBurned);
        EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), totalBurned);
        EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), totalBurned);
    }
    
    // Verify final totals
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), totalBurned);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + totalBurned);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
}

// Test 3: Regular Deposit vs FOREVER Deposit
TEST_F(DynamicSupplyIntegrationTest, RegularVsForeverDeposits) {
    uint64_t regularAmount = 500000000000ULL;  // 500,000 XFG
    uint64_t foreverAmount = 300000000000ULL;  // 300,000 XFG
    
    // Create regular deposit (not burned)
    Deposit regularDeposit;
    regularDeposit.amount = regularAmount;
    regularDeposit.term = 1000; // Regular term
    regularDeposit.creatingTransactionHash = Crypto::Hash();
    regularDeposit.spendingTransactionHash = Crypto::Hash();
    regularDeposit.height = 1000;
    regularDeposit.unlockHeight = 2000;
    
    // Create FOREVER deposit (burned)
    Deposit foreverDeposit;
    foreverDeposit.amount = foreverAmount;
    foreverDeposit.term = FOREVER_TERM;
    foreverDeposit.creatingTransactionHash = Crypto::Hash();
    foreverDeposit.spendingTransactionHash = Crypto::Hash();
    foreverDeposit.height = 1001;
    foreverDeposit.unlockHeight = 0;
    
    // Add both deposits
    m_depositIndex.addDeposit(regularDeposit);
    m_depositIndex.addDeposit(foreverDeposit);
    
    // Only FOREVER deposit should be counted as burned
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), foreverAmount);
    EXPECT_EQ(m_depositIndex.getTotalLockedAmount(), regularAmount);
    
    // Add burned amount to dynamic supply
    m_dynamicSupply.addBurnedXfg(foreverAmount);
    
    // Verify dynamic supply only reflects FOREVER deposit
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), foreverAmount);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + foreverAmount);
}

// Test 4: Currency Integration with Deposits
TEST_F(DynamicSupplyIntegrationTest, CurrencyIntegrationWithDeposits) {
    uint64_t burnAmount = TEST_DEPOSIT_AMOUNT;
    
    // Create FOREVER deposit
    Deposit deposit;
    deposit.amount = burnAmount;
    deposit.term = FOREVER_TERM;
    deposit.creatingTransactionHash = Crypto::Hash();
    deposit.spendingTransactionHash = Crypto::Hash();
    deposit.height = 1000;
    deposit.unlockHeight = 0;
    
    m_depositIndex.addDeposit(deposit);
    
    // Update currency with burned amount
    m_currency.addBurnedXfg(burnAmount);
    
    // Verify currency methods work correctly
    EXPECT_EQ(m_currency.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_currency.getBlockRewardSupply(), BASE_MONEY_SUPPLY + burnAmount);
    EXPECT_EQ(m_currency.getCirculatingSupply(), BASE_MONEY_SUPPLY);
    
    // Verify currency and dynamic supply are in sync
    EXPECT_EQ(m_currency.getTotalSupply(), m_dynamicSupply.getTotalSupply());
    EXPECT_EQ(m_currency.getBlockRewardSupply(), m_dynamicSupply.getBlockRewardSupply());
}

// Test 5: Deposit Index State Persistence
TEST_F(DynamicSupplyIntegrationTest, DepositIndexStatePersistence) {
    uint64_t depositAmount = TEST_DEPOSIT_AMOUNT;
    
    // Create and add deposit
    Deposit deposit;
    deposit.amount = depositAmount;
    deposit.term = FOREVER_TERM;
    deposit.creatingTransactionHash = Crypto::Hash();
    deposit.spendingTransactionHash = Crypto::Hash();
    deposit.height = 1000;
    deposit.unlockHeight = 0;
    
    m_depositIndex.addDeposit(deposit);
    
    // Serialize deposit index state
    std::string serialized = m_depositIndex.serialize();
    EXPECT_FALSE(serialized.empty());
    
    // Create new deposit index and deserialize
    DepositIndex newDepositIndex;
    newDepositIndex.deserialize(serialized);
    
    // Verify deserialized state matches original
    EXPECT_EQ(newDepositIndex.getTotalBurnedAmount(), m_depositIndex.getTotalBurnedAmount());
    EXPECT_EQ(newDepositIndex.getTotalLockedAmount(), m_depositIndex.getTotalLockedAmount());
}

// Test 6: Block Reward Calculation Integration
TEST_F(DynamicSupplyIntegrationTest, BlockRewardCalculationIntegration) {
    uint64_t burnAmount = BASE_MONEY_SUPPLY / 10; // 10% of base supply
    
    // Create FOREVER deposit
    Deposit deposit;
    deposit.amount = burnAmount;
    deposit.term = FOREVER_TERM;
    deposit.creatingTransactionHash = Crypto::Hash();
    deposit.spendingTransactionHash = Crypto::Hash();
    deposit.height = 1000;
    deposit.unlockHeight = 0;
    
    m_depositIndex.addDeposit(deposit);
    m_dynamicSupply.addBurnedXfg(burnAmount);
    m_currency.addBurnedXfg(burnAmount);
    
    // Test block reward calculation
    uint64_t blockReward = m_currency.getBlockReward(1000, 0, 0);
    
    // Verify block reward is calculated from increased base supply
    EXPECT_GT(blockReward, 0);
    
    // Verify the reward pool has increased
    uint64_t rewardPool = m_currency.getBlockRewardSupply();
    EXPECT_EQ(rewardPool, BASE_MONEY_SUPPLY + burnAmount);
}

// Test 7: Large Scale Integration Test
TEST_F(DynamicSupplyIntegrationTest, LargeScaleIntegrationTest) {
    const int numDeposits = 100;
    uint64_t depositAmount = 10000000000ULL; // 10,000 XFG each
    uint64_t totalBurned = depositAmount * numDeposits;
    
    // Create many FOREVER deposits
    for (int i = 0; i < numDeposits; ++i) {
        Deposit deposit;
        deposit.amount = depositAmount;
        deposit.term = FOREVER_TERM;
        deposit.creatingTransactionHash = Crypto::Hash();
        deposit.spendingTransactionHash = Crypto::Hash();
        deposit.height = 1000 + i;
        deposit.unlockHeight = 0;
        
        m_depositIndex.addDeposit(deposit);
        m_dynamicSupply.addBurnedXfg(depositAmount);
        m_currency.addBurnedXfg(depositAmount);
    }
    
    // Verify all components are in sync
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), totalBurned);
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), totalBurned);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), totalBurned);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + totalBurned);
    EXPECT_EQ(m_currency.getBlockRewardSupply(), BASE_MONEY_SUPPLY + totalBurned);
    
    // Verify economic balance
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_currency.getTotalSupply(), BASE_MONEY_SUPPLY);
}

// Test 8: Error Handling and Edge Cases
TEST_F(DynamicSupplyIntegrationTest, ErrorHandlingAndEdgeCases) {
    // Test with zero amount deposit
    Deposit zeroDeposit;
    zeroDeposit.amount = 0;
    zeroDeposit.term = FOREVER_TERM;
    zeroDeposit.creatingTransactionHash = Crypto::Hash();
    zeroDeposit.spendingTransactionHash = Crypto::Hash();
    zeroDeposit.height = 1000;
    zeroDeposit.unlockHeight = 0;
    
    m_depositIndex.addDeposit(zeroDeposit);
    m_dynamicSupply.addBurnedXfg(0);
    
    // Verify zero amount doesn't affect totals
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), 0);
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), 0);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY);
    
    // Test with maximum possible amount
    uint64_t maxAmount = UINT64_MAX - BASE_MONEY_SUPPLY;
    
    Deposit maxDeposit;
    maxDeposit.amount = maxAmount;
    maxDeposit.term = FOREVER_TERM;
    maxDeposit.creatingTransactionHash = Crypto::Hash();
    maxDeposit.spendingTransactionHash = Crypto::Hash();
    maxDeposit.height = 1001;
    maxDeposit.unlockHeight = 0;
    
    m_depositIndex.addDeposit(maxDeposit);
    m_dynamicSupply.addBurnedXfg(maxAmount);
    
    // Verify large amount is handled correctly
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), maxAmount);
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), maxAmount);
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + maxAmount);
}

// Test 9: State Validation Across Components
TEST_F(DynamicSupplyIntegrationTest, StateValidationAcrossComponents) {
    uint64_t burnAmount = TEST_DEPOSIT_AMOUNT;
    
    // Create deposit and update all components
    Deposit deposit;
    deposit.amount = burnAmount;
    deposit.term = FOREVER_TERM;
    deposit.creatingTransactionHash = Crypto::Hash();
    deposit.spendingTransactionHash = Crypto::Hash();
    deposit.height = 1000;
    deposit.unlockHeight = 0;
    
    m_depositIndex.addDeposit(deposit);
    m_dynamicSupply.addBurnedXfg(burnAmount);
    m_currency.addBurnedXfg(burnAmount);
    
    // Verify all components are consistent
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), m_dynamicSupply.getTotalBurnedXfg());
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), m_dynamicSupply.getTotalRebornXfg());
    EXPECT_EQ(m_dynamicSupply.getBaseMoneySupply(), BASE_MONEY_SUPPLY + burnAmount);
    EXPECT_EQ(m_dynamicSupply.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_currency.getTotalSupply(), BASE_MONEY_SUPPLY);
    EXPECT_EQ(m_currency.getBlockRewardSupply(), BASE_MONEY_SUPPLY + burnAmount);
    
    // Verify state validation passes
    EXPECT_TRUE(m_dynamicSupply.validateState());
}

// Test 10: Performance Test
TEST_F(DynamicSupplyIntegrationTest, PerformanceTest) {
    const int numOperations = 10000;
    uint64_t operationAmount = 1000000ULL; // 0.1 XFG each
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        Deposit deposit;
        deposit.amount = operationAmount;
        deposit.term = FOREVER_TERM;
        deposit.creatingTransactionHash = Crypto::Hash();
        deposit.spendingTransactionHash = Crypto::Hash();
        deposit.height = 1000 + i;
        deposit.unlockHeight = 0;
        
        m_depositIndex.addDeposit(deposit);
        m_dynamicSupply.addBurnedXfg(operationAmount);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Verify performance is reasonable (should complete in reasonable time)
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds for 10,000 operations
    
    // Verify final state is correct
    uint64_t expectedTotal = operationAmount * numOperations;
    EXPECT_EQ(m_depositIndex.getTotalBurnedAmount(), expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), expectedTotal);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
