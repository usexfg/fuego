// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/ElderfierServiceModifierBuilder.h"
#include "../../include/ElderfierServiceKernelBuilder.h"
#include "../../include/ElderfierServiceProofGenerator.h"
#include "../../include/ElderfierServiceProofVerifier.h"
#include "../../include/ElderfierServiceTypesSimple.h"
#include <iostream>
#include <memory>

// Forward declaration - in real implementation, this would be the actual core
namespace CryptoNote {
    class core {
    public:
        bool getTopBlock() { return true; } // Placeholder
    };
}

using namespace CryptoNote;

/**
 * @brief Example demonstrating Elderfier service integration
 * 
 * This example shows how to:
 * 1. Initialize the Elderfier service components
 * 2. Generate a service access proof
 * 3. Verify the proof
 * 4. Handle service access control
 */
class ElderfierServiceExample {
private:
    std::unique_ptr<core> m_core;
    std::unique_ptr<ElderfierServiceModifierBuilder> m_modifierBuilder;
    std::unique_ptr<ElderfierServiceKernelBuilder> m_kernelBuilder;
    std::unique_ptr<ElderfierServiceProofGenerator> m_proofGenerator;
    std::unique_ptr<ElderfierServiceProofVerifier> m_proofVerifier;
    
public:
    ElderfierServiceExample() {
        // Initialize core (placeholder)
        m_core = std::make_unique<core>();
        
        // Initialize Elderfier service components
        m_modifierBuilder = std::make_unique<ElderfierServiceModifierBuilder>(*m_core);
        m_kernelBuilder = std::make_unique<ElderfierServiceKernelBuilder>(*m_modifierBuilder);
        m_proofGenerator = std::make_unique<ElderfierServiceProofGenerator>(*m_kernelBuilder);
        m_proofVerifier = std::make_unique<ElderfierServiceProofVerifier>(*m_modifierBuilder);
        
        std::cout << "Elderfier Service initialized successfully" << std::endl;
    }
    
    /**
     * @brief Demonstrate complete proof generation and verification flow
     */
    void demonstrateProofFlow() {
        std::cout << "\n=== Elderfier Service Proof Flow Demo ===" << std::endl;
        
        // Example fee address (in real implementation, this would be from user input)
        std::string feeAddress = "FuegoElderfierServiceAddress123456789";
        uint64_t currentHeight = 1000;
        uint64_t minimumStake = 800000000000; // 800 XFG in atomic units
        
        std::cout << "Fee Address: " << feeAddress << std::endl;
        std::cout << "Current Height: " << currentHeight << std::endl;
        std::cout << "Minimum Stake: " << minimumStake << " atomic units" << std::endl;
        
        // Step 1: Generate service access proof
        std::cout << "\n--- Step 1: Generating Service Access Proof ---" << std::endl;
        ElderfierServiceProof proof;
        
        if (m_proofGenerator->generateStakeProof(feeAddress, minimumStake, currentHeight, proof)) {
            std::cout << "✓ Proof generated successfully!" << std::endl;
            std::cout << "Proof Hash: ";
            for (uint8_t byte : proof.proof_hash) {
                printf("%02x", byte);
            }
            std::cout << std::endl;
            std::cout << "Proof Timestamp: " << proof.proof_timestamp << std::endl;
            std::cout << "Proof Sequence: " << proof.proof_sequence << std::endl;
        } else {
            std::cout << "✗ Failed to generate proof" << std::endl;
            return;
        }
        
        // Step 2: Verify the proof
        std::cout << "\n--- Step 2: Verifying Service Access Proof ---" << std::endl;
        
        if (m_proofVerifier->verifyServiceAccessProof(proof, currentHeight, feeAddress)) {
            std::cout << "✓ Proof verified successfully!" << std::endl;
            std::cout << "Service access granted for Elderfier operations" << std::endl;
        } else {
            std::cout << "✗ Proof verification failed" << std::endl;
            return;
        }
        
        // Step 3: Demonstrate replay protection
        std::cout << "\n--- Step 3: Testing Replay Protection ---" << std::endl;
        
        if (m_proofVerifier->verifyServiceAccessProof(proof, currentHeight, feeAddress)) {
            std::cout << "✗ Replay protection failed - proof accepted twice!" << std::endl;
        } else {
            std::cout << "✓ Replay protection working - proof rejected on second use" << std::endl;
        }
        
        // Step 4: Show service statistics
        std::cout << "\n--- Step 4: Service Statistics ---" << std::endl;
        std::cout << "Cache Size: " << m_proofGenerator->getCacheSize() << " proofs" << std::endl;
        std::cout << "Used Proofs: " << m_proofVerifier->getUsedProofCount() << " proofs" << std::endl;
        
        std::cout << "\n=== Demo Complete ===" << std::endl;
    }
    
    /**
     * @brief Demonstrate service modifier management
     */
    void demonstrateModifierManagement() {
        std::cout << "\n=== Service Modifier Management Demo ===" << std::endl;
        
        uint64_t currentHeight = 1000;
        
        // Check if modifier needs update
        if (m_modifierBuilder->needsUpdate(currentHeight)) {
            std::cout << "Service modifier needs update" << std::endl;
            
            // Force update
            if (m_modifierBuilder->forceUpdate(currentHeight)) {
                std::cout << "✓ Service modifier updated successfully" << std::endl;
            } else {
                std::cout << "✗ Failed to update service modifier" << std::endl;
            }
        } else {
            std::cout << "Service modifier is current" << std::endl;
        }
        
        // Get current modifier
        const ElderfierServiceModifier& currentModifier = m_modifierBuilder->getCurrentModifier();
        std::cout << "Current Modifier:" << std::endl;
        std::cout << "  Height: " << currentModifier.last_pow_block_height << std::endl;
        std::cout << "  Timestamp: " << currentModifier.modifier_timestamp << std::endl;
        std::cout << "  Sequence: " << currentModifier.modifier_sequence << std::endl;
    }
    
    /**
     * @brief Demonstrate kernel building
     */
    void demonstrateKernelBuilding() {
        std::cout << "\n=== Service Kernel Building Demo ===" << std::endl;
        
        std::string feeAddress = "FuegoElderfierServiceAddress123456789";
        uint64_t currentHeight = 1000;
        uint64_t minimumStake = 800000000000; // 800 XFG
        
        ElderfierServiceKernel kernel;
        
        if (m_kernelBuilder->buildKernel(feeAddress, minimumStake, currentHeight, kernel)) {
            std::cout << "✓ Kernel built successfully!" << std::endl;
            std::cout << "Kernel Timestamp: " << kernel.kernel_timestamp << std::endl;
            std::cout << "Minimum Stake: " << kernel.minimum_stake_atomic << " atomic units" << std::endl;
        } else {
            std::cout << "✗ Failed to build kernel" << std::endl;
        }
    }
    
    /**
     * @brief Run all demonstrations
     */
    void runAllDemos() {
        demonstrateModifierManagement();
        demonstrateKernelBuilding();
        demonstrateProofFlow();
    }
};

/**
 * @brief Main function demonstrating Elderfier service usage
 */
int main() {
    try {
        std::cout << "Elderfier Service Integration Example" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        ElderfierServiceExample example;
        example.runAllDemos();
        
        std::cout << "\nExample completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
