#include "EldernodeIndexManager.h"
#include "EldernodeIndexTypes.h"
#include "crypto/crypto.h"
#include <iostream>
#include <cassert>

using namespace CryptoNote;

void testBasicEldernode() {
    std::cout << "Testing Basic Eldernode..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Test 1: Add Basic Eldernode (no stake required)
    ENindexEntry basicEntry;
    Crypto::generate_keys(basicEntry.eldernodePublicKey, basicEntry.eldernodeSecretKey);
    basicEntry.feeAddress = "FUEGO123456789abcdef";
    basicEntry.stakeAmount = 0; // No stake for basic Eldernodes (--set-fee-address only)
    basicEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    basicEntry.isActive = true;
    basicEntry.tier = EldernodeTier::BASIC;
    
    bool added = manager.addEldernode(basicEntry);
    assert(added);
    std::cout << "✓ Added Basic Eldernode successfully (no stake)" << std::endl;
    
    // Test 2: Get Basic Eldernode
    auto retrieved = manager.getEldernode(basicEntry.eldernodePublicKey);
    assert(retrieved.has_value());
    assert(retrieved->tier == EldernodeTier::BASIC);
    assert(retrieved->stakeAmount == 0);
    std::cout << "✓ Retrieved Basic Eldernode successfully" << std::endl;
    
    // Test 3: Basic Eldernode statistics
    assert(manager.getTotalEldernodeCount() == 1);
    assert(manager.getActiveEldernodeCount() == 1);
    assert(manager.getElderfierNodeCount() == 0);
    std::cout << "✓ Basic Eldernode statistics correct" << std::endl;
}

void testElderfierServiceNode() {
    std::cout << "Testing Elderfier Service Node..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Test 1: Add Elderfier with Custom Name (exactly 8 letters, all caps)
    ENindexEntry elderfierEntry;
    Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
    elderfierEntry.feeAddress = "FUEGO987654321fedcba";
    elderfierEntry.stakeAmount = 800000000; // 800 XFG minimum for Elderfier
    elderfierEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierEntry.isActive = true;
    elderfierEntry.tier = EldernodeTier::ELDERFIER;
    elderfierEntry.serviceId = ElderfierServiceId::createCustomName("FUEGONOD", "FUEGO987654321fedcba"); // Fixed: 8 chars
    
    bool added = manager.addEldernode(elderfierEntry);
    assert(added);
    std::cout << "✓ Added Elderfier with custom name successfully (8 letters, all caps)" << std::endl;
    
    // Test 2: Add Elderfier with Hashed Address
    ENindexEntry elderfierHashed;
    Crypto::generate_keys(elderfierHashed.eldernodePublicKey, elderfierHashed.eldernodeSecretKey);
    elderfierHashed.feeAddress = "FUEGO555666777888999";
    elderfierHashed.stakeAmount = 1000000000; // 1000 XFG
    elderfierHashed.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierHashed.isActive = true;
    elderfierHashed.tier = EldernodeTier::ELDERFIER;
    elderfierHashed.serviceId = ElderfierServiceId::createHashedAddress("FUEGO555666777888999");
    
    bool addedHashed = manager.addEldernode(elderfierHashed);
    assert(addedHashed);
    std::cout << "✓ Added Elderfier with hashed address successfully" << std::endl;
    
    // Test 3: Add Elderfier with Standard Address
    ENindexEntry elderfierStandard;
    Crypto::generate_keys(elderfierStandard.eldernodePublicKey, elderfierStandard.eldernodeSecretKey);
    elderfierStandard.feeAddress = "FUEGO111222333444555";
    elderfierStandard.stakeAmount = 800000000; // 800 XFG minimum
    elderfierStandard.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierStandard.isActive = true;
    elderfierStandard.tier = EldernodeTier::ELDERFIER;
    elderfierStandard.serviceId = ElderfierServiceId::createStandardAddress("FUEGO111222333444555");
    
    bool addedStandard = manager.addEldernode(elderfierStandard);
    assert(addedStandard);
    std::cout << "✓ Added Elderfier with standard address successfully" << std::endl;
    
    // Test 4: Get Elderfier nodes
    auto elderfierNodes = manager.getElderfierNodes();
    assert(elderfierNodes.size() == 3);
    std::cout << "✓ Retrieved " << elderfierNodes.size() << " Elderfier nodes" << std::endl;
    
    // Test 5: Get by service ID
    auto byServiceId = manager.getEldernodeByServiceId(elderfierEntry.serviceId);
    assert(byServiceId.has_value());
    assert(byServiceId->eldernodePublicKey == elderfierEntry.eldernodePublicKey);
    std::cout << "✓ Retrieved Elderfier by service ID successfully" << std::endl;
    
    // Test 6: Verify linked addresses and hashed addresses
    assert(elderfierEntry.serviceId.linkedAddress == elderfierEntry.feeAddress);
    assert(elderfierHashed.serviceId.linkedAddress == elderfierHashed.feeAddress);
    assert(!elderfierEntry.serviceId.hashedAddress.empty());
    assert(!elderfierHashed.serviceId.hashedAddress.empty());
    assert(!elderfierStandard.serviceId.hashedAddress.empty());
    std::cout << "✓ Service IDs properly linked to wallet addresses and have hashed addresses" << std::endl;
    
    // Test 7: Elderfier statistics
    assert(manager.getTotalEldernodeCount() == 3);
    assert(manager.getActiveEldernodeCount() == 3);
    assert(manager.getElderfierNodeCount() == 3);
    std::cout << "✓ Elderfier statistics correct" << std::endl;
}

