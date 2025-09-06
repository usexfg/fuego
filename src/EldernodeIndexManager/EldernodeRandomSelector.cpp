#include "EldernodeIndexManager/ElderfierDepositManager.h"
#include "EldernodeIndexTypes.h"
#include "CryptoTypes.h"
#include "Logging/LoggerRef.h"
#include <random>
#include <algorithm>
#include <numeric>

namespace CryptoNote {

class EldernodeRandomSelector {
public:
    EldernodeRandomSelector(Logging::ILogger& logger) : logger(logger, "EldernodeRandomSelector") {}
    
    // Select exactly 2 Elderfiers for verification using provably fair random selection
    ElderfierSelectionResult selectElderfiersForVerification(
        const std::vector<EldernodeConsensusParticipant>& availableElderfiers,
        uint64_t blockHeight,
        const Crypto::Hash& blockHash) const {
        
        logger(Logging::INFO) << "Selecting Elderfiers for verification at block " << blockHeight;
        
        ElderfierSelectionResult result;
        result.blockHeight = blockHeight;
        result.selectionHash = blockHash;
        
        // Filter only active Elderfiers
        std::vector<EldernodeConsensusParticipant> activeElderfiers;
        for (const auto& elderfier : availableElderfiers) {
            if (elderfier.isActive && elderfier.tier == EldernodeTier::ELDERFIER) {
                activeElderfiers.push_back(elderfier);
            }
        }
        
        if (activeElderfiers.size() < 2) {
            logger(Logging::WARNING) << "Not enough active Elderfiers for selection: " << activeElderfiers.size();
            return result; // Return empty result
        }
        
        // Calculate total weight (sum of all selection multipliers)
        result.totalWeight = 0;
        for (const auto& elderfier : activeElderfiers) {
            result.totalWeight += elderfier.selectionMultiplier;
            result.selectionWeights.push_back(elderfier.selectionMultiplier);
        }
        
        // Create weighted selection pool
        std::vector<size_t> selectionPool;
        for (size_t i = 0; i < activeElderfiers.size(); ++i) {
            for (uint32_t j = 0; j < activeElderfiers[i].selectionMultiplier; ++j) {
                selectionPool.push_back(i);
            }
        }
        
        // Use block hash as seed for provably fair randomness
        std::seed_seq seed_seq(blockHash.data, blockHash.data + sizeof(blockHash.data));
        std::mt19937 rng(seed_seq);
        
        // Select first Elderfier
        std::uniform_int_distribution<size_t> dist1(0, selectionPool.size() - 1);
        size_t firstIndex = selectionPool[dist1(rng)];
        result.selectedElderfiers.push_back(activeElderfiers[firstIndex]);
        
        // Remove selected Elderfier from pool to avoid duplicate selection
        selectionPool.erase(std::remove(selectionPool.begin(), selectionPool.end(), firstIndex), selectionPool.end());
        
        // Select second Elderfier
        if (!selectionPool.empty()) {
            std::uniform_int_distribution<size_t> dist2(0, selectionPool.size() - 1);
            size_t secondIndex = selectionPool[dist2(rng)];
            result.selectedElderfiers.push_back(activeElderfiers[secondIndex]);
        }
        
        logger(Logging::INFO) << "Selected " << result.selectedElderfiers.size() 
                              << " Elderfiers with total weight " << result.totalWeight;
        
        return result;
    }
    
    // Calculate selection multiplier based on uptime duration
    uint32_t calculateSelectionMultiplier(uint64_t totalUptimeSeconds) const {
        if (totalUptimeSeconds < SelectionMultipliers::MONTH_1_SECONDS) {
            return SelectionMultipliers::UPTIME_1_MONTH_MULTIPLIER;   // 1x (0-1 month)
        } else if (totalUptimeSeconds < SelectionMultipliers::MONTH_3_SECONDS) {
            return SelectionMultipliers::UPTIME_3_MONTH_MULTIPLIER;  // 2x (1-3 months)
        } else if (totalUptimeSeconds < SelectionMultipliers::MONTH_6_SECONDS) {
            return SelectionMultipliers::UPTIME_6_MONTH_MULTIPLIER;  // 4x (3-6 months)
        } else if (totalUptimeSeconds < SelectionMultipliers::YEAR_1_SECONDS) {
            return SelectionMultipliers::UPTIME_1_YEAR_MULTIPLIER;   // 8x (6-12 months)
        } else if (totalUptimeSeconds < SelectionMultipliers::YEAR_2_SECONDS) {
            return SelectionMultipliers::UPTIME_2_YEAR_MULTIPLIER;   // 16x (1-2 years)
        } else {
            return SelectionMultipliers::MAX_MULTIPLIER;             // Cap at 16x (2+ years)
        }
    }
    
    // Validate selection result
    bool validateSelectionResult(const ElderfierSelectionResult& result) const {
        if (result.selectedElderfiers.size() != 2) {
            logger(Logging::ERROR) << "Invalid selection: expected 2 Elderfiers, got " 
                                   << result.selectedElderfiers.size();
            return false;
        }
        
        // Check for duplicates
        if (result.selectedElderfiers[0].publicKey == result.selectedElderfiers[1].publicKey) {
            logger(Logging::ERROR) << "Invalid selection: duplicate Elderfiers selected";
            return false;
        }
        
        // Verify total weight calculation
        uint64_t calculatedWeight = 0;
        for (const auto& weight : result.selectionWeights) {
            calculatedWeight += weight;
        }
        
        if (calculatedWeight != result.totalWeight) {
            logger(Logging::ERROR) << "Invalid selection: weight mismatch";
            return false;
        }
        
        return true;
    }

private:
    Logging::LoggerRef logger;
};

} // namespace CryptoNote
