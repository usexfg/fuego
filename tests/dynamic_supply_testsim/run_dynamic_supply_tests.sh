#!/bin/bash

# Dynamic Supply Test Runner
# This script runs all dynamic supply tests and provides a comprehensive report

set -e

echo "🔥 Fuego Dynamic Supply Test Suite 🔥"
echo "====================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test and track results
run_test() {
    local test_name="$1"
    local test_executable="$2"
    
    echo -e "${BLUE}Running $test_name...${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ -f "$test_executable" ]; then
        if ./"$test_executable" --gtest_output=xml:"${test_name}_results.xml"; then
            echo -e "${GREEN}✅ $test_name PASSED${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "${RED}❌ $test_name FAILED${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo -e "${RED}❌ $test_name - Executable not found: $test_executable${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo ""
}

# Check if we're in the build directory
if [ ! -f "CMakeCache.txt" ]; then
    echo -e "${YELLOW}Warning: Not in build directory. Looking for build directory...${NC}"
    if [ -d "../build" ]; then
        cd ../build
        echo -e "${GREEN}Changed to build directory${NC}"
    elif [ -d "build" ]; then
        cd build
        echo -e "${GREEN}Changed to build directory${NC}"
    else
        echo -e "${RED}Error: Build directory not found. Please run this script from the build directory.${NC}"
        exit 1
    fi
fi

echo -e "${BLUE}Build directory: $(pwd)${NC}"
echo ""

# Check if tests are built
echo -e "${BLUE}Checking for test executables...${NC}"
if [ ! -f "tests/dynamic_money_supply_test" ]; then
    echo -e "${YELLOW}Building dynamic supply tests...${NC}"
    make DynamicMoneySupplyTest DynamicSupplyIntegrationTest DynamicSupplyPerformanceTest -j$(nproc)
fi

echo ""

# Run the simulation test first
echo -e "${BLUE}Running Dynamic Supply Simulation...${NC}"
if [ -f "test/dynamic_supply_simulation" ]; then
    echo -e "${BLUE}Running 6-month simulation with 1M XFG burned...${NC}"
    if ./test/dynamic_supply_simulation > simulation_results.txt 2>&1; then
        echo -e "${GREEN}✅ Dynamic Supply Simulation PASSED${NC}"
        echo -e "${BLUE}Simulation results saved to simulation_results.txt${NC}"
        
        # Extract key results
        echo -e "${BLUE}Key Results:${NC}"
        grep -E "(Initial Base Supply|Final Base Supply|Total Burned|Block Reward)" simulation_results.txt | head -10
    else
        echo -e "${RED}❌ Dynamic Supply Simulation FAILED${NC}"
    fi
else
    echo -e "${YELLOW}Simulation test not found, skipping...${NC}"
fi

echo ""

# Run unit tests
echo -e "${BLUE}Running Unit Tests...${NC}"
run_test "Dynamic Money Supply Unit Tests" "tests/dynamic_money_supply_test"
run_test "Dynamic Supply Integration Tests" "tests/dynamic_supply_integration_test"
run_test "Dynamic Supply Performance Tests" "tests/dynamic_supply_performance_test"

# Run additional tests if available
if [ -f "tests/core_tests" ]; then
    echo -e "${BLUE}Running Core Tests (includes dynamic supply components)...${NC}"
    run_test "Core Tests" "tests/core_tests"
fi

echo ""

# Generate test report
echo -e "${BLUE}📊 TEST REPORT 📊${NC}"
echo "=================="
echo -e "Total Tests: ${TOTAL_TESTS}"
echo -e "Passed: ${GREEN}${PASSED_TESTS}${NC}"
echo -e "Failed: ${RED}${FAILED_TESTS}${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED! 🎉${NC}"
    echo -e "${GREEN}Dynamic Supply System is working correctly!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests failed. Please check the output above.${NC}"
    exit 1
fi
