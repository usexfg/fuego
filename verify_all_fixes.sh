#!/bin/bash

# Comprehensive verification script for all build fixes
# This script verifies that all issues have been addressed across the codebase

set -e

echo "=========================================="
echo "Fuego Build Verification Script"
echo "=========================================="
echo ""
echo "This script verifies all fixes for:"
echo "  1. Daemon.cpp compilation errors"
echo "  2. Windows Boost installation"
echo "  3. Workflow configuration"
echo ""

ERRORS=0
WARNINGS=0

# ==========================================
# Part 1: Daemon.cpp Fixes
# ==========================================
echo "=========================================="
echo "PART 1: Verifying Daemon.cpp Fixes"
echo "=========================================="
echo ""

DAEMON_FILE="src/Daemon/Daemon.cpp"

if [ ! -f "$DAEMON_FILE" ]; then
    echo "✗ FAIL: Daemon.cpp not found at $DAEMON_FILE"
    ERRORS=$((ERRORS + 1))
else
    echo "[CHECK 1.1] TransactionExtra.h include..."
    if grep -q '#include "CryptoNoteCore/TransactionExtra.h"' "$DAEMON_FILE"; then
        echo "✓ PASS"
    else
        echo "✗ FAIL: Missing include"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 1.2] Non-const core parameter..."
    if grep -A2 "bool verifyStakeWithElderfierDeposit" "$DAEMON_FILE" | grep -q "CryptoNote::core& ccore"; then
        echo "✓ PASS"
    else
        echo "✗ FAIL: Still using const core&"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 1.3] getTransaction signature..."
    if grep -q "ccore.getTransaction(txHash, tx)" "$DAEMON_FILE" && ! grep -q "ccore.getTransaction(txHash, tx, blockHash, blockHeight)" "$DAEMON_FILE"; then
        echo "✓ PASS"
    else
        echo "✗ FAIL: Incorrect signature"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 1.4] depositAmountAtHeight access..."
    if grep -q "ccore.depositAmountAtHeight" "$DAEMON_FILE" && ! grep -q "ccore.getBlockchain().depositAmountAtHeight" "$DAEMON_FILE"; then
        echo "✓ PASS"
    else
        echo "✗ FAIL: Still using private getBlockchain()"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 1.5] Printf statement..."
    if grep -q 'printf("Fuego %s\\n", PROJECT_VERSION_LONG)' "$DAEMON_FILE"; then
        echo "✓ PASS"
    elif grep -q 'fuego_icon' "$DAEMON_FILE" || grep -q 'printf.*>>' "$DAEMON_FILE"; then
        echo "✗ FAIL: Invalid printf syntax"
        ERRORS=$((ERRORS + 1))
    else
        echo "⚠ WARNING: Printf not found (may be OK)"
        WARNINGS=$((WARNINGS + 1))
    fi

    echo "[CHECK 1.6] Balanced braces..."
    OPEN_BRACES=$(grep -o '{' "$DAEMON_FILE" | wc -l | tr -d ' ')
    CLOSE_BRACES=$(grep -o '}' "$DAEMON_FILE" | wc -l | tr -d ' ')
    if [ "$OPEN_BRACES" -eq "$CLOSE_BRACES" ]; then
        echo "✓ PASS: $OPEN_BRACES opening = $CLOSE_BRACES closing"
    else
        echo "✗ FAIL: $OPEN_BRACES opening ≠ $CLOSE_BRACES closing"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 1.7] No duplicate namespaces..."
    DUPLICATE_NS=$(grep -n "^ namespace$" "$DAEMON_FILE" | wc -l | tr -d ' ')
    if [ "$DUPLICATE_NS" -eq 0 ]; then
        echo "✓ PASS"
    else
        echo "✗ FAIL: Found $DUPLICATE_NS duplicate namespace(s)"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 1.8] Namespace closure comment..."
    if grep -q "} // anonymous namespace" "$DAEMON_FILE"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: Missing comment"
        WARNINGS=$((WARNINGS + 1))
    fi
fi

echo ""

# ==========================================
# Part 2: Windows Workflow Fixes
# ==========================================
echo "=========================================="
echo "PART 2: Verifying Windows Workflow Fixes"
echo "=========================================="
echo ""

CHECK_YML=".github/workflows/check.yml"

if [ ! -f "$CHECK_YML" ]; then
    echo "✗ FAIL: check.yml not found"
    ERRORS=$((ERRORS + 1))
