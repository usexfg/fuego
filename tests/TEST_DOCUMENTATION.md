# 🧪 Dynamic Supply Test Suite

## Overview

This document describes the comprehensive test suite for the Fuego Dynamic Supply System. The test suite includes unit tests, integration tests, performance tests, and simulation tests to ensure the dynamic supply system works correctly and efficiently.

## Test Structure

### 1. **Unit Tests** (`TestDynamicMoneySupply.cpp`)
Tests the core `DynamicMoneySupply` class functionality:

- **Initial State Verification**: Ensures proper initialization
- **Single Burn Operation**: Tests basic burn functionality
- **Multiple Burns**: Tests cumulative burn tracking
- **Large Burn Amounts**: Tests with significant amounts
- **Zero Burn Handling**: Edge case testing
- **Percentage Calculations**: Tests burn/reborn percentages
- **State Serialization**: Tests persistence functionality
- **State Validation**: Tests economic balance verification
- **Clear State**: Tests state reset functionality
- **Currency Integration**: Tests integration with Currency class
- **Edge Cases**: Maximum amounts, overflow protection
- **Stress Tests**: Many small operations
- **Block Reward Scaling**: Tests proportional scaling
- **Economic Balance**: Comprehensive balance verification
- **Supply Cap Enforcement**: Tests supply limits

### 2. **Integration Tests** (`TestDynamicSupplyIntegration.cpp`)
Tests integration between dynamic supply and blockchain components:

- **FOREVER Deposit Creation**: Tests burn deposit tracking
- **Multiple FOREVER Deposits**: Tests cumulative deposit handling
- **Regular vs FOREVER Deposits**: Tests deposit type differentiation
- **Currency Integration**: Tests Currency class integration
- **Deposit Index State Persistence**: Tests serialization
- **Block Reward Calculation**: Tests reward calculation integration
- **Large Scale Integration**: Tests with many deposits
- **Error Handling**: Tests edge cases and error conditions
- **State Validation**: Tests consistency across components
- **Performance Integration**: Tests performance with integration

### 3. **Performance Tests** (`TestDynamicSupplyPerformance.cpp`)
Tests performance characteristics of the dynamic supply system:

- **Single Operation Performance**: Tests individual operation speed
- **Batch Operations Performance**: Tests bulk operation efficiency
- **Random Amount Operations**: Tests with varying amounts
- **State Query Performance**: Tests read operation speed
- **Serialization Performance**: Tests persistence speed
- **Deserialization Performance**: Tests loading speed
- **Memory Usage**: Tests memory efficiency
- **Concurrent Access Simulation**: Tests thread safety
- **Large Amount Operations**: Tests with significant amounts
- **Stress Tests**: Tests under heavy load

### 4. **Simulation Tests** (`dynamic_supply_simulation.cpp`)
Long-running simulation tests:

- **6-Month Simulation**: Tests system stability over time
- **1 Million XFG Burn**: Tests with significant burn amounts
- **Economic Balance Verification**: Tests long-term balance
- **Block Reward Analysis**: Tests reward scaling over time
- **System Stability**: Tests for instabilities or errors

## Running Tests

### Prerequisites
```bash
# Install dependencies
sudo apt-get install build-essential cmake libboost-all-dev gtest

# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON ..
make -j$(nproc)
```

### Running Individual Tests
```bash
# Unit tests
./tests/dynamic_money_supply_test

# Integration tests
./tests/dynamic_supply_integration_test

# Performance tests
./tests/dynamic_supply_performance_test

# Simulation test
./test/dynamic_supply_simulation
```

### Running All Tests
```bash
# Use the test runner script
./run_dynamic_supply_tests.sh
```

### Running with CMake
```bash
# Run all tests
make tests

# Run specific test
make DynamicMoneySupplyTest
make DynamicSupplyIntegrationTest
make DynamicSupplyPerformanceTest
```

## Test Results Interpretation

### Expected Results

#### Unit Tests
- All 15 unit tests should pass
- No memory leaks or crashes
- All economic balance assertions should pass

#### Integration Tests
- All 10 integration tests should pass
- Deposit index and dynamic supply should stay in sync
- Currency integration should work correctly