void testServiceIdValidation() {
    std::cout << "Testing Service ID Validation..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Test 1: Invalid custom name (wrong length)
    ENindexEntry invalidEntry;
    Crypto::generate_keys(invalidEntry.eldernodePublicKey, invalidEntry.eldernodeSecretKey);
    invalidEntry.feeAddress = "FUEGO123456789abcdef";
    invalidEntry.stakeAmount = 800000000;
    invalidEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    invalidEntry.isActive = true;
    invalidEntry.tier = EldernodeTier::ELDERFIER;
    invalidEntry.serviceId = ElderfierServiceId::createCustomName("SHORT", "FUEGO123456789abcdef"); // Wrong length
    
    bool added = manager.addEldernode(invalidEntry);
    assert(!added); // Should fail
    std::cout << "✓ Rejected invalid custom name (wrong length)" << std::endl;
    
    // Test 2: Invalid custom name (not all caps)
    ENindexEntry invalidCapsEntry;
    Crypto::generate_keys(invalidCapsEntry.eldernodePublicKey, invalidCapsEntry.eldernodeSecretKey);
    invalidCapsEntry.feeAddress = "FUEGO123456789abcdef";
    invalidCapsEntry.stakeAmount = 800000000;
    invalidCapsEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    invalidCapsEntry.isActive = true;
    invalidCapsEntry.tier = EldernodeTier::ELDERFIER;
    invalidCapsEntry.serviceId = ElderfierServiceId::createCustomName("fuegonod", "FUEGO123456789abcdef"); // Not all caps
    
    bool addedCaps = manager.addEldernode(invalidCapsEntry);
    assert(!addedCaps); // Should fail
    std::cout << "✓ Rejected invalid custom name (not all caps)" << std::endl;
    
    // Test 3: Reserved custom name
    ENindexEntry reservedEntry;
    Crypto::generate_keys(reservedEntry.eldernodePublicKey, reservedEntry.eldernodeSecretKey);
    reservedEntry.feeAddress = "FUEGO123456789abcdef";
    reservedEntry.stakeAmount = 800000000;
    reservedEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    reservedEntry.isActive = true;
    reservedEntry.tier = EldernodeTier::ELDERFIER;
    reservedEntry.serviceId = ElderfierServiceId::createCustomName("ADMIN", "FUEGO123456789abcdef"); // Reserved name
    
    bool addedReserved = manager.addEldernode(reservedEntry);
    assert(!addedReserved); // Should fail
    std::cout << "✓ Rejected reserved custom name" << std::endl;
    
    // Test 4: Valid custom name (exactly 8 letters, all caps)
    ENindexEntry validEntry;
    Crypto::generate_keys(validEntry.eldernodePublicKey, validEntry.eldernodeSecretKey);
    validEntry.feeAddress = "FUEGO123456789abcdef";
    validEntry.stakeAmount = 800000000;
    validEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    validEntry.isActive = true;
    validEntry.tier = EldernodeTier::ELDERFIER;
    validEntry.serviceId = ElderfierServiceId::createCustomName("MYNODE", "FUEGO123456789abcdef");
    
    bool addedValid = manager.addEldernode(validEntry);
    assert(addedValid); // Should succeed
    std::cout << "✓ Accepted valid custom name (8 letters, all caps)" << std::endl;
    
    // Test 5: Insufficient stake for Elderfier
    ENindexEntry lowStakeEntry;
    Crypto::generate_keys(lowStakeEntry.eldernodePublicKey, lowStakeEntry.eldernodeSecretKey);
    lowStakeEntry.feeAddress = "FUEGO999888777666555";
    lowStakeEntry.stakeAmount = 100000000; // Only 100 XFG (less than 800 XFG required)
    lowStakeEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    lowStakeEntry.isActive = true;
    lowStakeEntry.tier = EldernodeTier::ELDERFIER;
    lowStakeEntry.serviceId = ElderfierServiceId::createCustomName("LOWSTAKE", "FUEGO999888777666555");
    
    bool addedLowStake = manager.addEldernode(lowStakeEntry);
    assert(!addedLowStake); // Should fail
    std::cout << "✓ Rejected Elderfier with insufficient stake (less than 800 XFG)" << std::endl;
}

