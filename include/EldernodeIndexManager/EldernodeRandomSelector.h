#pragma once

#include "EldernodeIndexTypes.h"
#include "Logging/ILogger.h"
#include "Logging/LoggerRef.h"

namespace CryptoNote {

class EldernodeRandomSelector {
public:
    explicit EldernodeRandomSelector(Logging::ILogger& logger);
    
    // Select exactly 2 Elderfiers for verification using provably fair random selection
    ElderfierSelectionResult selectElderfiersForVerification(
        const std::vector<EldernodeConsensusParticipant>& availableElderfiers,
        uint64_t blockHeight,
        const Crypto::Hash& blockHash) const;
    
    // Calculate selection multiplier based on uptime duration
    uint32_t calculateSelectionMultiplier(uint64_t totalUptimeSeconds) const;
    
    // Validate selection result
    bool validateSelectionResult(const ElderfierSelectionResult& result) const;

private:
    Logging::LoggerRef logger;
};

} // namespace CryptoNote
