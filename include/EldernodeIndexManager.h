#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <chrono>
#include "EldernodeIndexTypes.h"
#include "Logging/ILogger.h"
#include "Logging/LoggerRef.h"

namespace CryptoNote {

class IEldernodeIndexManager {
public:
    virtual ~IEldernodeIndexManager() = default;
    
    // Core ENindex management
    virtual bool addEldernode(const ENindexEntry& entry) = 0;
    virtual bool removeEldernode(const Crypto::PublicKey& publicKey) = 0;
    virtual bool updateEldernode(const ENindexEntry& entry) = 0;
    virtual std::optional<ENindexEntry> getEldernode(const Crypto::PublicKey& publicKey) const = 0;
    virtual std::vector<ENindexEntry> getAllEldernodes() const = 0;
    virtual std::vector<ENindexEntry> getActiveEldernodes() const = 0;
    
    // Elderfier-specific management
    virtual std::vector<ENindexEntry> getElderfierNodes() const = 0;
    virtual std::optional<ENindexEntry> getEldernodeByServiceId(const ElderfierServiceId& serviceId) const = 0;
    
    // Stake proof management
    virtual bool addStakeProof(const EldernodeStakeProof& proof) = 0;
    virtual bool verifyStakeProof(const EldernodeStakeProof& proof) const = 0;
    virtual std::vector<EldernodeStakeProof> getStakeProofs(const Crypto::PublicKey& publicKey) const = 0;
    
    // Consensus management
    virtual bool addConsensusParticipant(const EldernodeConsensusParticipant& participant) = 0;
    virtual bool removeConsensusParticipant(const Crypto::PublicKey& publicKey) = 0;
    virtual std::vector<EldernodeConsensusParticipant> getConsensusParticipants() const = 0;
    virtual EldernodeConsensusResult reachConsensus(const std::vector<uint8_t>& data, const ConsensusThresholds& thresholds) = 0;
    
    // Statistics and monitoring
    virtual uint32_t getTotalEldernodeCount() const = 0;
    virtual uint32_t getActiveEldernodeCount() const = 0;
    virtual uint32_t getElderfierNodeCount() const = 0;
    virtual uint64_t getTotalStakeAmount() const = 0;
    virtual std::chrono::system_clock::time_point getLastUpdate() const = 0;
    
    // Persistence
    virtual bool saveToStorage() = 0;
    virtual bool loadFromStorage() = 0;
    virtual bool clearStorage() = 0;
    
    // Slashing functionality
    virtual bool slashEldernode(const Crypto::PublicKey& publicKey, const std::string& reason) = 0;
};

class EldernodeIndexManager : public IEldernodeIndexManager {
public:
    EldernodeIndexManager(Logging::ILogger& logger);
    ~EldernodeIndexManager() override = default;
    
    // Core ENindex management
    bool addEldernode(const ENindexEntry& entry) override;
    bool removeEldernode(const Crypto::PublicKey& publicKey) override;
    bool updateEldernode(const ENindexEntry& entry) override;
    std::optional<ENindexEntry> getEldernode(const Crypto::PublicKey& publicKey) const override;
    std::vector<ENindexEntry> getAllEldernodes() const override;
    std::vector<ENindexEntry> getActiveEldernodes() const override;
    
    // Elderfier-specific management
    std::vector<ENindexEntry> getElderfierNodes() const override;
    std::optional<ENindexEntry> getEldernodeByServiceId(const ElderfierServiceId& serviceId) const override;
    
    // Stake proof management
    bool addStakeProof(const EldernodeStakeProof& proof) override;
    bool verifyStakeProof(const EldernodeStakeProof& proof) const override;
    std::vector<EldernodeStakeProof> getStakeProofs(const Crypto::PublicKey& publicKey) const override;
    
    // Consensus management
    bool addConsensusParticipant(const EldernodeConsensusParticipant& participant) override;
    bool removeConsensusParticipant(const Crypto::PublicKey& publicKey) override;
    std::vector<EldernodeConsensusParticipant> getConsensusParticipants() const override;
    EldernodeConsensusResult reachConsensus(const std::vector<uint8_t>& data, const ConsensusThresholds& thresholds) override;
    
    // Statistics and monitoring
    uint32_t getTotalEldernodeCount() const override;
    uint32_t getActiveEldernodeCount() const override;
    uint32_t getElderfierNodeCount() const override;
    uint64_t getTotalStakeAmount() const override;
    std::chrono::system_clock::time_point getLastUpdate() const override;
    
    // Persistence
    bool saveToStorage() override;
    bool loadFromStorage() override;
    bool clearStorage() override;
    
    // Configuration
    void setConsensusThresholds(const ConsensusThresholds& thresholds);
    ConsensusThresholds getConsensusThresholds() const;
    void setElderfierConfig(const ElderfierServiceConfig& config);
    ElderfierServiceConfig getElderfierConfig() const;
    
    // Auto-generation of fresh proofs
    bool generateFreshProof(const Crypto::PublicKey& publicKey, const std::string& feeAddress);
    bool regenerateAllProofs();
    
    // Constant stake proof management for cross-chain validation
    bool createConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                  ConstantStakeProofType proofType,
                                  const std::string& crossChainAddress,
                                  uint64_t stakeAmount);
    bool renewConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                 ConstantStakeProofType proofType);
    bool revokeConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                 ConstantStakeProofType proofType);
    std::vector<EldernodeStakeProof> getConstantStakeProofs(const Crypto::PublicKey& publicKey) const;
    std::vector<EldernodeStakeProof> getConstantStakeProofsByType(ConstantStakeProofType proofType) const;
    
    // Slashing functionality
    bool slashEldernode(const Crypto::PublicKey& publicKey, const std::string& reason) override;

private:
    Logging::LoggerRef logger;
    mutable std::mutex m_mutex;
    std::unordered_map<Crypto::PublicKey, ENindexEntry> m_eldernodes;
    std::unordered_map<Crypto::PublicKey, std::vector<EldernodeStakeProof>> m_stakeProofs;
    std::unordered_map<Crypto::PublicKey, EldernodeConsensusParticipant> m_consensusParticipants;
    ConsensusThresholds m_consensusThresholds;
    ElderfierServiceConfig m_elderfierConfig;
    std::chrono::system_clock::time_point m_lastUpdate;
    
    // Helper methods
    bool validateEldernodeEntry(const ENindexEntry& entry) const;
    bool validateStakeProof(const EldernodeStakeProof& proof) const;
    bool validateElderfierServiceId(const ElderfierServiceId& serviceId) const;
    bool hasServiceIdConflict(const ElderfierServiceId& serviceId, const Crypto::PublicKey& excludeKey) const;
    Crypto::Hash calculateStakeHash(const Crypto::PublicKey& publicKey, uint64_t amount, uint64_t timestamp) const;
    std::vector<uint8_t> aggregateSignatures(const std::vector<std::vector<uint8_t>>& signatures) const;
    void redistributeSlashedStake(uint64_t slashedAmount);
};

} // namespace CryptoNote
