#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <chrono>
#include <thread>
#include <atomic>
#include "EldernodeIndexTypes.h"
#include "Logging/ILogger.h"
#include "Logging/LoggerRef.h"
#include "IBlockchainExplorer.h"
#include "CryptoNoteCore/DepositIndex.h"

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
    
    // Elderfier deposit management
    virtual bool addElderfierDeposit(const ElderfierDepositData& deposit) = 0;
    virtual bool verifyElderfierDeposit(const ElderfierDepositData& deposit) const = 0;
    virtual ElderfierDepositData getElderfierDeposit(const Crypto::PublicKey& publicKey) const = 0;
    
    // Elderfier deposit monitoring and ENindex management
    virtual bool monitorElderfierDeposits() = 0;
    virtual bool validateElderfierForENindex(const Crypto::PublicKey& publicKey) const = 0;
    virtual bool addElderfierToENindex(const ElderfierDepositData& deposit) = 0;
    virtual bool removeElderfierFromENindex(const Crypto::PublicKey& publicKey) = 0;
    virtual std::vector<ElderfierDepositData> getValidElderfierDeposits() const = 0;
    
    // Security window management
    virtual bool updateElderfierSignature(const Crypto::PublicKey& publicKey, uint64_t timestamp) = 0;
    virtual bool canElderfierUnlock(const Crypto::PublicKey& publicKey) const = 0;
    virtual uint64_t getSecurityWindowRemaining(const Crypto::PublicKey& publicKey) const = 0;
    virtual bool isElderfierInSecurityWindow(const Crypto::PublicKey& publicKey) const = 0;
    virtual bool requestElderfierUnlock(const Crypto::PublicKey& publicKey, uint64_t timestamp) = 0;
    virtual bool processElderfierUnlock(const Crypto::PublicKey& publicKey) = 0;
    
    // Mempool buffer security window
    virtual bool processSpendingTransaction(const Crypto::Hash& transactionHash, const Crypto::PublicKey& elderfierPublicKey) = 0;
    virtual bool validateLastSignature(const Crypto::PublicKey& elderfierPublicKey) const = 0;
    virtual std::vector<MempoolSecurityWindow> getPendingSpendingTransactions() const = 0;
    virtual bool releaseTransaction(const Crypto::Hash& transactionHash) = 0;
    
    // Elder Council voting system
    virtual bool submitElderCouncilVote(const ElderCouncilVote& vote) = 0;
    virtual std::vector<ElderCouncilVote> getElderCouncilVotes(const Crypto::PublicKey& targetPublicKey) const = 0;
    virtual bool hasElderCouncilQuorum(const Crypto::PublicKey& targetPublicKey) const = 0;
    virtual bool canElderfierVote(const Crypto::PublicKey& voterPublicKey) const = 0;
    
    // Elderfier voting interface (email inbox system)
    virtual bool createVotingMessage(const MisbehaviorEvidence& evidence) = 0;
    virtual std::vector<ElderCouncilVotingMessage> getVotingMessages(const Crypto::PublicKey& elderfierPublicKey) const = 0;
    virtual std::vector<ElderCouncilVotingMessage> getUnreadVotingMessages(const Crypto::PublicKey& elderfierPublicKey) const = 0;
    virtual bool markMessageAsRead(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) = 0;
    virtual bool submitVoteOnMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey, ElderCouncilVoteType voteType) = 0;
    virtual bool confirmVoteOnMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) = 0;
    virtual bool cancelPendingVote(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) = 0;
    virtual ElderCouncilVoteType getPendingVoteType(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) const = 0;
    virtual bool hasPendingVote(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) const = 0;
    virtual ElderCouncilVotingMessage getVotingMessage(const Crypto::Hash& messageId) const = 0;
    virtual bool deleteVotingMessage(const Crypto::Hash& messageId) = 0;
    virtual std::vector<ElderCouncilVotingMessage> getActiveVotingMessages() const = 0;
    
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
    ~EldernodeIndexManager() override;
    
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
    
    // Elderfier deposit management
    bool addElderfierDeposit(const ElderfierDepositData& deposit) override;
    bool verifyElderfierDeposit(const ElderfierDepositData& deposit) const override;
    ElderfierDepositData getElderfierDeposit(const Crypto::PublicKey& publicKey) const override;
    
    // Elderfier deposit monitoring and ENindex management
    bool monitorElderfierDeposits() override;
    bool validateElderfierForENindex(const Crypto::PublicKey& publicKey) const override;
    bool addElderfierToENindex(const ElderfierDepositData& deposit) override;
    bool removeElderfierFromENindex(const Crypto::PublicKey& publicKey) override;
    std::vector<ElderfierDepositData> getValidElderfierDeposits() const override;
    
    // Security window management
    bool updateElderfierSignature(const Crypto::PublicKey& publicKey, uint64_t timestamp) override;
    bool canElderfierUnlock(const Crypto::PublicKey& publicKey) const override;
    uint64_t getSecurityWindowRemaining(const Crypto::PublicKey& publicKey) const override;
    bool isElderfierInSecurityWindow(const Crypto::PublicKey& publicKey) const override;
    bool requestElderfierUnlock(const Crypto::PublicKey& publicKey, uint64_t timestamp) override;
    bool processElderfierUnlock(const Crypto::PublicKey& publicKey) override;
    
    // Mempool buffer security window
    bool processSpendingTransaction(const Crypto::Hash& transactionHash, const Crypto::PublicKey& elderfierPublicKey) override;
    bool validateLastSignature(const Crypto::PublicKey& elderfierPublicKey) const override;
    std::vector<MempoolSecurityWindow> getPendingSpendingTransactions() const override;
    bool releaseTransaction(const Crypto::Hash& transactionHash) override;
    
    // Elder Council voting system
    bool submitElderCouncilVote(const ElderCouncilVote& vote) override;
    std::vector<ElderCouncilVote> getElderCouncilVotes(const Crypto::PublicKey& targetPublicKey) const override;
    bool hasElderCouncilQuorum(const Crypto::PublicKey& targetPublicKey) const override;
    bool canElderfierVote(const Crypto::PublicKey& voterPublicKey) const override;
    
    // Elderfier voting interface (email inbox system)
    bool createVotingMessage(const MisbehaviorEvidence& evidence) override;
    std::vector<ElderCouncilVotingMessage> getVotingMessages(const Crypto::PublicKey& elderfierPublicKey) const override;
    std::vector<ElderCouncilVotingMessage> getUnreadVotingMessages(const Crypto::PublicKey& elderfierPublicKey) const override;
    bool markMessageAsRead(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) override;
    bool submitVoteOnMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey, ElderCouncilVoteType voteType) override;
    bool confirmVoteOnMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) override;
    bool cancelPendingVote(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) override;
    ElderCouncilVoteType getPendingVoteType(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) const override;
    bool hasPendingVote(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) const override;
    ElderCouncilVotingMessage getVotingMessage(const Crypto::Hash& messageId) const override;
    bool deleteVotingMessage(const Crypto::Hash& messageId) override;
    std::vector<ElderCouncilVotingMessage> getActiveVotingMessages() const override;
    
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
    
    // Monitoring configuration
    void setMonitoringConfig(const ElderfierMonitoringConfig& config);
    ElderfierMonitoringConfig getMonitoringConfig() const;
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoringActive() const;
    
    // Blockchain integration
    void setBlockchainExplorer(IBlockchainExplorer* explorer);
    void setDepositIndex(const DepositIndex* depositIndex);
    
    // Auto-generation of fresh proofs
    bool generateFreshProof(const Crypto::PublicKey& publicKey, const std::string& feeAddress);
    bool regenerateAllProofs();
    
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
    
    // Elderfier deposit tracking
    std::unordered_map<Crypto::PublicKey, ElderfierDepositData> m_elderfierDeposits;
    std::unordered_map<std::string, Crypto::PublicKey> m_addressToPublicKey;
    
    // Blockchain integration
    IBlockchainExplorer* m_blockchainExplorer;
    const DepositIndex* m_depositIndex;
    
    // Monitoring configuration and thread
    ElderfierMonitoringConfig m_monitoringConfig;
    std::thread m_monitoringThread;
    std::atomic<bool> m_monitoringActive;
    std::atomic<bool> m_shouldStopMonitoring;
    
    // Mempool buffer security window
    std::unordered_map<Crypto::Hash, MempoolSecurityWindow> m_mempoolBuffer;
    std::unordered_map<Crypto::PublicKey, std::vector<ElderCouncilVote>> m_elderCouncilVotes;
    
    // Elderfier voting interface (email inbox system)
    std::unordered_map<Crypto::Hash, ElderCouncilVotingMessage> m_votingMessages;
    std::unordered_map<Crypto::PublicKey, std::vector<Crypto::Hash>> m_elderfierMessageInbox;
    std::unordered_map<Crypto::PublicKey, std::vector<Crypto::Hash>> m_elderfierReadMessages;
    std::unordered_map<Crypto::PublicKey, std::unordered_map<Crypto::Hash, ElderCouncilVoteType>> m_pendingVotes;
    
    // Helper methods
    bool validateEldernodeEntry(const ENindexEntry& entry) const;
    bool validateStakeProof(const EldernodeStakeProof& proof) const;
    bool validateElderfierServiceId(const ElderfierServiceId& serviceId) const;
    bool hasServiceIdConflict(const ElderfierServiceId& serviceId, const Crypto::PublicKey& excludeKey) const;
    Crypto::Hash calculateStakeHash(const Crypto::PublicKey& publicKey, uint64_t amount, uint64_t timestamp) const;
    std::vector<uint8_t> aggregateSignatures(const std::vector<std::vector<uint8_t>>& signatures) const;
    void redistributeSlashedStake(uint64_t slashedAmount);
    
    // Elderfier deposit monitoring helpers
    bool checkDepositSpending(const Crypto::PublicKey& publicKey) const;
    bool isDepositStillValid(const Crypto::PublicKey& publicKey) const;
    bool checkIfDepositOutputsSpent(const Crypto::Hash& depositHash) const;
    ENindexEntry createENindexEntryFromDeposit(const ElderfierDepositData& deposit) const;
    
    // Security window helpers
    void updateSecurityWindowForDeposit(ElderfierDepositData& deposit, uint64_t currentTimestamp) const;
    bool validateSignatureTimestamp(uint64_t lastSignature, uint64_t currentTimestamp) const;
    uint64_t calculateSecurityWindowEnd(uint64_t lastSignatureTimestamp) const;
    bool validateUnlockRequest(const ElderfierDepositData& deposit, uint64_t timestamp) const;
    
    // Blockchain integration helpers
    bool isOutputSpent(uint32_t globalIndex) const;
    bool isDepositTransactionSpent(const Crypto::Hash& depositHash) const;
    std::vector<uint32_t> getDepositOutputIndices(const Crypto::Hash& depositHash) const;
    
    // Mempool buffer helpers
    bool isTransactionInBuffer(const Crypto::Hash& transactionHash) const;
    void addTransactionToBuffer(const Crypto::Hash& transactionHash, const Crypto::PublicKey& elderfierPublicKey);
    void removeTransactionFromBuffer(const Crypto::Hash& transactionHash);
    bool shouldRequireElderCouncilVote(const Crypto::PublicKey& elderfierPublicKey) const;
    
    // Elder Council voting helpers
    bool validateVote(const ElderCouncilVote& vote) const;
    bool hasVoted(const Crypto::PublicKey& voterPublicKey, const Crypto::PublicKey& targetPublicKey) const;
    uint32_t calculateRequiredQuorum() const;
    
    // Elderfier voting interface helpers
    Crypto::Hash generateMessageId(const MisbehaviorEvidence& evidence) const;
    std::string generateVotingSubject(const MisbehaviorEvidence& evidence) const;
    std::string generateVotingDescription(const MisbehaviorEvidence& evidence) const;
    bool addMessageToElderfierInbox(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey);
    bool removeMessageFromElderfierInbox(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey);
    bool isMessageInElderfierInbox(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) const;
    bool hasElderfierReadMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) const;
    std::string getVoteTypeDescription(ElderCouncilVoteType voteType) const;
    std::string generateVoteConfirmationMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey, ElderCouncilVoteType voteType) const;
};

} // namespace CryptoNote
