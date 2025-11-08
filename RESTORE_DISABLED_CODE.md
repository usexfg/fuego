# Restore Disabled Code - M1 Debug Cleanup

## Overview
This document catalogs ALL code that was disabled during M1/Apple Silicon debugging and needs to be restored for proper functionality.

---

## Critical Issues Found

### 1. ❌ PHMAP Serialization DISABLED (Blockchain.cpp)
**Status**: BROKEN - Transaction map and spent keys not being saved/loaded!
**Impact**: CRITICAL - Blockchain state cannot be properly persisted

### 2. ❌ ICU Linking DISABLED (CMakeLists.txt)
**Status**: BROKEN - ICU libraries not linked on macOS
**Impact**: HIGH - Text processing/locale issues

### 3. ⚠️ Elderfier Service DISABLED (Daemon.cpp)
**Status**: Intentionally disabled for testing
**Impact**: MEDIUM - Feature unavailable

---

## 1. PHMAP Serialization (CRITICAL)

### Location
**File**: `src/CryptoNoteCore/Blockchain.cpp`
**Lines**: 203-226

### Problem
Transaction map and spent keys serialization is completely disabled:

```cpp
printf("INFO: %stransaction map\n", operation.c_str());
// Temporarily disabled phmap serialization - using std::unordered_map
// if (s.type() == ISerializer::INPUT)
// {
//   phmap::BinaryInputArchive ar_in(appendPath(m_bs.m_config_folder, "transactionsmap.dat").c_str());
//   m_bs.m_transactionMap.load(ar_in);
// }
// else
// {
//   phmap::BinaryOutputArchive ar_out(appendPath(m_bs.m_config_folder, "transactionsmap.dat").c_str());
//   m_bs.m_transactionMap.dump(ar_out);
// }

printf("INFO: %sspent keys\n", operation.c_str());
// Temporarily disabled phmap serialization - using std::unordered_map
// if (s.type() == ISerializer::INPUT)
// {
//   phmap::BinaryInputArchive ar_in(appendPath(m_bs.m_config_folder, "spentkeys.dat").c_str());
//   m_bs.m_spent_keys.load(ar_in);
// }
// else
// {
//   phmap::BinaryOutputArchive ar_out(appendPath(m_bs.m_config_folder, "spentkeys.dat").c_str());
//   m_bs.m_spent_keys.dump(ar_out);
// }
```

### Why This is BROKEN

The comment says "using std::unordered_map" but the actual type is still `parallel_flat_hash_map`:

```cpp
// In Blockchain.h:
typedef parallel_flat_hash_map<Crypto::Hash, TransactionIndex> TransactionMap;
typedef parallel_flat_hash_map<Crypto::KeyImage, uint32_t> SpentKeysContainer;
```

**Result**: These maps are NOT being serialized AT ALL, causing:
- Transaction lookups to fail after restart
- Spent key tracking to be lost
- Blockchain rollback to crash (unordered_map::at error)
- Checkpoint validation failures

### Fix Required

**Option A**: Re-enable phmap serialization (RECOMMENDED)

```cpp
printf("INFO: %stransaction map\n", operation.c_str());
if (s.type() == ISerializer::INPUT)
{
  phmap::BinaryInputArchive ar_in(appendPath(m_bs.m_config_folder, "transactionsmap.dat").c_str());
  m_bs.m_transactionMap.load(ar_in);
}
else
{
  phmap::BinaryOutputArchive ar_out(appendPath(m_bs.m_config_folder, "transactionsmap.dat").c_str());
  m_bs.m_transactionMap.dump(ar_out);
}

printf("INFO: %sspent keys\n", operation.c_str());
if (s.type() == ISerializer::INPUT)
{
  phmap::BinaryInputArchive ar_in(appendPath(m_bs.m_config_folder, "spentkeys.dat").c_str());
  m_bs.m_spent_keys.load(ar_in);
}
else
{
  phmap::BinaryOutputArchive ar_out(appendPath(m_bs.m_config_folder, "spentkeys.dat").c_str());
  m_bs.m_spent_keys.dump(ar_out);
}
```

**Option B**: Convert to std::unordered_map (NOT recommended - performance loss)

Change in `Blockchain.h`:
```cpp
typedef std::unordered_map<Crypto::Hash, TransactionIndex> TransactionMap;
typedef std::unordered_map<Crypto::KeyImage, uint32_t> SpentKeysContainer;
```

Then use standard serialization (slower, but would work).

