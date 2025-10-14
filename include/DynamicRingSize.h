// Copyright (c) 2024 Fuego Developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace CryptoNote {

// Structure to hold output information for dynamic ring size calculation
struct OutputInfo {
  uint64_t amount;
  size_t availableCount;
  std::string description;
  
  OutputInfo(uint64_t amt, size_t count, const std::string& desc = "")
    : amount(amt), availableCount(count), description(desc) {}
};

// Dynamic ring size calculator
class DynamicRingSizeCalculator {
public:
  // Calculate optimal ring size for a given amount and available outputs
  static size_t calculateOptimalRingSize(
    uint64_t amount,
    const std::vector<OutputInfo>& availableOutputs,
    uint8_t blockMajorVersion,
    size_t minRingSize = 8,
    size_t maxRingSize = 20
  );
  
  // Get target ring sizes in order of preference
  static std::vector<size_t> getTargetRingSizes();
  
  // Check if a ring size is achievable with available outputs
  static bool isRingSizeAchievable(
    size_t ringSize,
    const std::vector<OutputInfo>& availableOutputs
  );
  
  // Get privacy level description for a ring size
  static std::string getPrivacyLevelDescription(size_t ringSize);
};

// Privacy levels for different ring sizes
enum class PrivacyLevel {
  BASIC = 8,      // Minimum enhanced privacy
  GOOD = 10,      // Good privacy
  BETTER = 12,    // Better privacy
  STRONG = 15,    // Strong privacy
  MAXIMUM = 18    // Maximum privacy
};

} // namespace CryptoNote
