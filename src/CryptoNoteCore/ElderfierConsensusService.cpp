// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#include "ElderfierConsensusService.h"
#include "ICore.h"
#include "IEldernodeIndexManager.h"
#include "crypto/crypto.h"
#include "crypto/ThresholdSignature.h"
#include "Common/StringTools.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <optional>
#include <iomanip>

namespace CryptoNote {

ElderfierConsensusService::ElderfierConsensusService(ICore& core, IEldernodeIndexManager& elderIndex, Logging::ILogger& logger)
    : m_core(core), m_elderIndex(elderIndex), m_logger(logger, "ElderfierConsensusService"), m_running(false) {

    m_running = true;
    m_timer_thread = std::thread(&ElderfierConsensusService::timerLoop, this);
}

ElderfierConsensusService::~ElderfierConsensusService() {
    m_running = false;
    if (m_timer_thread.joinable()) {
        m_timer_thread.join();
    }
}

bool ElderfierConsensusService::startProofRequest(const Crypto::Hash& burn_tx_hash, ConsensusPath path) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    // Check if already exists
    if (m_active_proofs.find(burn_tx_hash) != m_active_proofs.end()) {
        m_logger(Logging::WARNING) << "Proof request already exists for " << Common::podToHex(burn_tx_hash);
        return false;
    }

    // Select Eldernode quorum
    auto selected_elders = selectEldernodeQuorum(burn_tx_hash, path);
    if (selected_elders.empty()) {
        m_logger(Logging::ERROR) << "Failed to select Eldernode quorum for " << Common::podToHex(burn_tx_hash);
        return false;
    }

    // Create proof request
    ProofRequest request;
    request.burn_tx_hash = burn_tx_hash;
    request.path = path;
    request.start_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    request.selected_elders = selected_elders;
    request.signatures_received = 0;
    request.status = ConsensusStatus::PENDING;

    m_active_proofs[burn_tx_hash] = request;

    m_logger(Logging::INFO) << "Started proof request for " << Common::podToHex(burn_tx_hash)
                           << " with path " << static_cast<int>(path)
                           << " using " << selected_elders.size() << " Eldernodes";

    return true;
}

std::optional<ConsensusResult> ElderfierConsensusService::getConsensusResult(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end()) {
        return std::nullopt;
    }

    const auto& request = it->second;

    ConsensusResult result;
    result.path = request.path;
    result.completion_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (request.status == ConsensusStatus::COMPLETED) {
        result.success = true;
        result.threshold_signature = Crypto::Signature(); // Would be set during aggregation
        // Extract winners from aggregated signature data
        // This would need proper MuSig2/FROST implementation
        result.winners = request.selected_elders;
    } else if (request.status == ConsensusStatus::FAILED) {
        result.success = false;
        result.threshold_signature = Crypto::Signature();
        result.winners.clear(); // No winners on failure
    } else {
        // Still in progress or other status
        return std::nullopt;
    }

    return result;
}

ConsensusStatus ElderfierConsensusService::getConsensusStatus(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end()) {
        return ConsensusStatus::FAILED;
    }

    return it->second.status;
}

bool ElderfierConsensusService::submitPartialSignature(const Crypto::Hash& burn_tx_hash,
                                                      const std::vector<uint8_t>& signature,
                                                      const Crypto::PublicKey& signer) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end()) {
        m_logger(Logging::WARNING) << "No active proof request for " << Common::podToHex(burn_tx_hash);
        return false;
    }

    auto& request = it->second;

    // Validate signature
    if (!validatePartialSignature(request, signature, signer)) {
        m_logger(Logging::WARNING) << "Invalid partial signature from " << Common::podToHex(signer)
                                  << " for " << Common::podToHex(burn_tx_hash);
        return false;
    }

    // Add signature to aggregation
    request.signatures_received++;

    m_logger(Logging::TRACE) << "Received partial signature " << request.signatures_received
                           << "/" << request.selected_elders.size()
                           << " for " << Common::podToHex(burn_tx_hash);

    // TODO: In real implementation, we'd analyze if this signature conflicts with majority
    // For now, record participation (correct/incorrect determination happens later)
    recordElderfierParticipation(signer);

    // Check if we have enough signatures
    uint32_t required_signatures = 0;
    switch (request.path) {
        case ConsensusPath::FAST_PASS:
            required_signatures = m_fast_pass_quorum;
            break;
        case ConsensusPath::FALLBACK:
            required_signatures = m_fallback_quorum;
            break;
        case ConsensusPath::COUNCIL_REVIEW:
            // Council review doesn't require signatures - it's for failed consensus
            required_signatures = 0;
            break;
    }

    if (request.signatures_received >= required_signatures) {
        if (aggregateSignatures(request)) {
            request.status = ConsensusStatus::COMPLETED;
            m_logger(Logging::INFO) << "Consensus completed for " << Common::podToHex(burn_tx_hash)
                                   << " with " << request.signatures_received << " signatures";
        } else {
            // Aggregation failed - record failure and escalate to council review
            recordConsensusFailure(createFailureDetail(request, "INSUFFICIENT_SIGNATURES"));
            request.status = ConsensusStatus::FAILED;
            escalateToCouncil(burn_tx_hash);
        }
    }

    return true;
}

