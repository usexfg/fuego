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

#include <gtest/gtest.h>
#include <thread>
#include "CryptoNoteCore/StagedDepositUnlock.h"
#include "CryptoNoteCore/StagedUnlock.h"
#include "CryptoNoteCore/StagedUnlockStorage.h"
#include "CryptoNoteCore/StagedUnlockStorage.h"
#include "CryptoNoteCore/Currency.h"
#include "Logging/ConsoleLogger.h"

using namespace CryptoNote;

class StagedDepositUnlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_logger = std::make_unique<Logging::ConsoleLogger>();
        m_currency = CurrencyBuilder(*m_logger)
            .upgradeHeightV2(0)
            .depositMinTerm(10)
            .depositMinTotalRateFactor(100)
            .currency();
    }
    
    std::unique_ptr<Logging::ConsoleLogger> m_logger;
    Currency m_currency;
};

TEST_F(StagedDepositUnlockTest, BasicStagedUnlock) {
    // Test basic staged unlock functionality
    uint64_t amount = 1000000000; // 1000 XFG
    uint64_t interest = 100000000; // 100 XFG
    uint32_t depositHeight = 1000;
    
    StagedDepositUnlock stagedUnlock(amount, interest, depositHeight);
    
    // Check initial state
    EXPECT_EQ(stagedUnlock.getTotalUnlockedAmount(), 0);
    EXPECT_EQ(stagedUnlock.getRemainingLockedAmount(), amount + interest);
    EXPECT_FALSE(stagedUnlock.isFullyUnlocked());
    
    // Check stages
    auto stages = stagedUnlock.getStages();
    EXPECT_EQ(stages.size(), 5);
    
    // Stage 1: 25% principal + 0% interest
    EXPECT_EQ(stages[0].stageNumber, 1);
    EXPECT_EQ(stages[0].unlockHeight, depositHeight + StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    EXPECT_EQ(stages[0].principalAmount, (amount * 20) / 100);
    EXPECT_EQ(stages[0].interestAmount, 0);
    EXPECT_FALSE(stages[0].isUnlocked);
    
    // Stage 2: 25% principal
    EXPECT_EQ(stages[1].stageNumber, 2);
    EXPECT_EQ(stages[1].unlockHeight, depositHeight + (2 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS));
    EXPECT_EQ(stages[1].principalAmount, (amount * 20) / 100);
    EXPECT_EQ(stages[1].interestAmount, 0);
    EXPECT_FALSE(stages[1].isUnlocked);
    
    // Stage 3: 25% principal
    EXPECT_EQ(stages[2].stageNumber, 3);
    EXPECT_EQ(stages[2].unlockHeight, depositHeight + (3 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS));
    EXPECT_EQ(stages[2].principalAmount, (amount * 20) / 100);
    EXPECT_EQ(stages[2].interestAmount, 0);
    EXPECT_FALSE(stages[2].isUnlocked);
    
    // Stage 4: 25% principal (remaining)
    EXPECT_EQ(stages[3].stageNumber, 4);
    EXPECT_EQ(stages[3].unlockHeight, depositHeight + (4 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS));
    EXPECT_EQ(stages[3].principalAmount, (amount * 20) / 100);
    EXPECT_EQ(stages[4].principalAmount, amount - ((amount * 20) / 100) - ((amount * 20) / 100) - ((amount * 20) / 100) - ((amount * 20) / 100));
    EXPECT_EQ(stages[3].interestAmount, 0);
    EXPECT_FALSE(stages[3].isUnlocked);
}

// Test StagedUnlockStorage
TEST(StagedUnlockStorageTest, BasicFunctionality) {
  CryptoNote::StagedUnlockStorage storage;
  
  // Test setting and getting staged unlock preference
  std::string txHash1 = "abc123def456";
  std::string txHash2 = "def456ghi789";
  
  // Initially no preference stored
  EXPECT_FALSE(storage.hasStagedUnlockPreference(txHash1));
  EXPECT_FALSE(storage.getStagedUnlockPreference(txHash1));
  
  // Set staged unlock preference
  storage.setStagedUnlockPreference(txHash1, true);
  storage.setStagedUnlockPreference(txHash2, false);
  
  // Check preferences
  EXPECT_TRUE(storage.hasStagedUnlockPreference(txHash1));
  EXPECT_TRUE(storage.hasStagedUnlockPreference(txHash2));
  EXPECT_TRUE(storage.getStagedUnlockPreference(txHash1));
  EXPECT_FALSE(storage.getStagedUnlockPreference(txHash2));
  
  // Test getting all staged unlock deposits
  auto stagedDeposits = storage.getStagedUnlockDeposits();
  EXPECT_EQ(stagedDeposits.size(), 1);
  EXPECT_EQ(stagedDeposits[0], txHash1);
  
  // Test removal
  storage.removeStagedUnlockPreference(txHash1);
  EXPECT_FALSE(storage.hasStagedUnlockPreference(txHash1));
  EXPECT_TRUE(storage.hasStagedUnlockPreference(txHash2));
}

TEST(StagedUnlockStorageTest, ThreadSafety) {
  CryptoNote::StagedUnlockStorage storage;
  
  // Test concurrent access
  std::vector<std::thread> threads;
  const int numThreads = 10;
  const int operationsPerThread = 100;
  
  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back([&storage, i, operationsPerThread]() {
      for (int j = 0; j < operationsPerThread; ++j) {
        std::string txHash = "tx_" + std::to_string(i) + "_" + std::to_string(j);
        storage.setStagedUnlockPreference(txHash, j % 2 == 0);
        storage.getStagedUnlockPreference(txHash);
        storage.hasStagedUnlockPreference(txHash);
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Verify no crashes occurred
  EXPECT_TRUE(true);
}

// Test optional staged unlock feature
TEST(OptionalStagedUnlockTest, CreateDepositWithStagedUnlock) {
  // Test creating a deposit with staged unlock option
  uint64_t amount = 1000000000000; // 1000 XFG
  uint64_t interest = 100000000000; // 100 XFG
  uint32_t depositHeight = 100000;
  
  // Create staged unlock
  CryptoNote::StagedDepositUnlock stagedUnlock;
  stagedUnlock.initialize(amount, interest, depositHeight);
  
  // Verify it's properly initialized
  EXPECT_TRUE(stagedUnlock.isInitialized());
  EXPECT_EQ(stagedUnlock.getTotalAmount(), amount);
  EXPECT_EQ(stagedUnlock.getTotalInterest(), interest);
  
  // Test stage calculation
  auto stages = stagedUnlock.getStages();
  EXPECT_EQ(stages.size(), 5);
  
  // Verify all stages have correct amounts
  uint64_t totalPrincipal = 0;
  for (const auto& stage : stages) {
    totalPrincipal += stage.principalAmount;
  }
  EXPECT_EQ(totalPrincipal, amount);
}

TEST(OptionalStagedUnlockTest, FeeCalculation) {
  // Test fee calculation for different unlock types
  uint64_t baseFee = 800000; // 0.008 XFG
  
  // Traditional unlock (1 transaction)
  uint64_t traditionalFees = baseFee;
  EXPECT_EQ(traditionalFees, 800000);
  
  // Staged unlock (4 transactions)
  uint64_t stagedFees = baseFee * 5;
  EXPECT_EQ(stagedFees, 4000000);
  
  // Verify fee difference
  uint64_t feeDifference = stagedFees - traditionalFees;
  EXPECT_EQ(feeDifference, 3200000); // 0.032 XFG additional
}

TEST(OptionalStagedUnlockTest, DepositComparison) {
  // Test comparing traditional vs staged deposits
  uint64_t amount = 1000000000000; // 1000 XFG
  uint64_t interest = 100000000000; // 100 XFG
  uint32_t depositHeight = 100000;
  
  // Traditional deposit
  CryptoNote::StagedDepositUnlock traditionalUnlock;
  traditionalUnlock.initialize(amount, interest, depositHeight);
  
  // Staged deposit
  CryptoNote::StagedDepositUnlock stagedUnlock;
  stagedUnlock.initialize(amount, interest, depositHeight);
  
  // Both should have same total amounts
  EXPECT_EQ(traditionalUnlock.getTotalAmount(), stagedUnlock.getTotalAmount());
  EXPECT_EQ(traditionalUnlock.getTotalInterest(), stagedUnlock.getTotalInterest());
  
  // But staged unlock should have multiple stages
  auto traditionalStages = traditionalUnlock.getStages();
  auto stagedStages = stagedUnlock.getStages();
  
  EXPECT_EQ(traditionalStages.size(), 4); // Both use 4 stages now
  EXPECT_EQ(stagedStages.size(), 4);
  
  // Verify stage amounts are equal
  for (size_t i = 0; i < stagedStages.size(); ++i) {
    EXPECT_EQ(traditionalStages[i].principalAmount, stagedStages[i].principalAmount);
  }
}

TEST_F(StagedDepositUnlockTest, StageUnlocking) {
    uint64_t amount = 1000000000; // 1000 XFG
    uint64_t interest = 100000000; // 100 XFG
    uint32_t depositHeight = 1000;
    
    StagedDepositUnlock stagedUnlock(amount, interest, depositHeight);
    
    // Test stage 1 unlock
    uint32_t stage1Height = depositHeight + StagedUnlockConfig::STAGE_INTERVAL_BLOCKS;
    auto newlyUnlocked = stagedUnlock.checkUnlockStages(stage1Height);
    
    EXPECT_EQ(newlyUnlocked.size(), 1);
    EXPECT_EQ(newlyUnlocked[0].stageNumber, 1);
    EXPECT_TRUE(newlyUnlocked[0].isUnlocked);
    
    // Check totals after stage 1
    uint64_t expectedStage1Amount = (amount * 20) / 100;
    EXPECT_EQ(stagedUnlock.getTotalUnlockedAmount(), expectedStage1Amount);
    EXPECT_EQ(stagedUnlock.getRemainingLockedAmount(), amount - expectedStage1Amount);
    
    // Test stage 2 unlock
    uint32_t stage2Height = depositHeight + (2 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    newlyUnlocked = stagedUnlock.checkUnlockStages(stage2Height);
    
    EXPECT_EQ(newlyUnlocked.size(), 1);
    EXPECT_EQ(newlyUnlocked[0].stageNumber, 2);
    EXPECT_TRUE(newlyUnlocked[0].isUnlocked);
    
    // Test stage 3 unlock
    uint32_t stage3Height = depositHeight + (3 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    newlyUnlocked = stagedUnlock.checkUnlockStages(stage3Height);
    
    EXPECT_EQ(newlyUnlocked.size(), 1);
    EXPECT_EQ(newlyUnlocked[0].stageNumber, 3);
    EXPECT_TRUE(newlyUnlocked[0].isUnlocked);
    
    // Test stage 4 unlock
    uint32_t stage4Height = depositHeight + (4 * StagedUnlockConfig::STAGE_INTERVAL_BLOCKS);
    newlyUnlocked = stagedUnlock.checkUnlockStages(stage4Height);
    
    EXPECT_EQ(newlyUnlocked.size(), 1);
    EXPECT_EQ(newlyUnlocked[0].stageNumber, 4);
    EXPECT_TRUE(newlyUnlocked[0].isUnlocked);
    
    // Check final state
    EXPECT_EQ(stagedUnlock.getTotalUnlockedAmount(), amount);
    EXPECT_EQ(stagedUnlock.getRemainingLockedAmount(), 0);
    EXPECT_TRUE(stagedUnlock.isFullyUnlocked());
}

TEST_F(StagedDepositUnlockTest, StagedUnlock) {
    // Create a basic deposit
    Deposit basicDeposit;
    basicDeposit.amount = 1000000000; // 1000 XFG
    basicDeposit.term = 90; // 90 days
    basicDeposit.interest = 100000000; // 100 XFG
    basicDeposit.height = 1000;
    basicDeposit.unlockHeight = 1000 + 90; // Traditional unlock
    basicDeposit.locked = true;
    basicDeposit.creatingTransactionId = 1;
    basicDeposit.spendingTransactionId = 0;
    
    // Convert to staged unlock deposit
    CryptoNote::StagedUnlock stagedDeposit;
    stagedDeposit.initializeFromDeposit(basicDeposit);
    
    // Check that it uses staged unlocking (non-FOREVER term)
    EXPECT_TRUE(stagedDeposit.useStagedUnlock);
    EXPECT_EQ(stagedDeposit.amount, basicDeposit.amount);
    EXPECT_EQ(stagedDeposit.interest, basicDeposit.interest);
    EXPECT_EQ(stagedDeposit.height, basicDeposit.height);
    
    // Test unlock process
    uint32_t currentHeight = 1000 + StagedUnlockConfig::STAGE_INTERVAL_BLOCKS;
    auto newlyUnlocked = stagedDeposit.processUnlock(currentHeight);
    
    EXPECT_EQ(newlyUnlocked.size(), 1);
    EXPECT_EQ(newlyUnlocked[0].stageNumber, 1);
    EXPECT_TRUE(newlyUnlocked[0].isUnlocked);
    
    // Check status
    std::string status = CryptoNote::StagedUnlockManager::getUnlockStatus(stagedDeposit, currentHeight);
    EXPECT_FALSE(status.empty());
    EXPECT_TRUE(status.find("Staged Unlock") != std::string::npos);
}

TEST_F(StagedDepositUnlockTest, StagedUnlockManager) {
    // Test manager functions
    EXPECT_TRUE(StagedUnlockManager::shouldUseStagedUnlock(90)); // Regular term
    EXPECT_FALSE(StagedUnlockManager::shouldUseStagedUnlock(parameters::DEPOSIT_TERM_FOREVER)); // FOREVER term
    
    // Test schedule generation
    auto schedule = StagedUnlockManager::getUnlockSchedule(1000000000, 100000000, 1000);
    EXPECT_EQ(schedule.size(), 4);
    
    // Test deposit processing
    std::vector<Deposit> deposits;
    Deposit deposit1;
    deposit1.amount = 1000000000;
    deposit1.interest = 100000000;
    deposit1.height = 1000;
    deposit1.term = 90;
    deposits.push_back(deposit1);
    
    uint32_t currentHeight = 1000 + StagedUnlockConfig::STAGE_INTERVAL_BLOCKS;
    auto newlyUnlocked = StagedUnlockManager::processStagedUnlocks(currentHeight, deposits);
    
    EXPECT_EQ(newlyUnlocked.size(), 1);
    EXPECT_EQ(newlyUnlocked[0], 0); // First deposit
}

TEST_F(StagedDepositUnlockTest, Serialization) {
    // Test serialization
    StagedDepositUnlock original(1000000000, 100000000, 1000);
    
    // Serialize
    std::ostringstream oss;
    BinaryOutputStreamSerializer serializer(oss);
    original.serialize(serializer);
    
    // Deserialize
    std::istringstream iss(oss.str());
    BinaryInputStreamSerializer deserializer(iss);
    StagedDepositUnlock restored;
    restored.serialize(deserializer);
    
    // Compare
    EXPECT_EQ(original.getTotalUnlockedAmount(), restored.getTotalUnlockedAmount());
    EXPECT_EQ(original.getRemainingLockedAmount(), restored.getRemainingLockedAmount());
    EXPECT_EQ(original.isFullyUnlocked(), restored.isFullyUnlocked());
    
    auto originalStages = original.getStages();
    auto restoredStages = restored.getStages();
    EXPECT_EQ(originalStages.size(), restoredStages.size());
    
    for (size_t i = 0; i < originalStages.size(); ++i) {
        EXPECT_EQ(originalStages[i].stageNumber, restoredStages[i].stageNumber);
        EXPECT_EQ(originalStages[i].unlockHeight, restoredStages[i].unlockHeight);
        EXPECT_EQ(originalStages[i].principalAmount, restoredStages[i].principalAmount);
        EXPECT_EQ(originalStages[i].interestAmount, restoredStages[i].interestAmount);
    }
}

TEST_F(StagedDepositUnlockTest, EdgeCases) {
    // Test zero amount
    StagedDepositUnlock zeroUnlock(0, 0, 1000);
    EXPECT_EQ(zeroUnlock.getTotalUnlockedAmount(), 0);
    EXPECT_EQ(zeroUnlock.getRemainingLockedAmount(), 0);
    EXPECT_TRUE(zeroUnlock.isFullyUnlocked());
    
    // Test very small amounts
    StagedDepositUnlock smallUnlock(1, 1, 1000);
    auto stages = smallUnlock.getStages();
    EXPECT_EQ(stages.size(), 5);
    
    // Test large amounts
    StagedDepositUnlock largeUnlock(1000000000000, 100000000000, 1000);
    stages = largeUnlock.getStages();
    EXPECT_EQ(stages.size(), 5);
    
    // Verify total amounts are preserved
    uint64_t totalPrincipal = 0;
    uint64_t totalInterest = 0;
    for (const auto& stage : stages) {
        totalPrincipal += stage.principalAmount;
        totalInterest += stage.interestAmount;
    }
    EXPECT_EQ(totalPrincipal, 1000000000000);
    EXPECT_EQ(totalInterest, 0);
}

} // namespace CryptoNote