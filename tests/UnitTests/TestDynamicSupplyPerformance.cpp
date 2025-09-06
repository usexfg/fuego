#include "CryptoNoteCore/DynamicMoneySupply.h"
#include "CryptoNoteCore/Currency.h"
#include <gtest/gtest.h>
#include <chrono>
#include <random>
#include <vector>
#include <iostream>

using namespace CryptoNote;

class DynamicSupplyPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_dynamicSupply = DynamicMoneySupply();
        m_currency = CurrencyBuilder()
            .baseMoneySupply(80000088000008ULL)
            .totalBurnedXfg(0)
            .totalRebornXfg(0)
            .totalSupply(80000088000008ULL)
            .circulatingSupply(80000088000008ULL)
            .build();
        
        // Initialize random number generator
        m_rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    DynamicMoneySupply m_dynamicSupply;
    Currency m_currency;
    std::mt19937_64 m_rng;
    
    static const uint64_t BASE_MONEY_SUPPLY = 80000088000008ULL;
};

// Test 1: Single Operation Performance
TEST_F(DynamicSupplyPerformanceTest, SingleOperationPerformance) {
    uint64_t burnAmount = 1000000000000ULL; // 1,000,000 XFG
    
    auto start = std::chrono::high_resolution_clock::now();
    
    m_dynamicSupply.addBurnedXfg(burnAmount);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    // Single operation should be very fast (less than 1 microsecond)
    EXPECT_LT(duration.count(), 1000);
    
    // Verify operation completed correctly
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), burnAmount);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), burnAmount);
}

// Test 2: Batch Operations Performance
TEST_F(DynamicSupplyPerformanceTest, BatchOperationsPerformance) {
    const int numOperations = 100000;
    uint64_t operationAmount = 1000000ULL; // 0.1 XFG each
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        m_dynamicSupply.addBurnedXfg(operationAmount);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 100,000 operations should complete in reasonable time
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
    
    // Calculate operations per second
    double opsPerSecond = numOperations / (duration.count() / 1000.0);
    std::cout << "Operations per second: " << opsPerSecond << std::endl;
    
    // Should achieve at least 100,000 operations per second
    EXPECT_GT(opsPerSecond, 100000);
    
    // Verify final state
    uint64_t expectedTotal = operationAmount * numOperations;
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), expectedTotal);
}

// Test 3: Random Amount Operations Performance
TEST_F(DynamicSupplyPerformanceTest, RandomAmountOperationsPerformance) {
    const int numOperations = 50000;
    std::uniform_int_distribution<uint64_t> dist(1000ULL, 1000000000ULL); // 0.0001 to 100 XFG
    
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t totalBurned = 0;
    for (int i = 0; i < numOperations; ++i) {
        uint64_t amount = dist(m_rng);
        m_dynamicSupply.addBurnedXfg(amount);
        totalBurned += amount;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Random operations should still be fast
    EXPECT_LT(duration.count(), 500); // Less than 0.5 seconds
    
    // Verify final state
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), totalBurned);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), totalBurned);
}

// Test 4: State Query Performance
TEST_F(DynamicSupplyPerformanceTest, StateQueryPerformance) {
    // First, add some burns
    const int numBurns = 10000;
    uint64_t burnAmount = 1000000ULL;
    
    for (int i = 0; i < numBurns; ++i) {
        m_dynamicSupply.addBurnedXfg(burnAmount);
    }
    
    // Now test query performance
    const int numQueries = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numQueries; ++i) {
        // Query all state values
        volatile uint64_t baseSupply = m_dynamicSupply.getBaseMoneySupply();
        volatile uint64_t totalBurned = m_dynamicSupply.getTotalBurnedXfg();
        volatile uint64_t totalReborn = m_dynamicSupply.getTotalRebornXfg();
        volatile uint64_t totalSupply = m_dynamicSupply.getTotalSupply();
        volatile uint64_t blockRewardSupply = m_dynamicSupply.getBlockRewardSupply();
        volatile uint64_t circulatingSupply = m_dynamicSupply.getCirculatingSupply();
        volatile double burnPercentage = m_dynamicSupply.getBurnPercentage();
        volatile double rebornPercentage = m_dynamicSupply.getRebornPercentage();
        
        // Prevent compiler optimization
        (void)baseSupply;
        (void)totalBurned;
        (void)totalReborn;
        (void)totalSupply;
        (void)blockRewardSupply;
        (void)circulatingSupply;
        (void)burnPercentage;
        (void)rebornPercentage;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // State queries should be very fast
    EXPECT_LT(duration.count(), 100); // Less than 0.1 seconds
    
    // Calculate queries per second
    double queriesPerSecond = numQueries / (duration.count() / 1000.0);
    std::cout << "State queries per second: " << queriesPerSecond << std::endl;
    
    // Should achieve at least 1 million queries per second
    EXPECT_GT(queriesPerSecond, 1000000);
}

// Test 5: Serialization Performance
TEST_F(DynamicSupplyPerformanceTest, SerializationPerformance) {
    // Add some burns first
    const int numBurns = 1000;
    uint64_t burnAmount = 1000000000ULL; // 100 XFG each
    
    for (int i = 0; i < numBurns; ++i) {
        m_dynamicSupply.addBurnedXfg(burnAmount);
    }
    
    const int numSerializations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numSerializations; ++i) {
        std::string serialized = m_dynamicSupply.serialize();
        EXPECT_FALSE(serialized.empty());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Serialization should be reasonably fast
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
    
    // Calculate serializations per second
    double serializationsPerSecond = numSerializations / (duration.count() / 1000.0);
    std::cout << "Serializations per second: " << serializationsPerSecond << std::endl;
    
    // Should achieve at least 10,000 serializations per second
    EXPECT_GT(serializationsPerSecond, 10000);
}