void ElderfierConsensusService::setConsensusTimeouts(uint32_t fast_pass_timeout,
                                                    uint32_t fallback_timeout,
                                                    uint32_t council_timeout) {
    m_fast_pass_timeout = fast_pass_timeout;
    m_fallback_timeout = fallback_timeout;
    m_council_timeout = council_timeout;
}

void ElderfierConsensusService::setQuorumThresholds(uint8_t fast_pass_quorum,
                                                   uint8_t fallback_quorum,
                                                   uint8_t council_quorum) {
    m_fast_pass_quorum = fast_pass_quorum;
    m_fallback_quorum = fallback_quorum;
    m_council_quorum = council_quorum;
}

bool ElderfierConsensusService::submitCouncilVote(const Crypto::Hash& burn_tx_hash,
                                                  const std::string& vote_choice,
                                                  const Crypto::Signature& signature) {
    std::lock_guard<std::mutex> lock(m_votes_mutex);

    // Validate vote choice - now based on aggregate slashing decisions
    if (vote_choice != "SLASH_ALL" && vote_choice != "SLASH_HALF" &&
        vote_choice != "SLASH_NONE" && vote_choice != "REVIEW_MORE") {
        m_logger(Logging::WARNING) << "Invalid vote choice: " << vote_choice;
        return false;
    }

    // TODO: Validate signature against Eldernode keys

    m_council_votes[burn_tx_hash].push_back({vote_choice, signature});

    m_logger(Logging::TRACE) << "Council vote received: " << vote_choice
                           << " for " << Common::podToHex(burn_tx_hash);

    return true;
}

std::vector<std::string> ElderfierConsensusService::getCouncilVotes(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_votes_mutex);

    std::vector<std::string> votes;
    auto it = m_council_votes.find(burn_tx_hash);
    if (it != m_council_votes.end()) {
        for (const auto& vote : it->second) {
            votes.push_back(vote.first);
        }
    }

    return votes;
}

// Strike tracking implementation for Elderfiers who provide conflicting proofs
void ElderfierConsensusService::recordConsensusFailure(const ConsensusFailureDetail& failure) {
    std::lock_guard<std::mutex> lock(m_strikes_mutex);

    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // TODO: In real implementation, we'd analyze which Elderfiers provided proofs
    // that conflicted with the majority consensus and give them strikes
    // For now, this logs the failure for council review

    m_logger(Logging::INFO) << "Recording consensus failure for "
                           << Common::podToHex(failure.burn_tx_hash);
    m_logger(Logging::INFO) << "Failure reason: " << failure.failure_reason
                           << ", Selected: " << failure.selected_elders.size()
                           << ", Responding: " << failure.responding_elders.size()
                           << ", Non-responding: " << failure.non_responding_elders.size();

    // Also record the failure detail in the proof request
    {
        std::lock_guard<std::mutex> proofs_lock(m_proofs_mutex);
        auto it = m_active_proofs.find(failure.burn_tx_hash);
        if (it != m_active_proofs.end()) {
            it->second.failure_history.push_back(failure);
        }
    }
}

