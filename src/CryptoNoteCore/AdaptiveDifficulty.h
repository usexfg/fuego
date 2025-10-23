#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace CryptoNote {

    // Adaptive Multi-Window Difficulty (DMWD) algorithm for Fuego
    
    class AdaptiveDifficulty {
    public:
        struct DifficultyConfig {
            uint64_t targetTime;           // Target block time (480 seconds)
            uint32_t shortWindow;          // Short window for rapid response (15 blocks)
            uint32_t mediumWindow;         // Medium window for stability (45 blocks)
            uint32_t longWindow;           // Long window for trend analysis (120 blocks)
            double minAdjustment;          // Minimum difficulty adjustment (0.5x)
            double maxAdjustment;          // Maximum difficulty adjustment (4.0x)
            double emergencyThreshold;     // Emergency threshold for rapid changes (0.1x or 10x)
            uint32_t emergencyWindow;      // Emergency response window (5 blocks)
        };

        struct BlockData {
            uint64_t timestamp;
            uint64_t difficulty;
            uint64_t cumulativeDifficulty;
        };

        AdaptiveDifficulty(const DifficultyConfig& config);
        
        // Main difficulty calculation function
        uint64_t calculateNextDifficulty(
            uint32_t height,
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& cumulativeDifficulties
        );

        // Emergency response for sudden hash rate changes
        uint64_t calculateEmergencyDifficulty(
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& cumulativeDifficulties
        );

        // Anti-block-stealing mechanism
        bool detectBlockStealingAttempt(
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& difficulties
        );

    private:
        DifficultyConfig m_config;
        
        // Calculate difficulty using multiple windows
        uint64_t calculateMultiWindowDifficulty(
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& cumulativeDifficulties
        );
        
        // Calculate LWMA for a specific window
        double calculateLWMA(
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& cumulativeDifficulties,
            uint32_t windowSize
        );
        
        // Calculate EMA (Exponential Moving Average) for trend analysis
        double calculateEMA(
            const std::vector<uint64_t>& timestamps,
            uint32_t windowSize,
            double alpha = 0.1
        );
        
        // Detect hash rate anomalies
        bool detectHashRateAnomaly(
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& difficulties
        );
        
        // Apply smoothing to prevent oscillations
        uint64_t applySmoothing(uint64_t newDifficulty, uint64_t previousDifficulty);
        
        // Calculate confidence score for difficulty adjustment
        double calculateConfidenceScore(
            const std::vector<uint64_t>& timestamps,
            const std::vector<uint64_t>& difficulties
        );
    };

    // Default config
    AdaptiveDifficulty::DifficultyConfig getDefaultFuegoConfig();

} // namespace CryptoNote