---

## 2. ICU Linking (macOS)

### Location
**File**: `CMakeLists.txt`
**Lines**: 161-172

### Problem

```cmake
# --------------------------
# ICU linking for executables (macOS) - TEMPORARILY DISABLED FOR M1 DEBUG
# --------------------------
if(APPLE AND ICU_FOUND)
    message(STATUS "ICU linking temporarily disabled for M1 debugging")
    # foreach(exec_target Daemon SimpleWallet PaymentGateService Optimizer)
    #     if(TARGET ${exec_target})
    #         message(STATUS "Linking ICU to ${exec_target}")
    #         target_link_libraries(${exec_target} PRIVATE ${ICU_LIBRARIES})
    #     endif()
    # endforeach()
endif()
```

### Why This is BROKEN

ICU (International Components for Unicode) provides:
- Locale support
- Text processing
- Character encoding
- Number formatting

Without ICU linked, macOS builds may have:
- Locale detection failures
- Text encoding issues
- Banner/UTF-8 display problems

### Fix Required

```cmake
# --------------------------
# ICU linking for executables (macOS)
# --------------------------
if(APPLE AND ICU_FOUND)
    message(STATUS "Linking ICU libraries to executables")
    foreach(exec_target Daemon SimpleWallet PaymentGateService Optimizer)
        if(TARGET ${exec_target})
            message(STATUS "Linking ICU to ${exec_target}")
            target_link_libraries(${exec_target} PRIVATE ${ICU_LIBRARIES})
        endif()
    endforeach()
endif()
```

---

## 3. Elderfier Service (Feature Flag)

### Location
**File**: `src/Daemon/Daemon.cpp`
**Lines**: 459-468

### Problem

```cpp
if (command_line::has_arg(vm, arg_enable_elderfier)) {
  printf("INFO: Elderfier service is available but temporarily disabled for testing\n");
  printf("INFO: Elderfier service requires proper blockchain initialization to verify stakes\n");
  printf("INFO: To enable: --enable-elderfier --set-fee-address YOUR_ADDRESS\n");

  // TODO: Re-enable Elderfier service after core stabilization
  // The stake verification code below may cause segfaults during initialization
  // For now, disable Elderfier to allow daemon to start properly
}
```

### Why This is Disabled