void ElderfierConsensusService::recordElderfierParticipation(const Crypto::PublicKey& elder_key) {
    std::lock_guard<std::mutex> lock(m_strikes_mutex);

    auto& record = m_elderfier_strikes[elder_key];
    record.total_rounds_participated++;

    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (record.first_strike_time == 0) {
        record.first_strike_time = current_time;
    }
    record.last_strike_time = current_time;

    m_logger(Logging::TRACE) << "Elderfier " << Common::podToHex(elder_key)
                           << " participation recorded (rounds: " << record.total_rounds_participated << ")";
}

uint32_t ElderfierConsensusService::getElderfierStrikes(const Crypto::PublicKey& elder_key) {
    std::lock_guard<std::mutex> lock(m_strikes_mutex);
    auto it = m_elderfier_strikes.find(elder_key);
    return (it != m_elderfier_strikes.end()) ? it->second.strike_count : 0;
}

ElderfierConsensusService::StrikeRecord ElderfierConsensusService::getElderfierRecord(const Crypto::PublicKey& elder_key) {
    std::lock_guard<std::mutex> lock(m_strikes_mutex);
    auto it = m_elderfier_strikes.find(elder_key);
    if (it != m_elderfier_strikes.end()) {
        return it->second;
    }

    // Return empty record for Elderfiers with no strikes
    StrikeRecord empty_record = {0, 0, 0, 0};
    return empty_record;
}

std::vector<std::pair<Crypto::PublicKey, ElderfierConsensusService::StrikeRecord>> ElderfierConsensusService::getAllStrikes() {
    std::lock_guard<std::mutex> lock(m_strikes_mutex);

    std::vector<std::pair<Crypto::PublicKey, StrikeRecord>> strikes;
    for (const auto& pair : m_elderfier_strikes) {
        strikes.push_back(pair);
    }

    return strikes;
}

std::string ElderfierConsensusService::generateCouncilReviewMessage(const Crypto::PublicKey& elder_key) {
    const auto& record = getElderfierRecord(elder_key);

    if (record.strike_count < 3) {
        return ""; // Only generate messages for Elderfiers with 3+ strikes
    }

    std::ostringstream msg;
    msg << "ELDERFIER COUNCIL REVIEW REQUIRED\n";
    msg << "================================\n\n";
    msg << "An Elderfier has provided proof AGAINST consensus " << record.strike_count
        << " times in " << record.total_rounds_participated << " total rounds participated.\n\n";
    msg << "Strike Count: " << record.strike_count << "\n";
    msg << "Total Rounds Participated: " << record.total_rounds_participated << "\n";
    msg << "Strike Rate: " << std::fixed << std::setprecision(2)
        << (static_cast<double>(record.strike_count) / record.total_rounds_participated * 100) << "%\n\n";
    msg << "Evidence of conflicting proof submissions in consensus rounds:\n";
    msg << "- Round participation with incorrect consensus proofs\n";
    msg << "- Strike accumulation over multiple consensus attempts\n\n";

    msg << "Please vote your decision for action:\n";
    msg << "a) SLASH_ALL - Slash all Elderfiers with 3+ strikes\n";
    msg << "b) SLASH_HALF - Slash 50% of Elderfiers with 3+ strikes\n";
    msg << "c) SLASH_NONE - No slashing, continue monitoring\n";
    msg << "d) REVIEW_MORE - Request additional investigation\n\n";
    msg << "Reply with your vote (a/b/c/d) signed with your Elderfier key.";

    return msg.str();
}

std::optional<ConsensusFailureDetail> ElderfierConsensusService::getDetailedFailureInfo(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end() || it->second.failure_history.empty()) {
        return std::nullopt;
    }

    // Return the most recent failure
    return it->second.failure_history.back();
}

// Private methods