#### Performance Tests
- Single operations: < 1 microsecond
- Batch operations: > 100,000 ops/second
- State queries: > 1,000,000 queries/second
- Serialization: > 10,000 serializations/second
- Memory usage: Constant (no dynamic allocations)

#### Simulation Tests
- 6-month simulation should complete without errors
- Economic balance should be maintained
- Block rewards should scale correctly
- No system instabilities should be detected

### Performance Benchmarks

| Test Type | Minimum Performance | Target Performance |
|-----------|-------------------|-------------------|
| Single Operation | < 1 μs | < 100 ns |
| Batch Operations | > 50,000 ops/s | > 200,000 ops/s |
| State Queries | > 500,000 queries/s | > 2,000,000 queries/s |
| Serialization | > 5,000 serializations/s | > 50,000 serializations/s |
| Memory Usage | Constant | < 1KB overhead |

## Test Coverage

### Code Coverage Areas
- ✅ DynamicMoneySupply class methods
- ✅ Currency class integration
- ✅ BankingIndex integration
- ✅ State serialization/deserialization
- ✅ Economic balance calculations
- ✅ Block reward scaling
- ✅ Error handling and edge cases
- ✅ Performance characteristics

### Test Scenarios Covered
- ✅ Normal operation scenarios
- ✅ Edge cases and error conditions
- ✅ Large-scale operations
- ✅ Long-running simulations
- ✅ Integration between components
- ✅ Performance under load
- ✅ Memory efficiency
- ✅ Thread safety (simulated)

## Continuous Integration

The test suite is integrated with GitHub Actions:

```yaml
# .github/workflows/test-dynamic-supply.yml
name: Test Dynamic Supply
on:
  push:
    branches: [ main, develop ]
    paths: 
      - 'src/CryptoNoteCore/DynamicMoneySupply.*'
      - 'src/CryptoNoteCore/BankingIndex.*'
      - 'src/CryptoNoteCore/Currency.*'
  pull_request:
    branches: [ main, develop ]
```

## Troubleshooting

### Common Issues

#### Build Errors
```bash
# Missing dependencies
sudo apt-get install libboost-all-dev gtest

# CMake configuration issues
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON ..
```

#### Test Failures
```bash
# Check test output for specific failures
./tests/dynamic_money_supply_test --gtest_output=xml:results.xml

# Run with verbose output
./tests/dynamic_money_supply_test --gtest_verbose
```

#### Performance Issues
```bash
# Check system resources
htop
free -h

# Run performance tests with profiling
perf record ./tests/dynamic_supply_performance_test
perf report
```

### Debug Information

#### Enable Debug Logging
```bash
# Set logging level
export LOGGING_LEVEL=3

# Run tests with debug output
./tests/dynamic_money_supply_test --gtest_verbose
```

#### Memory Debugging
```bash
# Use Valgrind for memory checking
valgrind --leak-check=full ./tests/dynamic_money_supply_test

# Use AddressSanitizer
export ASAN_OPTIONS=detect_leaks=1
./tests/dynamic_money_supply_test
```

## Contributing

When adding new tests:

1. **Follow naming conventions**: `TestDynamicSupply[Feature].cpp`
2. **Use descriptive test names**: `TEST_F(ClassName, DescriptiveTestName)`
3. **Include performance benchmarks**: For performance-critical code
4. **Add integration tests**: For new component interactions
5. **Update documentation**: Keep this file current

### Test Template
```cpp
TEST_F(DynamicMoneySupplyTest, NewFeatureTest) {
    // Arrange
    uint64_t testAmount = 1000000000ULL;
    
    // Act
    m_dynamicSupply.addBurnedXfg(testAmount);
    
    // Assert
    EXPECT_EQ(m_dynamicSupply.getTotalBurnedXfg(), testAmount);
    EXPECT_TRUE(m_dynamicSupply.validateState());
}
```

## Conclusion

The Dynamic Supply Test Suite provides comprehensive coverage of the dynamic supply system, ensuring:

- ✅ **Correctness**: All functionality works as expected
- ✅ **Performance**: System meets performance requirements
- ✅ **Reliability**: System is stable under various conditions
- ✅ **Integration**: Components work together correctly
- ✅ **Maintainability**: Tests catch regressions

For questions or issues with the test suite, please refer to the main project documentation or create an issue in the project repository.