The Elderfier service was causing segfaults during daemon initialization, likely due to:
1. Blockchain not fully loaded when stake verification runs
2. Transaction map being empty (see Issue #1 above)
3. Race condition between core init and service start

### Fix Required

After fixing the phmap serialization (Issue #1), re-enable:

```cpp
if (command_line::has_arg(vm, arg_enable_elderfier)) {
  std::string feeAddress = command_line::get_arg(vm, arg_set_fee_address);
  
  if (feeAddress.empty()) {
    logger(ERROR) << "Elderfier service requires --set-fee-address";
    return 1;
  }

  // Verify the fee address has 800 XFG stake via Elderfier deposit
  if (verifyStakeWithElderfierDeposit(feeAddress, ccore, currency)) {
    logger(INFO) << "Elderfier service enabled with valid stake at: " << feeAddress;
    // Enable STARK proof verification
    // Initialize Elderfier registry from GitHub
    // Start Elderfier service endpoint
  } else {
    logger(ERROR) << "Elderfier service requires 800 XFG stake in Elderfier deposit";
    return 1;
  }
}
```

---

## 4. Banner/Version Display

### Location
**File**: `src/Daemon/Daemon.cpp`
**Line**: 337

### Current Code

```cpp
printf("Fuego %s\n", PROJECT_VERSION_LONG);
```

### Expected Banner

The banner should display:
```
🔥 Fuego v10.X.X
```

### Fix Required

```cpp
// Display startup banner
const char* fuego_icon = isUtf8Supported() ? "🔥 " : "";
printf("%sFuego %s\n", fuego_icon, PROJECT_VERSION_LONG);
printf("Cryptocurrency for the Elderfire generation\n");
printf("Loading blockchain...\n");
```

---

## 5. Signal Handler

### Location
**File**: `src/Daemon/Daemon.cpp`
**Lines**: 472-473

### Current Code

```cpp
// Temporarily UN-disable signal handler for testing
Tools::SignalHandler::install([&dch, &p2psrv] {
```

### Comment Cleanup

This comment is confusing. Should be:

```cpp
// Install signal handler for graceful shutdown
Tools::SignalHandler::install([&dch, &p2psrv] {
```

---

## Priority Order for Fixes

### Immediate (CRITICAL)
1. **Re-enable phmap serialization** - Blockchain functionality broken without this
2. **Fix transaction map persistence** - Required for rollback, checkpoints, queries

### High Priority
3. **Re-enable ICU linking** - macOS builds missing locale support
4. **Verify checkpoint functionality** - Ensure checkpoints work after phmap fix

### Medium Priority
5. **Re-enable Elderfier service** - After blockchain is stable
6. **Improve startup banner** - Better user experience
7. **Clean up comments** - Remove "temporary" notes

---

## Testing After Fixes

### Test 1: Blockchain Persistence
```bash
# Start daemon
./fuegoX --data-dir test-data

# Let it sync some blocks
# Stop daemon (Ctrl+C)

# Restart daemon
./fuegoX --data-dir test-data

# Check that transaction map loaded
# Should see: "INFO: loading transaction map"
# Should NOT resync from genesis
```

### Test 2: Checkpoint Validation
```bash
# Start daemon with existing blockchain
./fuegoX

# Should NOT see "Invalid checkpoint found"
# Should NOT see "Rollback blockchain"
# Should load to current height without errors
```

### Test 3: Transaction Queries
```bash
# After restart, query a transaction
./fuego-wallet-cli
> get_tx <tx_hash>

# Should return transaction details
# Should NOT crash with "unordered_map::at"
```

### Test 4: macOS ICU
```bash
# On macOS
./fuegoX --help

# Should display 🔥 emoji correctly
# Should handle UTF-8 in logs properly
```

---

## Files to Modify

1. `src/CryptoNoteCore/Blockchain.cpp`
   - Lines 203-226: Re-enable phmap serialization

2. `CMakeLists.txt`
   - Lines 161-172: Re-enable ICU linking

3. `src/Daemon/Daemon.cpp`
   - Lines 337-338: Enhance banner
   - Lines 459-468: Re-enable Elderfier (after phmap fix)
   - Line 472: Clean up comment

---

## Root Cause Analysis

### Why Was Code Disabled?

Someone debugging M1/Apple Silicon compatibility issues:

1. **Saw crashes** → Disabled phmap thinking it was M1-specific
2. **Saw ICU errors** → Disabled ICU linking
3. **Saw segfaults** → Disabled Elderfier service
4. **Never re-enabled** → Left in broken state

### The Real Problem

The actual issue was likely:
- Race conditions during initialization
- Missing null checks
- Improper shutdown handling

Disabling serialization **made things worse**, not better.

---

## Verification Commands

```bash
# Check if phmap serialization is enabled
grep -A5 "transaction map" src/CryptoNoteCore/Blockchain.cpp | grep -v "^[[:space:]]*//"

# Check if ICU is linked
grep -A5 "ICU linking" CMakeLists.txt | grep -v "^[[:space:]]*#"

# Check blockchain loads properly
./fuegoX 2>&1 | grep -i "loading\|loaded"

# Verify no crashes
./fuegoX && echo "Startup successful"
```

---

## Commit Message Template

```
Restore disabled M1 debug code - fix blockchain persistence

Critical fixes:
1. Re-enable phmap serialization for transaction map and spent keys
   - Was completely disabled, causing blockchain state loss on restart
   - Caused checkpoint failures and rollback crashes
   
2. Re-enable ICU linking for macOS executables
   - Fixes locale support and UTF-8 handling
   
3. Clean up temporary debug comments
   - Remove "temporarily disabled" notes
   - Improve code clarity

Impact: Blockchain now properly persists state across restarts.
No more checkpoint validation failures or transaction lookup crashes.

Resolves: #XXX (blockchain persistence issues)
Resolves: #YYY (macOS ICU errors)
```

---

## WARNING

**DO NOT DISABLE SERIALIZATION WITHOUT A BACKUP PLAN**

If there are M1 compatibility issues with phmap:
1. Fix the actual issue (null checks, initialization order)
2. Add fallback to std::unordered_map if needed
3. Keep serialization working!

Disabling serialization = broken blockchain database.

---

**Status**: 🔴 CRITICAL - Fix immediately
**Priority**: P0
**Effort**: 30 minutes to restore code, 1 hour to test
**Risk**: LOW - Just un-commenting working code
**Impact**: HIGH - Fixes multiple critical bugs

---

*Document created: 2025-11-08*
*Last updated: 2025-11-08*
*Author: Code Review / Recovery*