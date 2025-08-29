#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "Common/JsonValue.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/Currency.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteCore/ICore.h"

namespace CryptoNote {

// Forward declarations
class core;
class NodeServer;
class ICryptoNoteProtocolQuery;

/**
 * @brief Integrated Eldernode Service for the main daemon
 * 
 * This service integrates directly with the Fuego daemon to provide
 * Eldernode functionality without requiring separate compilation.
 */
class EldernodeService {
public:
    EldernodeService(core& c, NodeServer& p2p, const ICryptoNoteProtocolQuery& protocolQuery);
    ~EldernodeService();

    // Service lifecycle
    bool initialize(const std::string& feeAddress, const std::string& viewKey,
                   const std::string& configPath, const std::string& registryUrl);
    void shutdown();
    bool isRunning() const;

    // Configuration
    bool loadConfiguration(const std::string& configPath);
    bool loadEldernodeRegistryFromUrl(const std::string& registryUrl);
    void setProgressiveConsensus(); // Hardcoded: 2/2 → 3/5
    
    // Fee address identity
    std::string getEldernodeIdentity() const;
    bool verifyFeeTransaction(const std::string& txnHash) const;
    
    // Stake verification
    bool verifyMinimumStake() const;
    uint64_t getCurrentStake() const;
    std::string getStakeStatus() const;

    // Consensus operations
    std::string generateConsensusProof(const std::string& txnHash, uint64_t consensusThreshold = 0);
    std::string getEldernodeStatistics() const;
    std::string getConsensusStatus() const;

    // RPC endpoint handlers
    bool handleGetValidationProof(const std::string& request, std::string& response);
    bool handleGetEldernodeStatistics(const std::string& request, std::string& response);
    bool handleStartEldernodeMonitoring(const std::string& request, std::string& response);
    bool handleStopEldernodeMonitoring(const std::string& request, std::string& response);

private:
    // Core references
    core& m_core;
    NodeServer& m_p2p;
    const ICryptoNoteProtocolQuery& m_protocolQuery;
    
    // Fee address identity
    std::string m_feeAddress;
    std::string m_viewKey;
    AccountPublicAddress m_feeAccount;

    // Service state
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_isMonitoring{false};
    std::thread m_monitoringThread;
    mutable std::mutex m_serviceMutex;

    // Configuration
    std::vector<std::string> m_knownEldernodes;
    std::string m_configPath;
    std::string m_registryUrl;
    
    // Hardcoded Progressive Consensus
    static const uint64_t FAST_CONSENSUS_THRESHOLD = 2;
    static const uint64_t FAST_CONSENSUS_TOTAL = 2;
    static const uint64_t ROBUST_CONSENSUS_THRESHOLD = 3;
    static const uint64_t ROBUST_CONSENSUS_TOTAL = 5;
    
    // Stake Requirements
    static const uint64_t MINIMUM_STAKE_ATOMIC = 800000000000; // 800 XFG in atomic units (7 decimal places)
    static const double MINIMUM_STAKE_XFG = 800.0; // 800 XFG

    // Statistics
    std::atomic<uint64_t> m_totalProofsProcessed{0};
    std::atomic<uint64_t> m_totalProofsSubmitted{0};
    std::atomic<uint64_t> m_totalProofsFailed{0};
    std::atomic<uint64_t> m_lastProcessedBlock{0};

    // Internal methods
    void monitoringLoop();
    bool validateTransaction(const std::string& txnHash);
    std::string createValidationProof(const std::string& txnHash, uint64_t blockHeight, 
                                    const std::string& blockHash, uint64_t amount);
    std::vector<std::string> selectRandomEldernodes(uint64_t count);
    bool validateConsensusProofs(const std::vector<std::string>& proofs);
    std::string mergeConsensusProofs(const std::vector<std::string>& proofs, uint64_t threshold);

    // Helper methods
    std::string generateEldernodeId() const;
    std::string getCurrentTimestamp() const;
    bool isLargeBurn(uint64_t amount) const;
    uint64_t getBlockHeight(const std::string& blockHash) const;
    std::string getBlockHash(uint64_t height) const;
};

} // namespace CryptoNote