void testTierPrioritization() {
    std::cout << "Testing Tier Prioritization..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Add Basic Eldernode (no stake)
    ENindexEntry basicEntry;
    Crypto::generate_keys(basicEntry.eldernodePublicKey, basicEntry.eldernodeSecretKey);
    basicEntry.feeAddress = "FUEGO123456789abcdef";
    basicEntry.stakeAmount = 0; // No stake for basic
    basicEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    basicEntry.isActive = true;
    basicEntry.tier = EldernodeTier::BASIC;
    
    bool addedBasic = manager.addEldernode(basicEntry);
    assert(addedBasic);
    
    // Add Elderfier with minimum stake
    ENindexEntry elderfierEntry;
    Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
    elderfierEntry.feeAddress = "FUEGO987654321fedcba";
    elderfierEntry.stakeAmount = 800000000; // 800 XFG minimum
    elderfierEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierEntry.isActive = true;
    elderfierEntry.tier = EldernodeTier::ELDERFIER;
    elderfierEntry.serviceId = ElderfierServiceId::createCustomName("PRIORITY", "FUEGO987654321fedcba");
    
    bool addedElderfier = manager.addEldernode(elderfierEntry);
    assert(addedElderfier);
    
    // Add consensus participants
    EldernodeConsensusParticipant basicParticipant;
    basicParticipant.publicKey = basicEntry.eldernodePublicKey;
    basicParticipant.address = basicEntry.feeAddress;
    basicParticipant.stakeAmount = basicEntry.stakeAmount;
    basicParticipant.isActive = true;
    basicParticipant.tier = EldernodeTier::BASIC;
    
    EldernodeConsensusParticipant elderfierParticipant;
    elderfierParticipant.publicKey = elderfierEntry.eldernodePublicKey;
    elderfierParticipant.address = elderfierEntry.feeAddress;
    elderfierParticipant.stakeAmount = elderfierEntry.stakeAmount;
    elderfierParticipant.isActive = true;
    elderfierParticipant.tier = EldernodeTier::ELDERFIER;
    elderfierParticipant.serviceId = elderfierEntry.serviceId;
    
    manager.addConsensusParticipant(basicParticipant);
    manager.addConsensusParticipant(elderfierParticipant);
    
    // Test consensus prioritization
    std::vector<uint8_t> testData = {1, 2, 3, 4, 5};
    ConsensusThresholds thresholds;
    thresholds.minimumEldernodes = 2;
    thresholds.requiredAgreement = 2;
    thresholds.timeoutSeconds = 30;
    thresholds.retryAttempts = 3;
    
    auto result = manager.reachConsensus(testData, thresholds);
    assert(result.consensusReached);
    assert(result.participatingEldernodes.size() == 2);
    
    // Elderfier should be prioritized (first in list)
    assert(result.participatingEldernodes[0] == elderfierParticipant.publicKey);
    std::cout << "✓ Elderfier nodes prioritized in consensus" << std::endl;
}

