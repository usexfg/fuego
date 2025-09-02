#include "EldernodeIndexManager.h"
#include "EldernodeIndexTypes.h"
#include "crypto/crypto.h"
#include <iostream>
#include <cassert>

using namespace CryptoNote;

void testBasicEldernode() {
    std::cout << "Testing Basic Eldernode..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Test 1: Add Basic Eldernode
    ENindexEntry basicEntry;
    Crypto::generate_keys(basicEntry.eldernodePublicKey, basicEntry.eldernodeSecretKey);
    basicEntry.feeAddress = "FUEGO123456789abcdef";
    basicEntry.stakeAmount = 1000000; // 1 FUEGO minimum for basic
    basicEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    basicEntry.isActive = true;
    basicEntry.tier = EldernodeTier::BASIC;
    
    bool added = manager.addEldernode(basicEntry);
    assert(added);
    std::cout << "✓ Added Basic Eldernode successfully" << std::endl;
    
    // Test 2: Get Basic Eldernode
    auto retrieved = manager.getEldernode(basicEntry.eldernodePublicKey);
    assert(retrieved.has_value());
    assert(retrieved->tier == EldernodeTier::BASIC);
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
    
    // Test 1: Add Elderfier with Custom Name
    ENindexEntry elderfierEntry;
    Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
    elderfierEntry.feeAddress = "FUEGO987654321fedcba";
    elderfierEntry.stakeAmount = 5000000; // 5 FUEGO minimum for Elderfier
    elderfierEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierEntry.isActive = true;
    elderfierEntry.tier = EldernodeTier::ELDERFIER;
    elderfierEntry.serviceId = ElderfierServiceId::createCustomName("FuegoNode");
    
    bool added = manager.addEldernode(elderfierEntry);
    assert(added);
    std::cout << "✓ Added Elderfier with custom name successfully" << std::endl;
    
    // Test 2: Add Elderfier with Hashed Address
    ENindexEntry elderfierHashed;
    Crypto::generate_keys(elderfierHashed.eldernodePublicKey, elderfierHashed.eldernodeSecretKey);
    elderfierHashed.feeAddress = "FUEGO555666777888999";
    elderfierHashed.stakeAmount = 10000000; // 10 FUEGO
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
    elderfierStandard.stakeAmount = 7500000; // 7.5 FUEGO
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
    
    // Test 6: Elderfier statistics
    assert(manager.getTotalEldernodeCount() == 3);
    assert(manager.getActiveEldernodeCount() == 3);
    assert(manager.getElderfierNodeCount() == 3);
    std::cout << "✓ Elderfier statistics correct" << std::endl;
}

void testServiceIdValidation() {
    std::cout << "Testing Service ID Validation..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Test 1: Invalid custom name (too long)
    ENindexEntry invalidEntry;
    Crypto::generate_keys(invalidEntry.eldernodePublicKey, invalidEntry.eldernodeSecretKey);
    invalidEntry.feeAddress = "FUEGO123456789abcdef";
    invalidEntry.stakeAmount = 5000000;
    invalidEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    invalidEntry.isActive = true;
    invalidEntry.tier = EldernodeTier::ELDERFIER;
    invalidEntry.serviceId = ElderfierServiceId::createCustomName("TooLongName123"); // > 8 chars
    
    bool added = manager.addEldernode(invalidEntry);
    assert(!added); // Should fail
    std::cout << "✓ Rejected invalid custom name (too long)" << std::endl;
    
    // Test 2: Reserved custom name
    ENindexEntry reservedEntry;
    Crypto::generate_keys(reservedEntry.eldernodePublicKey, reservedEntry.eldernodeSecretKey);
    reservedEntry.feeAddress = "FUEGO123456789abcdef";
    reservedEntry.stakeAmount = 5000000;
    reservedEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    reservedEntry.isActive = true;
    reservedEntry.tier = EldernodeTier::ELDERFIER;
    reservedEntry.serviceId = ElderfierServiceId::createCustomName("admin"); // Reserved name
    
    bool addedReserved = manager.addEldernode(reservedEntry);
    assert(!addedReserved); // Should fail
    std::cout << "✓ Rejected reserved custom name" << std::endl;
    
    // Test 3: Valid custom name
    ENindexEntry validEntry;
    Crypto::generate_keys(validEntry.eldernodePublicKey, validEntry.eldernodeSecretKey);
    validEntry.feeAddress = "FUEGO123456789abcdef";
    validEntry.stakeAmount = 5000000;
    validEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    validEntry.isActive = true;
    validEntry.tier = EldernodeTier::ELDERFIER;
    validEntry.serviceId = ElderfierServiceId::createCustomName("MyNode");
    
    bool addedValid = manager.addEldernode(validEntry);
    assert(addedValid); // Should succeed
    std::cout << "✓ Accepted valid custom name" << std::endl;
}

void testTierPrioritization() {
    std::cout << "Testing Tier Prioritization..." << std::endl;
    
    EldernodeIndexManager manager;
    
    // Add Basic Eldernode
    ENindexEntry basicEntry;
    Crypto::generate_keys(basicEntry.eldernodePublicKey, basicEntry.eldernodeSecretKey);
    basicEntry.feeAddress = "FUEGO123456789abcdef";
    basicEntry.stakeAmount = 1000000;
    basicEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    basicEntry.isActive = true;
    basicEntry.tier = EldernodeTier::BASIC;
    
    bool addedBasic = manager.addEldernode(basicEntry);
    assert(addedBasic);
    
    // Add Elderfier with lower stake
    ENindexEntry elderfierEntry;
    Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
    elderfierEntry.feeAddress = "FUEGO987654321fedcba";
    elderfierEntry.stakeAmount = 5000000; // Higher than basic minimum
    elderfierEntry.registrationTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    elderfierEntry.isActive = true;
    elderfierEntry.tier = EldernodeTier::ELDERFIER;
    elderfierEntry.serviceId = ElderfierServiceId::createCustomName("Priority");
    
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

void testEldernodeIndexManager() {
    std::cout << "Running comprehensive Eldernode Index Manager tests..." << std::endl;
    
    testBasicEldernode();
    testElderfierServiceNode();
    testServiceIdValidation();
    testTierPrioritization();
    
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
