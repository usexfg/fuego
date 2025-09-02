#include "EldernodeIndexManager.h"
#include "EldernodeIndexTypes.h"
#include "crypto/crypto.h"
#include <iostream>
#include <cassert>

using namespace CryptoNote;

void testEldernodeIndexManager() {
    std::cout << "Testing EldernodeIndexManager..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Test 1: Add Eldernode
    ENindexEntry entry;
    Crypto::generate_keys(entry.eldernodePublicKey, entry.eldernodeSecretKey);
    entry.feeAddress = "FUEGO123456789abcdef";
    entry.stakeAmount = 1000000; // 1 FUEGO
    entry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.isActive = true;
    
    bool added = manager.addEldernode(entry);
    assert(added);
    std::cout << "✓ Added Eldernode successfully" << std::endl;
    
    // Test 2: Get Eldernode
    auto retrieved = manager.getEldernode(entry.eldernodePublicKey);
    assert(retrieved.has_value());
    assert(retrieved->eldernodePublicKey == entry.eldernodePublicKey);
    std::cout << "✓ Retrieved Eldernode successfully" << std::endl;
    
    // Test 3: Add stake proof
    EldernodeStakeProof proof;
    proof.eldernodePublicKey = entry.eldernodePublicKey;
    proof.stakeAmount = entry.stakeAmount;
    proof.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    proof.feeAddress = entry.feeAddress;
    proof.stakeHash = Crypto::Hash(); // Will be calculated
    proof.proofSignature.resize(64, 0); // Placeholder signature
    
    bool proofAdded = manager.addStakeProof(proof);
    assert(proofAdded);
    std::cout << "✓ Added stake proof successfully" << std::endl;
    
    // Test 4: Get active Eldernodes
    auto activeEldernodes = manager.getActiveEldernodes();
    assert(activeEldernodes.size() == 1);
    assert(activeEldernodes[0].eldernodePublicKey == entry.eldernodePublicKey);
    std::cout << "✓ Retrieved active Eldernodes successfully" << std::endl;
    
    // Test 5: Consensus thresholds
    auto thresholds = manager.getConsensusThresholds();
    assert(thresholds.requiredAgreement == 4); // Should be 4/5
    assert(thresholds.minimumEldernodes == 5);
    std::cout << "✓ Consensus thresholds configured correctly (4/5)" << std::endl;
    
    // Test 6: Generate fresh proof
    bool freshProof = manager.generateFreshProof(entry.eldernodePublicKey, entry.feeAddress);
    assert(freshProof);
    std::cout << "✓ Generated fresh proof successfully" << std::endl;
    
    // Test 7: Statistics
    assert(manager.getTotalEldernodeCount() == 1);
    assert(manager.getActiveEldernodeCount() == 1);
    assert(manager.getTotalStakeAmount() == entry.stakeAmount);
    std::cout << "✓ Statistics calculated correctly" << std::endl;
    
    std::cout << "All tests passed! ✓" << std::endl;
}

int main() {
    try {
        testEldernodeIndexManager();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
