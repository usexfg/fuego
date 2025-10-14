// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "AdaptiveDifficulty.h"
#include "CryptoNoteConfig.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace CryptoNote {

    AdaptiveDifficulty::AdaptiveDifficulty(const DifficultyConfig& config) 
        : m_config(config) {
    }

    uint64_t AdaptiveDifficulty::calculateNextDifficulty(
        uint32_t height,
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& cumulativeDifficulties) {
        
        // Early chain protection
        if (timestamps.size() < 3) {
            return 10000; // Minimum difficulty
        }
        
        // Check for emergency conditions
        if (detectHashRateAnomaly(timestamps, cumulativeDifficulties)) {
            return calculateEmergencyDifficulty(timestamps, cumulativeDifficulties);
        }
        
        // Check for block stealing attempts
        if (detectBlockStealingAttempt(timestamps, cumulativeDifficulties)) {
            return calculateEmergencyDifficulty(timestamps, cumulativeDifficulties);
        }
        
        // Use multi-window adaptive algorithm
        return calculateMultiWindowDifficulty(timestamps, cumulativeDifficulties);
    }

    uint64_t AdaptiveDifficulty::calculateMultiWindowDifficulty(
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& cumulativeDifficulties) {
        
        // Calculate LWMA for different windows
        double shortLWMA = calculateLWMA(timestamps, cumulativeDifficulties, m_config.shortWindow);
        double mediumLWMA = calculateLWMA(timestamps, cumulativeDifficulties, m_config.mediumWindow);
        double longLWMA = calculateLWMA(timestamps, cumulativeDifficulties, m_config.longWindow);
        
        // Calculate EMA for trend analysis (currently unused but available for future enhancements)
        // double shortEMA = calculateEMA(timestamps, m_config.shortWindow, 0.2);
        // double mediumEMA = calculateEMA(timestamps, m_config.mediumWindow, 0.1);
        
        // Weighted combination based on confidence
        double confidence = calculateConfidenceScore(timestamps, cumulativeDifficulties);
        
        // Adaptive weighting based on network conditions
        double shortWeight = 0.4 * confidence;
        double mediumWeight = 0.4 * confidence;
        double longWeight = 0.2 * (1.0 - confidence);
        
        // Calculate weighted average solve time
        double weightedSolveTime = (shortLWMA * shortWeight + 
                                   mediumLWMA * mediumWeight + 
                                   longLWMA * longWeight) / 
                                   (shortWeight + mediumWeight + longWeight);
        
        // Calculate current average difficulty
        uint32_t effectiveWindow = std::min(static_cast<uint32_t>(timestamps.size() - 1), m_config.mediumWindow);

        // Prevent division by zero
        if (effectiveWindow == 0) {
            return 10000; // Minimum difficulty
        }

        uint64_t avgDifficulty = (cumulativeDifficulties[effectiveWindow] - cumulativeDifficulties[0]) / effectiveWindow;
        
        // Calculate new difficulty
        if (weightedSolveTime <= 0.0) {
            return 10000; // Minimum difficulty for invalid solve time
        }

        double difficultyRatio = static_cast<double>(m_config.targetTime) / weightedSolveTime;

        // Apply bounds
        difficultyRatio = std::max(m_config.minAdjustment,
                                  std::min(m_config.maxAdjustment, difficultyRatio));
        
        // Prevent overflow in multiplication
        double calculatedDifficulty = static_cast<double>(avgDifficulty) * difficultyRatio;

        // Clamp to prevent overflow
        if (calculatedDifficulty > static_cast<double>(std::numeric_limits<uint64_t>::max())) {
            calculatedDifficulty = static_cast<double>(std::numeric_limits<uint64_t>::max());
        }

        uint64_t newDifficulty = static_cast<uint64_t>(calculatedDifficulty);

        // Apply smoothing to prevent oscillations
        if (timestamps.size() > 1) {
            uint64_t prevDifficulty = cumulativeDifficulties[effectiveWindow] - cumulativeDifficulties[effectiveWindow - 1];
            newDifficulty = applySmoothing(newDifficulty, prevDifficulty);
        }

        // Minimum difficulty protection
        return std::max(static_cast<uint64_t>(10000), newDifficulty);
    }

    double AdaptiveDifficulty::calculateLWMA(
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& cumulativeDifficulties,
        uint32_t windowSize) {
        
        uint32_t effectiveWindow = std::min(static_cast<uint32_t>(timestamps.size() - 1), windowSize);
        
        double weightedSum = 0.0;
        double weightSum = 0.0;
        
        for (uint32_t i = 1; i <= effectiveWindow; ++i) {
            int64_t solveTime = static_cast<int64_t>(timestamps[i]) - static_cast<int64_t>(timestamps[i - 1]);

            // Clamp solve time to prevent manipulation
            solveTime = std::max(static_cast<int64_t>(m_config.targetTime / 10),
                                std::min(static_cast<int64_t>(m_config.targetTime * 10), solveTime));

            double weight = static_cast<double>(i);
            weightedSum += solveTime * weight;
            weightSum += weight;
        }

        // Prevent division by zero
        if (weightSum == 0.0) {
            return static_cast<double>(m_config.targetTime);
        }

        return weightedSum / weightSum;
    }

    double AdaptiveDifficulty::calculateEMA(
        const std::vector<uint64_t>& timestamps,
        uint32_t windowSize,
        double alpha) {
        
        uint32_t effectiveWindow = std::min(static_cast<uint32_t>(timestamps.size() - 1), windowSize);
        
        if (effectiveWindow == 0) return static_cast<double>(m_config.targetTime);
        
        double ema = static_cast<double>(timestamps[1] - timestamps[0]);
        
        for (uint32_t i = 2; i <= effectiveWindow; ++i) {
            int64_t solveTime = static_cast<int64_t>(timestamps[i]) - static_cast<int64_t>(timestamps[i - 1]);
            solveTime = std::max(static_cast<int64_t>(m_config.targetTime / 10), 
                               std::min(static_cast<int64_t>(m_config.targetTime * 10), solveTime));
            
            ema = alpha * solveTime + (1.0 - alpha) * ema;
        }
        
        return ema;
    }

    uint64_t AdaptiveDifficulty::calculateEmergencyDifficulty(
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& cumulativeDifficulties) {
        
        uint32_t emergencyWindow = std::min(static_cast<uint32_t>(timestamps.size() - 1), m_config.emergencyWindow);

        if (emergencyWindow == 0) return 10000;

        // Calculate recent solve time
        double recentSolveTime = static_cast<double>(timestamps[emergencyWindow] - timestamps[0]) / emergencyWindow;

        if (recentSolveTime <= 0.0) {
            return 10000; // Minimum difficulty for invalid solve time
        }

        // Calculate current difficulty
        uint64_t currentDifficulty = (cumulativeDifficulties[emergencyWindow] - cumulativeDifficulties[0]) / emergencyWindow;
        
        // Emergency adjustment
        double emergencyRatio = static_cast<double>(m_config.targetTime) / recentSolveTime;

        // Apply emergency bounds
        emergencyRatio = std::max(m_config.emergencyThreshold,
                                 std::min(1.0 / m_config.emergencyThreshold, emergencyRatio));

        // Prevent overflow in emergency calculation
        double emergencyDifficultyCalc = static_cast<double>(currentDifficulty) * emergencyRatio;

        // Clamp to prevent overflow
        if (emergencyDifficultyCalc > static_cast<double>(std::numeric_limits<uint64_t>::max())) {
            emergencyDifficultyCalc = static_cast<double>(std::numeric_limits<uint64_t>::max());
        }

        uint64_t emergencyDifficulty = static_cast<uint64_t>(emergencyDifficultyCalc);

        return std::max(static_cast<uint64_t>(10000), emergencyDifficulty);
    }

    bool AdaptiveDifficulty::detectHashRateAnomaly(
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& difficulties) {
        
        if (timestamps.size() < 5) return false;
        
        // Calculate recent vs historical solve times
        uint32_t recentWindow = std::min(5u, static_cast<uint32_t>(timestamps.size() - 1));
        uint32_t historicalWindow = std::min(20u, static_cast<uint32_t>(timestamps.size() - 1));
        
        double recentSolveTime = static_cast<double>(timestamps[recentWindow] - timestamps[0]) / recentWindow;
        double historicalSolveTime = static_cast<double>(timestamps[historicalWindow] - timestamps[historicalWindow - recentWindow]) / recentWindow;
        
        // Detect if recent solve time is significantly different
        double ratio = recentSolveTime / historicalSolveTime;
        
        return (ratio < 0.1 || ratio > 10.0); // 10x change threshold
    }

    bool AdaptiveDifficulty::detectBlockStealingAttempt(
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& difficulties) {
        
        if (timestamps.size() < 3) return false;
        
        // Detect suspiciously fast consecutive blocks
        uint32_t fastBlockCount = 0;
        uint32_t checkBlocks = std::min(static_cast<uint32_t>(5), static_cast<uint32_t>(timestamps.size() - 1));
        
        for (size_t i = 1; i <= checkBlocks; ++i) {
            int64_t solveTime = static_cast<int64_t>(timestamps[i]) - static_cast<int64_t>(timestamps[i - 1]);
            
            // If blocks are coming too fast (less than 5% of target time = ~24 seconds)
            if (solveTime < static_cast<int64_t>(m_config.targetTime / 20)) {
                fastBlockCount++;
            }
        }
        
        // Trigger if more than 2 blocks in 5 are suspiciously fast
        return fastBlockCount >= 2;
    }

    uint64_t AdaptiveDifficulty::applySmoothing(uint64_t newDifficulty, uint64_t previousDifficulty) {
        // Apply exponential smoothing to prevent oscillations
        double alpha = 0.3; // Smoothing factor

        // Prevent overflow by using double precision arithmetic
        double smoothed = alpha * static_cast<double>(newDifficulty) +
                         (1.0 - alpha) * static_cast<double>(previousDifficulty);

        // Clamp to prevent overflow
        if (smoothed > static_cast<double>(std::numeric_limits<uint64_t>::max())) {
            return std::numeric_limits<uint64_t>::max();
        }

        return static_cast<uint64_t>(smoothed);
    }

    double AdaptiveDifficulty::calculateConfidenceScore(
        const std::vector<uint64_t>& timestamps,
        const std::vector<uint64_t>& difficulties) {
        
        if (timestamps.size() < 3) return 0.5;
        
        // Calculate coefficient of variation for solve times
        std::vector<double> solveTimes;
        for (size_t i = 1; i < timestamps.size(); ++i) {
            solveTimes.push_back(static_cast<double>(timestamps[i] - timestamps[i - 1]));
        }
        
        double mean = std::accumulate(solveTimes.begin(), solveTimes.end(), 0.0) / solveTimes.size();
        double variance = 0.0;
        
        for (double solveTime : solveTimes) {
            variance += (solveTime - mean) * (solveTime - mean);
        }
        variance /= solveTimes.size();
        
        double coefficientOfVariation = std::sqrt(variance) / mean;
        
        // Convert to confidence score (lower variation = higher confidence)
        return std::max(0.1, std::min(1.0, 1.0 - coefficientOfVariation));
    }

    AdaptiveDifficulty::DifficultyConfig getDefaultFuegoConfig() {
        AdaptiveDifficulty::DifficultyConfig config;
        config.targetTime = CryptoNote::parameters::DIFFICULTY_TARGET; // 480 seconds
        config.shortWindow = 15;    // Rapid response
        config.mediumWindow = 45;   // Current window
        config.longWindow = 120;    // Trend analysis
        config.minAdjustment = 0.5; // 50% minimum change
        config.maxAdjustment = 4.0; // 400% maximum change
        config.emergencyThreshold = 0.1; // 10% emergency threshold
        config.emergencyWindow = 5; // Emergency response window
        
        return config;
    }

} // namespace CryptoNote