void testSlashingFunctionality() {
    std::cout << "Testing Slashing Functionality..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Add Elderfier node for slashing test
    ENindexEntry elderfierEntry;
    Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
    elderfierEntry.feeAddress = "FUEGO987654321fedcba";
    elderfierEntry.stakeAmount = 1000000000; // 1000 XFG
    elderfierEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierEntry.isActive = true;
    elderfierEntry.tier = EldernodeTier::ELDERFIER;
    elderfierEntry.serviceId = ElderfierServiceId::createCustomName("SLASHNODE", "FUEGO987654321fedcba");
    
    bool added = manager.addEldernode(elderfierEntry);
    assert(added);
    
    // Test 1: Slash Elderfier node
    uint64_t originalStake = elderfierEntry.stakeAmount;
    bool slashed = manager.slashEldernode(elderfierEntry.eldernodePublicKey, "Test slashing");
    assert(slashed);
    
    // Verify stake was reduced
    auto updatedEntry = manager.getEldernode(elderfierEntry.eldernodePublicKey);
    assert(updatedEntry.has_value());
    assert(updatedEntry->stakeAmount < originalStake);
    std::cout << "✓ Successfully slashed Elderfier node" << std::endl;
    
    // Test 2: Try to slash Basic Eldernode (should fail)
    ENindexEntry basicEntry;
    Crypto::generate_keys(basicEntry.eldernodePublicKey, basicEntry.eldernodeSecretKey);
    basicEntry.feeAddress = "FUEGO123456789abcdef";
    basicEntry.stakeAmount = 0;
    basicEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    basicEntry.isActive = true;
    basicEntry.tier = EldernodeTier::BASIC;
    
    bool addedBasic = manager.addEldernode(basicEntry);
    assert(addedBasic);
    
    bool slashBasic = manager.slashEldernode(basicEntry.eldernodePublicKey, "Test slashing basic");
    assert(!slashBasic); // Should fail
    std::cout << "✓ Correctly rejected slashing Basic Eldernode" << std::endl;
    
    // Test 3: Try to slash non-existent Eldernode
    Crypto::PublicKey fakeKey;
    Crypto::generate_keys(fakeKey, fakeKey); // Generate random key
    bool slashFake = manager.slashEldernode(fakeKey, "Test slashing fake");
    assert(!slashFake); // Should fail
    std::cout << "✓ Correctly rejected slashing non-existent Eldernode" << std::endl;
}

void testEldernodeIndexManager() {
    std::cout << "Running comprehensive Eldernode Index Manager tests..." << std::endl;
    
    testBasicEldernode();
    testElderfierServiceNode();
    testServiceIdValidation();
    testTierPrioritization();
    testSlashingFunctionality();
    
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