ConsensusFailureDetail ElderfierConsensusService::createFailureDetail(const ProofRequest& request, const std::string& reason) {
    ConsensusFailureDetail detail;
    detail.burn_tx_hash = request.burn_tx_hash;
    detail.path_attempted = request.path;
    detail.selected_elders = request.selected_elders;
    detail.failure_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    detail.failure_reason = reason;

    // In a real implementation, we'd track which Eldernodes actually responded to our P2P requests
    // For now, we'll categorize based on whether they were selected but didn't contribute to consensus
    // The key insight: strikes are for Elderfiers who were selected for consensus but failed to participate properly

    // Non-responding = selected for consensus but didn't provide valid signatures
    // This could be due to: network issues, malicious behavior, or being offline
    for (const auto& elder : request.selected_elders) {
        // TODO: In real implementation, track which specific Eldernodes responded vs didn't
        // For now, simplified: if we got any signatures, assume some responded
        // In production: track per-Eldernode response status
        if (request.signatures_received == 0) {
            // No signatures received - all selected Eldernodes are non-responding
            detail.non_responding_elders.push_back(elder);
        } else {
            // Some signatures received - in real implementation, check which specific Eldernodes responded
            // For now, simplified: assume all selected Eldernodes either responded or didn't
            detail.responding_elders.push_back(elder);
        }
    }

    detail.signatures_received = request.signatures_received;

    return detail;
}

std::vector<Crypto::PublicKey> ElderfierConsensusService::selectEldernodeQuorum(const Crypto::Hash& burn_tx_hash,
                                                                              ConsensusPath path) {
    // Get active Eldernodes
    auto active_elders = m_elderIndex.getActiveEldernodes();
    if (active_elders.empty()) {
        return {};
    }

    // Deterministic selection based on burn transaction hash
    std::vector<Crypto::PublicKey> selected;

    switch (path) {
        case ConsensusPath::FAST_PASS:
            // Select 3 Eldernodes
            for (size_t i = 0; i < 3 && i < active_elders.size(); ++i) {
                size_t index = (burn_tx_hash.data[i % 32] + i) % active_elders.size();
                selected.push_back(active_elders[index].publicKey);
            }
            break;

        case ConsensusPath::FALLBACK:
            // Select 8 Eldernodes (6 required for 75% consensus)
            for (size_t i = 0; i < 8 && i < active_elders.size(); ++i) {
                size_t index = (burn_tx_hash.data[i % 32] + i * 2) % active_elders.size();
                selected.push_back(active_elders[index].publicKey);
            }
            break;

        case ConsensusPath::COUNCIL_REVIEW:
            // Select all active Eldernodes
            for (const auto& elder : active_elders) {
                selected.push_back(elder.publicKey);
            }
            break;
    }

    return selected;
}

bool ElderfierConsensusService::validatePartialSignature(const ProofRequest& request,
                                                        const std::vector<uint8_t>& signature,
                                                        const Crypto::PublicKey& signer) {
    // Check if signer is in the selected quorum
    auto it = std::find(request.selected_elders.begin(), request.selected_elders.end(), signer);
    if (it == request.selected_elders.end()) {
        m_logger(Logging::WARNING) << "Signature from unauthorized Eldernode: " << Common::podToHex(signer);
        return false;
    }

    // TODO: Implement proper signature validation
    // This would validate the partial signature against the burn transaction hash
    // and ensure it follows the MuSig2/FROST protocol

    return signature.size() == 64; // Placeholder - proper validation needed
}

bool ElderfierConsensusService::aggregateSignatures(ProofRequest& request) {
    if (request.selected_elders.empty()) {
        return false;
    }

    // Generate aggregate public key for the selected Eldernodes
    Crypto::PublicKey aggregateKey = ThresholdSignature::aggregatePublicKeys(request.selected_elders);

    // Create message hash for signing (burn_tx_hash + consensus_path)
    Crypto::Hash message;
    std::vector<uint8_t> messageData;
    messageData.insert(messageData.end(), request.burn_tx_hash.data, request.burn_tx_hash.data + 32);
    messageData.push_back(static_cast<uint8_t>(request.path));
    message = Crypto::cn_fast_hash(messageData.data(), messageData.size());

    // Aggregate the partial signatures
    std::vector<std::vector<uint8_t>> partialSignatures;
    // In a real implementation, we'd collect actual partial signatures
    // For now, we'll simulate this with placeholder signatures

    for (size_t i = 0; i < request.selected_elders.size(); ++i) {
        // Generate placeholder partial signature
        std::vector<uint8_t> partialSig = ThresholdSignature::generatePartialSignature(
            message, Crypto::SecretKey(), aggregateKey);
        partialSignatures.push_back(partialSig);
    }

    // Aggregate signatures into threshold signature
    request.aggregated_signature = ThresholdSignature::aggregateSignatures(
        partialSignatures, request.selected_elders, aggregateKey);

    return !request.aggregated_signature.empty();
}