else
    echo "[CHECK 2.1] No Start-Process on ZIP file..."
    if grep -q "Start-Process.*OutputPath" "$CHECK_YML"; then
        echo "✗ FAIL: Still trying to execute ZIP file"
        ERRORS=$((ERRORS + 1))
    else
        echo "✓ PASS"
    fi

    echo "[CHECK 2.2] Using Chocolatey for Boost..."
    if grep -q "choco install.*boost-msvc-14.3" "$CHECK_YML"; then
        echo "✓ PASS"
    else
        echo "✗ FAIL: Not using Chocolatey"
        ERRORS=$((ERRORS + 1))
    fi

    echo "[CHECK 2.3] Correct BOOST_ROOT path..."
    if grep -q "BOOST_ROOT.*C:\\\\local\\\\boost" "$CHECK_YML" || grep -q 'BOOST_ROOT.*C:\local\boost' "$CHECK_YML"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: BOOST_ROOT may not be set to standard path"
        WARNINGS=$((WARNINGS + 1))
    fi

    echo "[CHECK 2.4] Boost verification step exists..."
    if grep -q "Verify Boost Installation" "$CHECK_YML"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: No verification step found"
        WARNINGS=$((WARNINGS + 1))
    fi
fi

echo ""

# ==========================================
# Part 3: Wallet Desktop Workflow
# ==========================================
echo "=========================================="
echo "PART 3: Verifying Wallet Desktop Workflow"
echo "=========================================="
echo ""

WALLET_YML=".github/workflows/wallet-desktop.yml"

if [ ! -f "$WALLET_YML" ]; then
    echo "✗ FAIL: wallet-desktop.yml not found"
    ERRORS=$((ERRORS + 1))
else
    echo "[CHECK 3.1] Windows uses Chocolatey..."
    if grep -q "choco install.*boost-msvc-14.3" "$WALLET_YML"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: May not be using Chocolatey"
        WARNINGS=$((WARNINGS + 1))
    fi

    echo "[CHECK 3.2] Ubuntu has required deps..."
    if grep -q "libboost-all-dev" "$WALLET_YML"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: May be missing Boost"
        WARNINGS=$((WARNINGS + 1))
    fi

    echo "[CHECK 3.3] macOS uses Homebrew..."
    if grep -q "brew install.*boost" "$WALLET_YML"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: May not be using Homebrew"
        WARNINGS=$((WARNINGS + 1))
    fi
fi

echo ""

# ==========================================
# Part 4: CMake Configuration
# ==========================================
echo "=========================================="
echo "PART 4: Verifying CMake Configuration"
echo "=========================================="
echo ""

CMAKE_FILE="CMakeLists.txt"

if [ ! -f "$CMAKE_FILE" ]; then
    echo "✗ FAIL: CMakeLists.txt not found"
    ERRORS=$((ERRORS + 1))
else
    echo "[CHECK 4.1] Boost is required..."
    if grep -qi "find_package.*Boost.*REQUIRED" "$CMAKE_FILE"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: Boost may not be required"
        WARNINGS=$((WARNINGS + 1))
    fi

    echo "[CHECK 4.2] Project uses C++11 or higher..."
    if grep -qi "CMAKE_CXX_STANDARD" "$CMAKE_FILE" || grep -qi "std=c++11" "$CMAKE_FILE"; then
        echo "✓ PASS"
    else
        echo "⚠ WARNING: C++ standard not explicitly set"
        WARNINGS=$((WARNINGS + 1))
    fi
fi

echo ""

# ==========================================
# Part 5: Git Status
# ==========================================
echo "=========================================="
echo "PART 5: Git Repository Status"
echo "=========================================="
echo ""

if command -v git &> /dev/null && [ -d ".git" ]; then
    echo "[CHECK 5.1] Current branch..."
    CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    echo "  Branch: $CURRENT_BRANCH"

    echo "[CHECK 5.2] Uncommitted changes..."
    if git diff-index --quiet HEAD --; then
        echo "  ✓ Working tree clean"
    else
        echo "  ⚠ WARNING: Uncommitted changes present"
        WARNINGS=$((WARNINGS + 1))
        git status --short
    fi

    echo "[CHECK 5.3] Last commit..."
    git log -1 --format="  %h - %s (%ar)" 2>/dev/null || echo "  Unable to get commit info"
else
    echo "⚠ WARNING: Not a git repository or git not available"
    WARNINGS=$((WARNINGS + 1))
fi

echo ""

# ==========================================
# Summary
# ==========================================
echo "=========================================="
echo "VERIFICATION SUMMARY"
echo "=========================================="
echo ""

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo "🎉 ALL CHECKS PASSED!"
    echo ""
    echo "The codebase is ready for CI/CD builds."
    echo "All known issues have been resolved."
    echo ""
    echo "Next steps:"
    echo "  1. Commit any pending changes"
    echo "  2. Push to trigger CI/CD pipeline"
    echo "  3. Monitor build results"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo "✓ All critical checks passed"
    echo "⚠ $WARNINGS warning(s) found"
    echo ""
    echo "The build should succeed, but review warnings above."
    exit 0
else
    echo "✗ $ERRORS error(s) found"
    echo "⚠ $WARNINGS warning(s) found"
    echo ""
    echo "Critical issues detected. Please fix the errors above."
    exit 1
fi
