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

#include "DynamicRingSize.h"
#include "../CryptoNoteConfig.h"
#include <algorithm>
#include <sstream>

namespace CryptoNote {

size_t DynamicRingSizeCalculator::calculateOptimalRingSize(
  uint64_t amount,
  const std::vector<OutputInfo>& availableOutputs,
  uint8_t blockMajorVersion,
  size_t minRingSize,
  size_t maxRingSize
) {
  // For older block versions, use static ring size
  if (blockMajorVersion < BLOCK_MAJOR_VERSION_10) {
    return minRingSize;
  }
  
  // Get target ring sizes in order of preference
  std::vector<size_t> targetRingSizes = getTargetRingSizes();
  
  // Find the largest achievable ring size
  for (size_t targetSize : targetRingSizes) {
    if (targetSize >= minRingSize && targetSize <= maxRingSize) {
      if (isRingSizeAchievable(targetSize, availableOutputs)) {
        return targetSize;
      }
    }
  }
  
  // Fall back to minimum if no targets are achievable
  return minRingSize;
}

std::vector<size_t> DynamicRingSizeCalculator::getTargetRingSizes() {
  // Target ring sizes in order of preference (highest privacy first)
  return {18, 15, 12, 11, 10, 9, 8};
}

bool DynamicRingSizeCalculator::isRingSizeAchievable(
  size_t ringSize,
  const std::vector<OutputInfo>& availableOutputs
) {
  // Check if we have enough outputs of any amount to achieve the ring size
  for (const auto& output : availableOutputs) {
    if (output.availableCount >= ringSize) {
      return true;
    }
  }
  
  // Check if we can combine outputs from different amounts
  size_t totalAvailable = 0;
  for (const auto& output : availableOutputs) {
    totalAvailable += output.availableCount;
  }
  
  return totalAvailable >= ringSize;
}

std::string DynamicRingSizeCalculator::getPrivacyLevelDescription(size_t ringSize) {
  if (ringSize >= 18) {
    return "Maximum Privacy (Ring Size " + std::to_string(ringSize) + ")";
  } else if (ringSize >= 15) {
    return "Strong Privacy (Ring Size " + std::to_string(ringSize) + ")";
  } else if (ringSize >= 12) {
    return "Better Privacy (Ring Size " + std::to_string(ringSize) + ")";
  } else if (ringSize >= 10) {
    return "Good Privacy (Ring Size " + std::to_string(ringSize) + ")";
  } else if (ringSize >= 8) {
    return "Enhanced Privacy (Ring Size " + std::to_string(ringSize) + ")";
  } else {
    return "Basic Privacy (Ring Size " + std::to_string(ringSize) + ")";
  }
}

} // namespace CryptoNote