void ElderfierConsensusService::processConsensusTimeout(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end()) {
        return;
    }

    auto& request = it->second;

    // Check timeout based on consensus path
    uint32_t timeout_seconds = 0;
    switch (request.path) {
        case ConsensusPath::FAST_PASS:
            timeout_seconds = m_fast_pass_timeout;
            break;
        case ConsensusPath::FALLBACK:
            timeout_seconds = m_fallback_timeout;
            break;
        case ConsensusPath::COUNCIL_REVIEW:
            timeout_seconds = m_council_timeout;
            break;
    }

    uint64_t elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - request.start_time;

    if (elapsed >= timeout_seconds) {
        m_logger(Logging::WARNING) << "Consensus timeout for " << Common::podToHex(burn_tx_hash)
                                  << " after " << elapsed << " seconds";

        // Escalate to next path or council
        if (request.path == ConsensusPath::FAST_PASS) {
            // FastPass timed out, escalate to Fallback
            escalateToFallback(burn_tx_hash);
        } else if (request.path == ConsensusPath::FALLBACK) {
            // Fallback timed out - record failure and escalate to Council Review
            recordConsensusFailure(createFailureDetail(request, "TIMEOUT"));
            request.status = ConsensusStatus::FAILED;
            escalateToCouncil(burn_tx_hash);
        }
    }
}

void ElderfierConsensusService::escalateToFallback(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end()) {
        return;
    }

    auto& request = it->second;

    // Select new Eldernode quorum for Fallback (8 nodes, 6 required)
    auto selected_elders = selectEldernodeQuorum(burn_tx_hash, ConsensusPath::FALLBACK);
    if (selected_elders.empty()) {
        m_logger(Logging::ERROR) << "Failed to select Fallback Eldernode quorum for " << Common::podToHex(burn_tx_hash);
        escalateToCouncil(burn_tx_hash);
        return;
    }

    request.path = ConsensusPath::FALLBACK;
    request.selected_elders = selected_elders;
    request.signatures_received = 0;
    request.start_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    m_logger(Logging::INFO) << "Escalating " << Common::podToHex(burn_tx_hash)
                           << " from FastPass to Fallback with " << selected_elders.size() << " Eldernodes";
}

void ElderfierConsensusService::escalateToCouncil(const Crypto::Hash& burn_tx_hash) {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    auto it = m_active_proofs.find(burn_tx_hash);
    if (it == m_active_proofs.end()) {
        return;
    }

    auto& request = it->second;
    request.status = ConsensusStatus::FAILED;
    request.path = ConsensusPath::COUNCIL_REVIEW;

    // Record the failure for council review
    if (!request.failure_history.empty()) {
        const auto& latest_failure = request.failure_history.back();

        m_logger(Logging::INFO) << "Consensus failed for " << Common::podToHex(burn_tx_hash)
                               << " - escalating to Elder Council review";
        m_logger(Logging::INFO) << "Failure details: " << latest_failure.failure_reason
                               << ", signatures received: " << latest_failure.signatures_received
                               << ", non-responding nodes: " << latest_failure.non_responding_elders.size();
    }
}

void ElderfierConsensusService::cleanupExpiredProofs() {
    std::lock_guard<std::mutex> lock(m_proofs_mutex);

    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto it = m_active_proofs.begin();
    while (it != m_active_proofs.end()) {
        uint64_t elapsed = current_time - it->second.start_time;

        // Clean up very old proofs (24 hours)
        if (elapsed > 86400) {
            m_logger(Logging::TRACE) << "Cleaning up expired proof request: " << Common::podToHex(it->first);
            it = m_active_proofs.erase(it);
        } else {
            ++it;
        }
    }
}

void ElderfierConsensusService::timerLoop() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(30)); // Check every 30 seconds

        if (!m_running) break;

        // Process timeouts for all active proofs
        {
            std::lock_guard<std::mutex> lock(m_proofs_mutex);
            for (const auto& pair : m_active_proofs) {
                if (pair.second.status == ConsensusStatus::PENDING ||
                    pair.second.status == ConsensusStatus::IN_PROGRESS) {
                    processConsensusTimeout(pair.first);
                }
            }
        }

        cleanupExpiredProofs();
    }
}

} // namespace CryptoNote
