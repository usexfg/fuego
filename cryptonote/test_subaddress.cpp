#include <iostream>
#include <vector>
#include <string>
#include "src/crypto/crypto.h"
#include "src/crypto/subaddress.h"
#include "include/CryptoNote.h"
#include "src/Common/StringTools.h"

using namespace Crypto;
using namespace CryptoNote;

void test_basic_key_generation() {
    std::cout << "=== Testing Basic Key Generation ===" << std::endl;
    
    // Generate main wallet keys
    PublicKey viewPublicKey, spendPublicKey;
    SecretKey viewSecretKey, spendSecretKey;
    
    generate_keys(viewPublicKey, viewSecretKey);
    generate_keys(spendPublicKey, spendSecretKey);
    
    std::cout << "✓ Generated main wallet keys" << std::endl;
    
    // Verify key conversion works
    PublicKey derivedViewPublicKey;
    bool success = secret_key_to_public_key(viewSecretKey, derivedViewPublicKey);
    if (success && viewPublicKey == derivedViewPublicKey) {
        std::cout << "✓ View key conversion works correctly" << std::endl;
    } else {
        std::cout << "✗ View key conversion failed" << std::endl;
        return;
    }
}

void test_subaddress_derivation() {
    std::cout << "\n=== Testing Subaddress Derivation ===" << std::endl;
    
    // Generate main wallet keys
    PublicKey viewPublicKey, spendPublicKey;
    SecretKey viewSecretKey, spendSecretKey;
    
    generate_keys(viewPublicKey, viewSecretKey);
    generate_keys(spendPublicKey, spendSecretKey);
    
    // Test subaddress derivation for different indices
    for (uint32_t i = 0; i < 3; i++) {
        SecretKey derivedSpendKey, derivedViewKey;
        
        try {
            derive_subaddress_keys(viewSecretKey, spendSecretKey, i, derivedSpendKey, derivedViewKey);
            
            // Convert derived secret keys to public keys
            PublicKey derivedSpendPublicKey, derivedViewPublicKey;
            bool spendSuccess = secret_key_to_public_key(derivedSpendKey, derivedSpendPublicKey);
            bool viewSuccess = secret_key_to_public_key(derivedViewKey, derivedViewPublicKey);
            
            if (spendSuccess && viewSuccess) {
                std::cout << "✓ Subaddress " << i << " derived successfully" << std::endl;
                std::cout << "  Spend Public Key: " << Common::podToHex(derivedSpendPublicKey) << std::endl;
                std::cout << "  View Public Key: " << Common::podToHex(derivedViewPublicKey) << std::endl;
            } else {
                std::cout << "✗ Failed to convert derived keys to public keys for subaddress " << i << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ Exception during subaddress " << i << " derivation: " << e.what() << std::endl;
        }
    }
}

void test_deterministic_derivation() {
    std::cout << "\n=== Testing Deterministic Derivation ===" << std::endl;
    
    // Generate main wallet keys
    PublicKey viewPublicKey, spendPublicKey;
    SecretKey viewSecretKey, spendSecretKey;
    
    generate_keys(viewPublicKey, viewSecretKey);
    generate_keys(spendPublicKey, spendSecretKey);
    
    // Derive same subaddress twice
    SecretKey derivedSpendKey1, derivedViewKey1;
    SecretKey derivedSpendKey2, derivedViewKey2;
    
    derive_subaddress_keys(viewSecretKey, spendSecretKey, 5, derivedSpendKey1, derivedViewKey1);
    derive_subaddress_keys(viewSecretKey, spendSecretKey, 5, derivedSpendKey2, derivedViewKey2);
    
    if (derivedSpendKey1 == derivedSpendKey2 && derivedViewKey1 == derivedViewKey2) {
        std::cout << "✓ Deterministic derivation works (same subaddress index produces same keys)" << std::endl;
    } else {
        std::cout << "✗ Deterministic derivation failed" << std::endl;
    }
    
    // Test different indices produce different keys
    SecretKey derivedSpendKey3, derivedViewKey3;
    derive_subaddress_keys(viewSecretKey, spendSecretKey, 6, derivedSpendKey3, derivedViewKey3);
    
    if (derivedSpendKey1 != derivedSpendKey3) {
        std::cout << "✓ Different subaddress indices produce different keys" << std::endl;
    } else {
        std::cout << "✗ Different subaddress indices produced same keys" << std::endl;
    }
}

void test_transaction_compatibility() {
    std::cout << "\n=== Testing Transaction Compatibility ===" << std::endl;
    
    // Generate main wallet keys
    PublicKey viewPublicKey, spendPublicKey;
    SecretKey viewSecretKey, spendSecretKey;
    
    generate_keys(viewPublicKey, viewSecretKey);
    generate_keys(spendPublicKey, spendSecretKey);
    
    // Test traditional transaction key derivation (should still work)
    PublicKey txPublicKey;
    SecretKey txSecretKey;
    generate_keys(txPublicKey, txSecretKey);
    
    KeyDerivation txDerivation;
    bool txSuccess = generate_key_derivation(txPublicKey, viewSecretKey, txDerivation);
    
    if (txSuccess) {
        std::cout << "✓ Traditional transaction key derivation works" << std::endl;
        
        // Test deriving transaction output key
        PublicKey derivedTxKey;
        bool deriveSuccess = derive_public_key(txDerivation, 0, spendPublicKey, derivedTxKey);
        
        if (deriveSuccess) {
            std::cout << "✓ Transaction output key derivation works" << std::endl;
        } else {
            std::cout << "✗ Transaction output key derivation failed" << std::endl;
        }
    } else {
        std::cout << "✗ Traditional transaction key derivation failed" << std::endl;
    }
}

void test_subaddress_vs_transaction_keys() {
    std::cout << "\n=== Testing Subaddress vs Transaction Key Spaces ===" << std::endl;
    
    // Generate main wallet keys
    PublicKey viewPublicKey, spendPublicKey;
    SecretKey viewSecretKey, spendSecretKey;
    
    generate_keys(viewPublicKey, viewSecretKey);
    generate_keys(spendPublicKey, spendSecretKey);
    
    // Generate transaction keys
    PublicKey txPublicKey;
    SecretKey txSecretKey;
    generate_keys(txPublicKey, txSecretKey);
    
    // Test subaddress derivation
    SecretKey subSpendKey, subViewKey;
    derive_subaddress_keys(viewSecretKey, spendSecretKey, 0, subSpendKey, subViewKey);
    
    // Test transaction derivation
    KeyDerivation txDerivation;
    generate_key_derivation(txPublicKey, viewSecretKey, txDerivation);
    
    PublicKey derivedTxKey;
    derive_public_key(txDerivation, 0, spendPublicKey, derivedTxKey);
    
    // Convert subaddress keys to public keys for comparison
    PublicKey subSpendPublicKey, subViewPublicKey;
    secret_key_to_public_key(subSpendKey, subSpendPublicKey);
    secret_key_to_public_key(subViewKey, subViewPublicKey);
    
    std::cout << "Main spend public key: " << Common::podToHex(spendPublicKey) << std::endl;
    std::cout << "Subaddress spend public key: " << Common::podToHex(subSpendPublicKey) << std::endl;
    std::cout << "Transaction derived key: " << Common::podToHex(derivedTxKey) << std::endl;
    
    // Verify they're different (different key spaces)
    if (spendPublicKey != subSpendPublicKey && subSpendPublicKey != derivedTxKey) {
        std::cout << "✓ Subaddress and transaction keys are in different key spaces" << std::endl;
    } else {
        std::cout << "✗ Key spaces overlap (this would be a problem)" << std::endl;
    }
}

int main() {
    std::cout << "Testing Fuego Subaddress Implementation\n" << std::endl;
    
    try {
        test_basic_key_generation();
        test_subaddress_derivation();
        test_deterministic_derivation();
        test_transaction_compatibility();
        test_subaddress_vs_transaction_keys();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
        std::cout << "If all tests passed, subaddress implementation is working correctly!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 