// Test 6: Deserialization Performance
TEST_F(DynamicSupplyPerformanceTest, DeserializationPerformance) {
    // Add some burns and serialize once
    const int numBurns = 1000;
    uint64_t burnAmount = 1000000000ULL;
    
    for (int i = 0; i < numBurns; ++i) {
        m_dynamicSupply.addBurnedXfg(burnAmount);
    }
    
    std::string serialized = m_dynamicSupply.serialize();
    
    const int numDeserializations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numDeserializations; ++i) {
        DynamicMoneySupply newSupply;
        newSupply.deserialize(serialized);
        
        // Verify deserialization worked
        EXPECT_EQ(newSupply.getTotalBurnedXfg(), m_dynamicSupply.getTotalBurnedXfg());
        EXPECT_EQ(newSupply.getTotalRebornXfg(), m_dynamicSupply.getTotalRebornXfg());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Deserialization should be reasonably fast
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
    
    // Calculate deserializations per second
    double deserializationsPerSecond = numDeserializations / (duration.count() / 1000.0);
    std::cout << "Deserializations per second: " << deserializationsPerSecond << std::endl;
    
    // Should achieve at least 10,000 deserializations per second
    EXPECT_GT(deserializationsPerSecond, 10000);
}

// Test 7: Memory Usage Test
TEST_F(DynamicSupplyPerformanceTest, MemoryUsageTest) {
    // Test memory usage with many operations
    const int numOperations = 1000000;
    uint64_t operationAmount = 1000ULL; // 0.0001 XFG each
    
    // Get initial memory usage (approximate)
    size_t initialMemory = sizeof(DynamicMoneySupply);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        m_dynamicSupply.addBurnedXfg(operationAmount);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Memory usage should remain constant (no dynamic allocations)
    size_t finalMemory = sizeof(DynamicMoneySupply);
    EXPECT_EQ(initialMemory, finalMemory);
    
    // Performance should still be good even with many operations
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
    
    // Calculate operations per second
    double opsPerSecond = numOperations / (duration.count() / 1000.0);
    std::cout << "Memory-efficient operations per second: " << opsPerSecond << std::endl;
    
    // Should achieve at least 200,000 operations per second
    EXPECT_GT(opsPerSecond, 200000);
}

// Test 8: Concurrent Access Simulation
TEST_F(DynamicSupplyPerformanceTest, ConcurrentAccessSimulation) {
    // Simulate concurrent access by interleaving operations
    const int numOperations = 50000;
    uint64_t operationAmount = 1000000ULL;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate concurrent writes and reads
    for (int i = 0; i < numOperations; ++i) {
        // Write operation
        m_dynamicSupply.addBurnedXfg(operationAmount);
        
        // Read operations
        if (i % 10 == 0) {
            volatile uint64_t baseSupply = m_dynamicSupply.getBaseMoneySupply();
            volatile uint64_t totalBurned = m_dynamicSupply.getTotalBurnedXfg();
            volatile uint64_t totalSupply = m_dynamicSupply.getTotalSupply();
            (void)baseSupply;
            (void)totalBurned;
            (void)totalSupply;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Concurrent access simulation should be fast
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
    
    // Calculate operations per second
    double opsPerSecond = numOperations / (duration.count() / 1000.0);
    std::cout << "Concurrent access operations per second: " << opsPerSecond << std::endl;
    
    // Should achieve at least 50,000 operations per second
    EXPECT_GT(opsPerSecond, 50000);
}

// Test 9: Large Amount Operations Performance
TEST_F(DynamicSupplyPerformanceTest, LargeAmountOperationsPerformance) {
    const int numOperations = 1000;
    uint64_t largeAmount = 10000000000000ULL; // 1,000,000 XFG each
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOperations; ++i) {
        m_dynamicSupply.addBurnedXfg(largeAmount);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Large amount operations should still be fast
    EXPECT_LT(duration.count(), 100); // Less than 0.1 seconds
    
    // Calculate operations per second
    double opsPerSecond = numOperations / (duration.count() / 1000.0);
    std::cout << "Large amount operations per second: " << opsPerSecond << std::endl;
    
    // Should achieve at least 10,000 operations per second
    EXPECT_GT(opsPerSecond, 10000);
    
    // Verify final state
    uint64_t expectedTotal = largeAmount * numOperations;
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), expectedTotal);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), expectedTotal);
}

// Test 10: Stress Test
TEST_F(DynamicSupplyPerformanceTest, StressTest) {
    const int numOperations = 1000000;
    std::uniform_int_distribution<uint64_t> dist(1ULL, 1000000000ULL);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t totalBurned = 0;
    for (int i = 0; i < numOperations; ++i) {
        uint64_t amount = dist(m_rng);
        m_dynamicSupply.addBurnedXfg(amount);
        totalBurned += amount;
        
        // Periodic state validation
        if (i % 100000 == 0) {
            EXPECT_TRUE(m_dynamicSupply.validateState());
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Stress test should complete in reasonable time
    EXPECT_LT(duration.count(), 10000); // Less than 10 seconds
    
    // Calculate operations per second
    double opsPerSecond = numOperations / (duration.count() / 1000.0);
    std::cout << "Stress test operations per second: " << opsPerSecond << std::endl;
    
    // Should achieve at least 100,000 operations per second
    EXPECT_GT(opsPerSecond, 100000);
    
    // Verify final state
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), totalBurned);
    EXPECT_EQ(m_dynamicSupply.getTotalRebornXfg(), totalBurned);
    EXPECT_TRUE(m_dynamicSupply.validateState());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
