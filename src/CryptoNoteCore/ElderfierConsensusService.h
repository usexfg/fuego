// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#pragma once

#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include "IObservable.h"

#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/ThresholdSignature.h"
#include "Logging/LoggerRef.h"
#include "Common/ObserverManager.h"

namespace CryptoNote {

class IEldernodeIndexManager;
class ICore;

enum class ConsensusPath {
    FAST_PASS = 0,
    FALLBACK = 1,
    COUNCIL_REVIEW = 2  // Only for failed consensus cases
};

enum class ConsensusStatus {
    PENDING = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2,
    FAILED = 3,
    COUNCIL_REVIEW = 4
};

struct ConsensusFailureDetail {
    Crypto::Hash burn_tx_hash;
    ConsensusPath path_attempted;
    std::vector<Crypto::PublicKey> selected_elders;
    std::vector<Crypto::PublicKey> responding_elders;
    std::vector<Crypto::PublicKey> non_responding_elders;
    uint32_t signatures_received;
    uint64_t failure_time;
    std::string failure_reason; // "TIMEOUT", "INSUFFICIENT_SIGNATURES", "INVALID_SIGNATURES"
};

struct ProofRequest {
    Crypto::Hash burn_tx_hash;
    ConsensusPath path;
    uint64_t start_time;
    std::vector<Crypto::PublicKey> selected_elders;
    std::vector<uint8_t> aggregated_signature;
    uint32_t signatures_received;
    ConsensusStatus status;
    std::vector<ConsensusFailureDetail> failure_history;
};

struct ConsensusResult {
    bool success;
    ConsensusPath path;
    std::vector<Crypto::PublicKey> winners;
    Crypto::Signature threshold_signature;
    uint64_t completion_time;
};

class ElderfierConsensusService : public IObservable<ElderfierConsensusService> {
public:
    ElderfierConsensusService(ICore& core, IEldernodeIndexManager& elderIndex, Logging::ILogger& logger);
    ~ElderfierConsensusService();

    // Public interface
    bool startProofRequest(const Crypto::Hash& burn_tx_hash, ConsensusPath path);
    std::optional<ConsensusResult> getConsensusResult(const Crypto::Hash& burn_tx_hash);
    ConsensusStatus getConsensusStatus(const Crypto::Hash& burn_tx_hash);
    bool submitPartialSignature(const Crypto::Hash& burn_tx_hash, const std::vector<uint8_t>& signature, const Crypto::PublicKey& signer);

    // Configuration
    void setConsensusTimeouts(uint32_t fast_pass_timeout, uint32_t fallback_timeout, uint32_t council_timeout);
    void setQuorumThresholds(uint8_t fast_pass_quorum, uint8_t fallback_quorum, uint8_t council_quorum);

    // Elder Council voting for failed consensus
    bool submitCouncilVote(const Crypto::Hash& burn_tx_hash, const std::string& vote_choice, const Crypto::Signature& signature);
    std::vector<std::string> getCouncilVotes(const Crypto::Hash& burn_tx_hash);

    // Strike tracking system
    void recordConsensusFailure(const ConsensusFailureDetail& failure);
    uint32_t getElderfierStrikes(const Crypto::PublicKey& elder_key);
    std::vector<std::pair<Crypto::PublicKey, uint32_t>> getAllStrikes();

    // Council review with detailed failure information
    std::optional<ConsensusFailureDetail> getDetailedFailureInfo(const Crypto::Hash& burn_tx_hash);

private:
    // Core references
    ICore& m_core;
    IEldernodeIndexManager& m_elderIndex;
    Logging::LoggerRef m_logger;

    // Configuration
    uint32_t m_fast_pass_timeout = 180;  // 3 minutes
    uint32_t m_fallback_timeout = 360;   // 6 minutes
    uint32_t m_council_timeout = 3600;   // 1 hour
    uint8_t m_fast_pass_quorum = 3;      // 3/3
    uint8_t m_fallback_quorum = 6;       // 6/8 (75%)
    uint8_t m_council_quorum = 8;        // 8/10 (80%)

    // Active proof requests
    std::unordered_map<Crypto::Hash, ProofRequest> m_active_proofs;
    mutable std::mutex m_proofs_mutex;

    // Elder Council votes for failed consensus
    std::unordered_map<Crypto::Hash, std::vector<std::pair<std::string, Crypto::Signature>>> m_council_votes;
    mutable std::mutex m_votes_mutex;

    // Strike tracking for Elderfiers
    std::unordered_map<Crypto::PublicKey, uint32_t> m_elderfier_strikes;
    mutable std::mutex m_strikes_mutex;

    // Timer for consensus timeouts
    std::atomic<bool> m_running;
    std::thread m_timer_thread;

    // Helper methods
    std::vector<Crypto::PublicKey> selectEldernodeQuorum(const Crypto::Hash& burn_tx_hash, ConsensusPath path);
    bool validatePartialSignature(const ProofRequest& request, const std::vector<uint8_t>& signature, const Crypto::PublicKey& signer);
    bool aggregateSignatures(ProofRequest& request);
    void processConsensusTimeout(const Crypto::Hash& burn_tx_hash);
    void escalateToFallback(const Crypto::Hash& burn_tx_hash);
    void escalateToCouncil(const Crypto::Hash& burn_tx_hash);
    void cleanupExpiredProofs();

    // Timer thread
    void timerLoop();
};

} // namespace CryptoNote
