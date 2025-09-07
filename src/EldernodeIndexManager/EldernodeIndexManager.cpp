#include "EldernodeIndexManager.h"
#include "Common/StringTools.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "Logging/LoggerRef.h"
#include "IBlockchainExplorer.h"
#include "CryptoNoteCore/DepositIndex.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

using namespace Logging;

namespace CryptoNote {

EldernodeIndexManager::EldernodeIndexManager(Logging::ILogger& log)
    : logger(log, "EldernodeIndexManager")
    , m_consensusThresholds(ConsensusThresholds::getDefault())
    , m_elderfierConfig(ElderfierServiceConfig::getDefault())
    , m_lastUpdate(std::chrono::system_clock::now())
    , m_blockchainExplorer(nullptr)
    , m_depositIndex(nullptr)
    , m_monitoringConfig(ElderfierMonitoringConfig::getDefault())
    , m_monitoringActive(false)
    , m_shouldStopMonitoring(false) {
}

EldernodeIndexManager::~EldernodeIndexManager() {
    // Stop monitoring thread if active
    if (m_monitoringActive) {
        stopMonitoring();
    }
}

bool EldernodeIndexManager::addEldernode(const ENindexEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateEldernodeEntry(entry)) {
        logger(ERROR) << "Invalid Eldernode entry for public key: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    auto it = m_eldernodes.find(entry.eldernodePublicKey);
    if (it != m_eldernodes.end()) {
        logger(WARNING) << "Eldernode already exists: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    // Check for service ID conflicts for Elderfier nodes
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (!validateElderfierServiceId(entry.serviceId)) {
            logger(ERROR) << "Invalid Elderfier service ID for public key: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        if (hasServiceIdConflict(entry.serviceId, entry.eldernodePublicKey)) {
            logger(ERROR) << "Service ID conflict for Elderfier node: " << entry.serviceId.toString();
            return false;
        }
        
        // Verify that linked address matches fee address for all Elderfier types
        if (entry.serviceId.linkedAddress != entry.feeAddress) {
            logger(ERROR) << "Linked address mismatch for Elderfier node: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        // Verify that hashed address is present for all Elderfier nodes
        if (entry.serviceId.hashedAddress.empty()) {
            logger(ERROR) << "Missing hashed address for Elderfier node: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
    }
    
    m_eldernodes[entry.eldernodePublicKey] = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (entry.tier == EldernodeTier::ELDERFIER) ? "Basic" : "Elderfier";
    logger(INFO) << "Added " << tierName << " Eldernode: " << Common::podToHex(entry.eldernodePublicKey) 
                << " with stake: " << entry.stakeAmount;
    
    if (entry.tier == EldernodeTier::ELDERFIER) {
        logger(INFO) << "Elderfier service ID: " << entry.serviceId.toString();
    }
    
    return true;
}

bool EldernodeIndexManager::removeEldernode(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(WARNING) << "Eldernode not found for removal: " << Common::podToHex(publicKey);
        return false;
    }
    
    std::string tierName = (it->second.tier == EldernodeTier::ELDERFIER) ? "Basic" : "Elderfier";
    logger(INFO) << "Removing " << tierName << " Eldernode: " << Common::podToHex(publicKey);
    
    m_eldernodes.erase(it);
    m_stakeProofs.erase(publicKey);
    m_consensusParticipants.erase(publicKey);
    m_lastUpdate = std::chrono::system_clock::now();
    
    return true;
}

bool EldernodeIndexManager::updateEldernode(const ENindexEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateEldernodeEntry(entry)) {
        logger(ERROR) << "Invalid Eldernode entry for update: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    auto it = m_eldernodes.find(entry.eldernodePublicKey);
    if (it == m_eldernodes.end()) {
        logger(WARNING) << "Eldernode not found for update: " << Common::podToHex(entry.eldernodePublicKey);
        return false;
    }
    
    // Check for service ID conflicts for Elderfier nodes (excluding self)
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (!validateElderfierServiceId(entry.serviceId)) {
            logger(ERROR) << "Invalid Elderfier service ID for update: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        if (hasServiceIdConflict(entry.serviceId, entry.eldernodePublicKey)) {
            logger(ERROR) << "Service ID conflict for Elderfier update: " << entry.serviceId.toString();
            return false;
        }
        
        // Verify that linked address matches fee address for all Elderfier types
        if (entry.serviceId.linkedAddress != entry.feeAddress) {
            logger(ERROR) << "Linked address mismatch for Elderfier update: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
        
        // Verify that hashed address is present for all Elderfier nodes
        if (entry.serviceId.hashedAddress.empty()) {
            logger(ERROR) << "Missing hashed address for Elderfier update: " << Common::podToHex(entry.eldernodePublicKey);
            return false;
        }
    }
    
    it->second = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (entry.tier == EldernodeTier::ELDERFIER) ? "Basic" : "Elderfier";
    logger(INFO) << "Updated " << tierName << " Eldernode: " << Common::podToHex(entry.eldernodePublicKey);
    
    return true;
}

std::optional<ENindexEntry> EldernodeIndexManager::getEldernode(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::vector<ENindexEntry> EldernodeIndexManager::getAllEldernodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ENindexEntry> result;
    result.reserve(m_eldernodes.size());
    
    for (const auto& pair : m_eldernodes) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<ENindexEntry> EldernodeIndexManager::getActiveEldernodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ENindexEntry> result;
    result.reserve(m_eldernodes.size());
    
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<ENindexEntry> EldernodeIndexManager::getElderfierNodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ENindexEntry> result;
    result.reserve(m_eldernodes.size());
    
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::optional<ENindexEntry> EldernodeIndexManager::getEldernodeByServiceId(const ElderfierServiceId& serviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && 
            pair.second.serviceId.identifier == serviceId.identifier) {
            return pair.second;
        }
    }
    
    return std::nullopt;
}

bool EldernodeIndexManager::addElderfierDeposit(const ElderfierDepositData& deposit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!deposit.isValid()) {
        logger(ERROR) << "Invalid Elderfier deposit for public key: " << Common::podToHex(deposit.elderfierPublicKey);
        return false;
    }
    
    m_elderfierDeposits[deposit.elderfierPublicKey] = deposit;
    m_addressToPublicKey[deposit.elderfierAddress] = deposit.elderfierPublicKey;
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Added Elderfier deposit for: " << Common::podToHex(deposit.elderfierPublicKey)
                << " amount: " << deposit.depositAmount
                << " address: " << deposit.elderfierAddress;
    
    logger(INFO) << "Elderfier service ID: " << deposit.serviceId.toString();
    
    return true;
}

bool EldernodeIndexManager::verifyElderfierDeposit(const ElderfierDepositData& deposit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return deposit.isValid();
}

ElderfierDepositData EldernodeIndexManager::getElderfierDeposit(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        ElderfierDepositData invalidDeposit;
        return invalidDeposit;
    }
    
    return it->second;
}

// Elderfier deposit monitoring and ENindex management

bool EldernodeIndexManager::monitorElderfierDeposits() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    bool changesMade = false;
    std::vector<Crypto::PublicKey> toRemove;
    
    // Check all Elderfier deposits for validity
    for (const auto& pair : m_elderfierDeposits) {
        const Crypto::PublicKey& publicKey = pair.first;
        const ElderfierDepositData& deposit = pair.second;
        
        // Check if deposit is still valid (not spent)
        if (!isDepositStillValid(publicKey)) {
            logger(WARNING) << "Elderfier deposit no longer valid (spent): " 
                            << Common::podToHex(publicKey);
            
            // Remove from ENindex
            if (removeElderfierFromENindex(publicKey)) {
                changesMade = true;
            }
            
            // Mark for removal from deposits
            toRemove.push_back(publicKey);
        } else {
            // Update security window status
            uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            updateSecurityWindowForDeposit(const_cast<ElderfierDepositData&>(deposit), currentTimestamp);
            
            // Check if Elderfier should be in ENindex but isn't
            auto eldernodeIt = m_eldernodes.find(publicKey);
            if (eldernodeIt == m_eldernodes.end()) {
                // Add to ENindex
                if (addElderfierToENindex(deposit)) {
                    changesMade = true;
                }
            } else {
                // Update existing ENindex entry if needed
                if (!eldernodeIt->second.isActive) {
                    eldernodeIt->second.isActive = true;
                    changesMade = true;
                    logger(INFO) << "Reactivated Elderfier in ENindex: " 
                                  << Common::podToHex(publicKey);
                }
            }
        }
    }
    
    // Remove invalid deposits
    for (const auto& publicKey : toRemove) {
        m_elderfierDeposits.erase(publicKey);
        changesMade = true;
    }
    
    if (changesMade) {
        m_lastUpdate = std::chrono::system_clock::now();
        logger(INFO) << "Elderfier deposit monitoring completed with changes";
    }
    
    return changesMade;
}

bool EldernodeIndexManager::validateElderfierForENindex(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if deposit exists and is valid
    auto depositIt = m_elderfierDeposits.find(publicKey);
    if (depositIt == m_elderfierDeposits.end()) {
        return false;
    }
    
    const ElderfierDepositData& deposit = depositIt->second;
    
    // Validate deposit requirements
    if (!deposit.isValid() || deposit.isSpent || !deposit.isActive) {
        return false;
    }
    
    // Check minimum deposit amount
    if (deposit.depositAmount < m_elderfierConfig.minimumStakeAmount) {
        return false;
    }
    
    // Check if deposit is still valid (not spent)
    if (!isDepositStillValid(publicKey)) {
        return false;
    }
    
    return true;
}

bool EldernodeIndexManager::addElderfierToENindex(const ElderfierDepositData& deposit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!validateElderfierForENindex(deposit.elderfierPublicKey)) {
        logger(ERROR) << "Cannot add invalid Elderfier to ENindex: " 
                      << Common::podToHex(deposit.elderfierPublicKey);
        return false;
    }
    
    // Create ENindex entry from deposit
    ENindexEntry entry = createENindexEntryFromDeposit(deposit);
    
    // Add to ENindex
    m_eldernodes[deposit.elderfierPublicKey] = entry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Added Elderfier to ENindex: " 
                << Common::podToHex(deposit.elderfierPublicKey)
                << " deposit: " << deposit.depositAmount
                << " address: " << deposit.elderfierAddress;
    
    return true;
}

bool EldernodeIndexManager::removeElderfierFromENindex(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(WARNING) << "Elderfier not found in ENindex for removal: " 
                        << Common::podToHex(publicKey);
        return false;
    }
    
    // Only remove Elderfier tier nodes
    if (it->second.tier != EldernodeTier::ELDERFIER) {
        logger(WARNING) << "Cannot remove non-Elderfier node: " 
                        << Common::podToHex(publicKey);
        return false;
    }
    
    // Remove from ENindex
    m_eldernodes.erase(it);
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Removed Elderfier from ENindex: " 
                << Common::podToHex(publicKey);
    
    return true;
}

std::vector<ElderfierDepositData> EldernodeIndexManager::getValidElderfierDeposits() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ElderfierDepositData> validDeposits;
    
    for (const auto& pair : m_elderfierDeposits) {
        const ElderfierDepositData& deposit = pair.second;
        
        // Only include valid, active, non-spent deposits
        if (deposit.isValid() && deposit.isActive && !deposit.isSpent) {
            // Double-check deposit is still valid
            if (isDepositStillValid(deposit.elderfierPublicKey)) {
                validDeposits.push_back(deposit);
            }
        }
    }
    
    return validDeposits;
}

// Security window management

bool EldernodeIndexManager::updateElderfierSignature(const Crypto::PublicKey& publicKey, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        logger(ERROR) << "Elderfier deposit not found for signature update: " 
                      << Common::podToHex(publicKey);
        return false;
    }
    
    ElderfierDepositData& deposit = it->second;
    
    // Validate signature timestamp (prevent spam)
    if (!validateSignatureTimestamp(deposit.lastSignatureTimestamp, timestamp)) {
        logger(WARNING) << "Invalid signature timestamp for Elderfier: " 
                        << Common::podToHex(publicKey);
        return false;
    }
    
    // Update signature timestamp
    deposit.updateLastSignature(timestamp);
    
    // Update security window
    updateSecurityWindowForDeposit(deposit, timestamp);
    
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Updated Elderfier signature: " 
                << Common::podToHex(publicKey)
                << " timestamp: " << timestamp
                << " security window ends: " << deposit.securityWindowEnd;
    
    return true;
}

bool EldernodeIndexManager::canElderfierUnlock(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        return false;
    }
    
    const ElderfierDepositData& deposit = it->second;
    
    // Check if deposit can be unlocked (outside security window)
    return deposit.canUnlock();
}

uint64_t EldernodeIndexManager::getSecurityWindowRemaining(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        return 0;
    }
    
    const ElderfierDepositData& deposit = it->second;
    
    // Get remaining time in security window
    return deposit.getSecurityWindowRemaining();
}

bool EldernodeIndexManager::isElderfierInSecurityWindow(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        return false;
    }
    
    const ElderfierDepositData& deposit = it->second;
    
    // Check if Elderfier is currently in security window
    return deposit.isInSecurityWindow;
}

bool EldernodeIndexManager::requestElderfierUnlock(const Crypto::PublicKey& publicKey, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        logger(ERROR) << "Elderfier deposit not found for unlock request: " 
                      << Common::podToHex(publicKey);
        return false;
    }
    
    ElderfierDepositData& deposit = it->second;
    
    // Validate unlock request
    if (!validateUnlockRequest(deposit, timestamp)) {
        logger(WARNING) << "Invalid unlock request for Elderfier: " 
                        << Common::podToHex(publicKey);
        return false;
    }
    
    // Request unlock
    deposit.requestUnlock(timestamp);
    
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Elderfier unlock requested: " 
                << Common::podToHex(publicKey)
                << " timestamp: " << timestamp
                << " security window ends: " << deposit.securityWindowEnd;
    
    return true;
}

bool EldernodeIndexManager::processElderfierUnlock(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        logger(ERROR) << "Elderfier deposit not found for unlock processing: " 
                      << Common::podToHex(publicKey);
        return false;
    }
    
    ElderfierDepositData& deposit = it->second;
    
    // Check if unlock can be processed
    if (!deposit.unlockRequested) {
        logger(WARNING) << "No unlock request pending for Elderfier: " 
                        << Common::podToHex(publicKey);
        return false;
    }
    
    // Check if security window has expired
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (currentTimestamp < deposit.securityWindowEnd) {
        logger(WARNING) << "Security window still active for Elderfier: " 
                        << Common::podToHex(publicKey)
                        << " remaining: " << (deposit.securityWindowEnd - currentTimestamp) << " seconds";
        return false;
    }
    
    // Process unlock - mark as unlocked and remove from ENindex
    deposit.isUnlocked = true;
    deposit.isActive = false;
    
    // Remove from ENindex
    removeElderfierFromENindex(publicKey);
    
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Elderfier unlock processed: " 
                << Common::podToHex(publicKey)
                << " deposit unlocked and removed from ENindex";
    
    return true;
}

// Mempool buffer security window

bool EldernodeIndexManager::processSpendingTransaction(const Crypto::Hash& transactionHash, const Crypto::PublicKey& elderfierPublicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if transaction is already in buffer
    if (isTransactionInBuffer(transactionHash)) {
        logger(WARNING) << "Transaction already in mempool buffer: " << Common::podToHex(transactionHash);
        return false;
    }
    
    // Validate Elderfier exists
    auto depositIt = m_elderfierDeposits.find(elderfierPublicKey);
    if (depositIt == m_elderfierDeposits.end()) {
        logger(ERROR) << "Elderfier not found for spending transaction: " << Common::podToHex(elderfierPublicKey);
        return false;
    }
    
    // Create mempool security window entry
    MempoolSecurityWindow securityWindow;
    securityWindow.transactionHash = transactionHash;
    securityWindow.elderfierPublicKey = elderfierPublicKey;
    securityWindow.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    securityWindow.securityWindowEnd = securityWindow.timestamp + m_monitoringConfig.mempoolBufferDuration;
    securityWindow.signatureValidated = false;
    securityWindow.elderCouncilVoteRequired = false;
    securityWindow.requiredVotes = m_monitoringConfig.elderCouncilQuorumSize;
    securityWindow.currentVotes = 0;
    
    // Validate last signature
    securityWindow.signatureValidated = validateLastSignature(elderfierPublicKey);
    
    // Determine if Elder Council vote is required
    securityWindow.elderCouncilVoteRequired = shouldRequireElderCouncilVote(elderfierPublicKey);
    
    // Add to mempool buffer
    m_mempoolBuffer[transactionHash] = securityWindow;
    
    logger(INFO) << "Added spending transaction to mempool buffer: " 
                << Common::podToHex(transactionHash)
                << " Elderfier: " << Common::podToHex(elderfierPublicKey)
                << " signature valid: " << securityWindow.signatureValidated
                << " council vote required: " << securityWindow.elderCouncilVoteRequired;
    
    return true;
}

bool EldernodeIndexManager::validateLastSignature(const Crypto::PublicKey& elderfierPublicKey) const {
    auto depositIt = m_elderfierDeposits.find(elderfierPublicKey);
    if (depositIt == m_elderfierDeposits.end()) {
        return false;
    }
    
    const ElderfierDepositData& deposit = depositIt->second;
    
    // Check if Elderfier has a recent valid signature
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Signature is valid if it's within the security window
    return deposit.lastSignatureTimestamp > 0 && 
           (currentTimestamp - deposit.lastSignatureTimestamp) < SecurityWindow::DEFAULT_DURATION_SECONDS;
}

std::vector<MempoolSecurityWindow> EldernodeIndexManager::getPendingSpendingTransactions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<MempoolSecurityWindow> pendingTransactions;
    for (const auto& pair : m_mempoolBuffer) {
        pendingTransactions.push_back(pair.second);
    }
    
    return pendingTransactions;
}

bool EldernodeIndexManager::releaseTransaction(const Crypto::Hash& transactionHash) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_mempoolBuffer.find(transactionHash);
    if (it == m_mempoolBuffer.end()) {
        logger(WARNING) << "Transaction not found in mempool buffer: " << Common::podToHex(transactionHash);
        return false;
    }
    
    const MempoolSecurityWindow& securityWindow = it->second;
    
    // Check if transaction can be released
    if (!securityWindow.canReleaseTransaction()) {
        logger(WARNING) << "Transaction cannot be released yet: " << Common::podToHex(transactionHash);
        return false;
    }
    
    // Remove from buffer
    m_mempoolBuffer.erase(it);
    
    logger(INFO) << "Released transaction from mempool buffer: " 
                << Common::podToHex(transactionHash)
                << " Elderfier: " << Common::podToHex(securityWindow.elderfierPublicKey);
    
    return true;
}

// Elderfier voting interface (email inbox system)

bool EldernodeIndexManager::createVotingMessage(const MisbehaviorEvidence& evidence) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!evidence.isValid()) {
        logger(ERROR) << "Invalid misbehavior evidence provided";
        return false;
    }
    
    // Generate unique message ID
    Crypto::Hash messageId = generateMessageId(evidence);
    
    // Check if message already exists
    if (m_votingMessages.find(messageId) != m_votingMessages.end()) {
        logger(WARNING) << "Voting message already exists: " << Common::podToHex(messageId);
        return false;
    }
    
    // Create voting message
    ElderCouncilVotingMessage message;
    message.messageId = messageId;
    message.targetElderfier = evidence.elderfierPublicKey;
    message.subject = generateVotingSubject(evidence);
    message.description = generateVotingDescription(evidence);
    message.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    message.votingDeadline = message.timestamp + m_monitoringConfig.votingWindowDuration;
    message.isRead = false;
    message.hasVoted = false;
    message.hasConfirmedVote = false;
    message.pendingVoteType = ElderCouncilVoteType::SLASH_ALL; // Default pending vote
    message.confirmedVoteType = ElderCouncilVoteType::SLASH_ALL; // Default confirmed vote
    message.requiredVotes = calculateRequiredQuorum();
    message.currentVotes = 0;
    
    // Add message to storage
    m_votingMessages[messageId] = message;
    
    // Add message to all active Elderfier inboxes
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            addMessageToElderfierInbox(messageId, pair.first);
        }
    }
    
    logger(INFO) << "Created Elder Council voting message: " 
                << Common::podToHex(messageId)
                << " for Elderfier: " << Common::podToHex(evidence.elderfierPublicKey)
                << " subject: " << message.subject;
    
    return true;
}

std::vector<ElderCouncilVotingMessage> EldernodeIndexManager::getVotingMessages(const Crypto::PublicKey& elderfierPublicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ElderCouncilVotingMessage> messages;
    auto inboxIt = m_elderfierMessageInbox.find(elderfierPublicKey);
    
    if (inboxIt != m_elderfierMessageInbox.end()) {
        for (const auto& messageId : inboxIt->second) {
            auto messageIt = m_votingMessages.find(messageId);
            if (messageIt != m_votingMessages.end()) {
                messages.push_back(messageIt->second);
            }
        }
    }
    
    // Sort by timestamp (newest first)
    std::sort(messages.begin(), messages.end(), 
              [](const ElderCouncilVotingMessage& a, const ElderCouncilVotingMessage& b) {
                  return a.timestamp > b.timestamp;
              });
    
    return messages;
}

std::vector<ElderCouncilVotingMessage> EldernodeIndexManager::getUnreadVotingMessages(const Crypto::PublicKey& elderfierPublicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ElderCouncilVotingMessage> unreadMessages;
    auto inboxIt = m_elderfierMessageInbox.find(elderfierPublicKey);
    
    if (inboxIt != m_elderfierMessageInbox.end()) {
        for (const auto& messageId : inboxIt->second) {
            auto messageIt = m_votingMessages.find(messageId);
            if (messageIt != m_votingMessages.end() && !messageIt->second.isRead) {
                unreadMessages.push_back(messageIt->second);
            }
        }
    }
    
    // Sort by timestamp (newest first)
    std::sort(unreadMessages.begin(), unreadMessages.end(), 
              [](const ElderCouncilVotingMessage& a, const ElderCouncilVotingMessage& b) {
                  return a.timestamp > b.timestamp;
              });
    
    return unreadMessages;
}

bool EldernodeIndexManager::markMessageAsRead(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto messageIt = m_votingMessages.find(messageId);
    if (messageIt == m_votingMessages.end()) {
        logger(WARNING) << "Voting message not found: " << Common::podToHex(messageId);
        return false;
    }
    
    // Check if Elderfier has this message in their inbox
    if (!isMessageInElderfierInbox(messageId, elderfierPublicKey)) {
        logger(WARNING) << "Elderfier does not have access to message: " << Common::podToHex(messageId);
        return false;
    }
    
    // Mark message as read
    messageIt->second.isRead = true;
    
    // Add to read messages list
    m_elderfierReadMessages[elderfierPublicKey].push_back(messageId);
    
    logger(INFO) << "Marked voting message as read: " 
                << Common::podToHex(messageId)
                << " by Elderfier: " << Common::podToHex(elderfierPublicKey);
    
    return true;
}

bool EldernodeIndexManager::submitVoteOnMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey, ElderCouncilVoteType voteType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto messageIt = m_votingMessages.find(messageId);
    if (messageIt == m_votingMessages.end()) {
        logger(WARNING) << "Voting message not found: " << Common::podToHex(messageId);
        return false;
    }
    
    ElderCouncilVotingMessage& message = messageIt->second;
    
    // Check if voting is still active
    if (!message.isVotingActive()) {
        logger(WARNING) << "Voting is no longer active for message: " << Common::podToHex(messageId);
        return false;
    }
    
    // Check if Elderfier can vote
    if (!canElderfierVote(voterPublicKey)) {
        logger(ERROR) << "Elderfier cannot vote: " << Common::podToHex(voterPublicKey);
        return false;
    }
    
    // Check if Elderfier has already voted
    if (message.hasVoted) {
        logger(WARNING) << "Elderfier has already voted on message: " << Common::podToHex(messageId);
        return false;
    }
    
    // Set pending vote (not confirmed yet)
    message.pendingVoteType = voteType;
    m_pendingVotes[voterPublicKey][messageId] = voteType;
    
    logger(INFO) << "Pending vote set on message: " 
                << Common::podToHex(messageId)
                << " by Elderfier: " << Common::podToHex(voterPublicKey)
                << " vote type: " << static_cast<int>(voteType)
                << " (PENDING CONFIRMATION)";
    
    return true;
}

bool EldernodeIndexManager::confirmVoteOnMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto messageIt = m_votingMessages.find(messageId);
    if (messageIt == m_votingMessages.end()) {
        logger(WARNING) << "Voting message not found: " << Common::podToHex(messageId);
        return false;
    }
    
    ElderCouncilVotingMessage& message = messageIt->second;
    
    // Check if voting is still active
    if (!message.isVotingActive()) {
        logger(WARNING) << "Voting is no longer active for message: " << Common::podToHex(messageId);
        return false;
    }
    
    // Check if Elderfier has a pending vote
    auto pendingIt = m_pendingVotes.find(voterPublicKey);
    if (pendingIt == m_pendingVotes.end()) {
        logger(WARNING) << "No pending vote found for Elderfier: " << Common::podToHex(voterPublicKey);
        return false;
    }
    
    auto voteIt = pendingIt->second.find(messageId);
    if (voteIt == pendingIt->second.end()) {
        logger(WARNING) << "No pending vote found for message: " << Common::podToHex(messageId);
        return false;
    }
    
    ElderCouncilVoteType voteType = voteIt->second;
    
    // Create confirmed vote
    ElderCouncilVote vote;
    vote.voterPublicKey = voterPublicKey;
    vote.targetPublicKey = message.targetElderfier;
    vote.voteFor = (voteType == ElderCouncilVoteType::SLASH_NONE); // true = allow (no slash), false = deny (slash)
    vote.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    vote.voteHash = vote.calculateVoteHash();
    vote.signature = std::vector<uint8_t>(); // Placeholder for signature
    
    // Add vote to message
    message.votes.push_back(vote);
    message.currentVotes++;
    message.hasVoted = true;
    message.hasConfirmedVote = true;
    message.confirmedVoteType = voteType;
    
    // Update Elder Council votes
    m_elderCouncilVotes[message.targetElderfier].push_back(vote);
    
    // Remove pending vote
    pendingIt->second.erase(voteIt);
    if (pendingIt->second.empty()) {
        m_pendingVotes.erase(pendingIt);
    }
    
    logger(INFO) << "Vote confirmed on message: " 
                << Common::podToHex(messageId)
                << " by Elderfier: " << Common::podToHex(voterPublicKey)
                << " vote type: " << static_cast<int>(voteType)
                << " (CONFIRMED)";
    
    return true;
}

bool EldernodeIndexManager::cancelPendingVote(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto pendingIt = m_pendingVotes.find(voterPublicKey);
    if (pendingIt == m_pendingVotes.end()) {
        logger(WARNING) << "No pending votes found for Elderfier: " << Common::podToHex(voterPublicKey);
        return false;
    }
    
    auto voteIt = pendingIt->second.find(messageId);
    if (voteIt == pendingIt->second.end()) {
        logger(WARNING) << "No pending vote found for message: " << Common::podToHex(messageId);
        return false;
    }
    
    // Remove pending vote
    pendingIt->second.erase(voteIt);
    if (pendingIt->second.empty()) {
        m_pendingVotes.erase(pendingIt);
    }
    
    logger(INFO) << "Pending vote cancelled for message: " 
                << Common::podToHex(messageId)
                << " by Elderfier: " << Common::podToHex(voterPublicKey);
    
    return true;
}

ElderCouncilVoteType EldernodeIndexManager::getPendingVoteType(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto pendingIt = m_pendingVotes.find(voterPublicKey);
    if (pendingIt == m_pendingVotes.end()) {
        return ElderCouncilVoteType::SLASH_NONE; // Default if no pending vote
    }
    
    auto voteIt = pendingIt->second.find(messageId);
    if (voteIt == pendingIt->second.end()) {
        return ElderCouncilVoteType::SLASH_NONE; // Default if no pending vote
    }
    
    return voteIt->second;
}

bool EldernodeIndexManager::hasPendingVote(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto pendingIt = m_pendingVotes.find(voterPublicKey);
    if (pendingIt == m_pendingVotes.end()) {
        return false;
    }
    
    return pendingIt->second.find(messageId) != pendingIt->second.end();
}

ElderCouncilVotingMessage EldernodeIndexManager::getVotingMessage(const Crypto::Hash& messageId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_votingMessages.find(messageId);
    if (it != m_votingMessages.end()) {
    return it->second;
    }
    
    // Return empty message if not found
    return ElderCouncilVotingMessage();
}

bool EldernodeIndexManager::deleteVotingMessage(const Crypto::Hash& messageId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto messageIt = m_votingMessages.find(messageId);
    if (messageIt == m_votingMessages.end()) {
        logger(WARNING) << "Voting message not found for deletion: " << Common::podToHex(messageId);
        return false;
    }
    
    // Remove from all Elderfier inboxes
    for (auto& pair : m_elderfierMessageInbox) {
        auto& inbox = pair.second;
        inbox.erase(std::remove(inbox.begin(), inbox.end(), messageId), inbox.end());
    }
    
    // Remove from read messages
    for (auto& pair : m_elderfierReadMessages) {
        auto& readMessages = pair.second;
        readMessages.erase(std::remove(readMessages.begin(), readMessages.end(), messageId), readMessages.end());
    }
    
    // Remove message
    m_votingMessages.erase(messageIt);
    
    logger(INFO) << "Deleted voting message: " << Common::podToHex(messageId);
    
    return true;
}

std::vector<ElderCouncilVotingMessage> EldernodeIndexManager::getActiveVotingMessages() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ElderCouncilVotingMessage> activeMessages;
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (const auto& pair : m_votingMessages) {
        const ElderCouncilVotingMessage& message = pair.second;
        if (message.timestamp <= currentTimestamp && currentTimestamp <= message.votingDeadline) {
            activeMessages.push_back(message);
        }
    }
    
    // Sort by timestamp (newest first)
    std::sort(activeMessages.begin(), activeMessages.end(), 
              [](const ElderCouncilVotingMessage& a, const ElderCouncilVotingMessage& b) {
                  return a.timestamp > b.timestamp;
              });
    
    return activeMessages;
}

std::string EldernodeIndexManager::getVoteTypeDescription(ElderCouncilVoteType voteType) const {
    switch (voteType) {
        case ElderCouncilVoteType::SLASH_ALL:
            return "SLASH ALL - Burn all of Elderfier's stake";
        case ElderCouncilVoteType::SLASH_HALF:
            return "SLASH HALF - Burn half of Elderfier's stake";
        case ElderCouncilVoteType::SLASH_NONE:
            return "SLASH NONE - Allow Elderfier to keep all stake";
        default:
            return "UNKNOWN VOTE TYPE";
    }
}

std::string EldernodeIndexManager::generateVoteConfirmationMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& voterPublicKey, ElderCouncilVoteType voteType) const {
    std::stringstream ss;
    ss << "⚠️  VOTE CONFIRMATION REQUIRED ⚠️\n\n";
    ss << "You are about to submit the following vote:\n\n";
    ss << "Vote Type: " << getVoteTypeDescription(voteType) << "\n\n";
    ss << "This vote will be FINAL and CANNOT be changed once submitted.\n";
    ss << "Please review your decision carefully.\n\n";
    ss << "Are you sure you want to submit this vote?\n";
    ss << "Type 'YES' to confirm or 'NO' to cancel.";
    
    return ss.str();
}

// Elder Council voting system

bool EldernodeIndexManager::submitElderCouncilVote(const ElderCouncilVote& vote) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Validate vote
    if (!validateVote(vote)) {
        logger(ERROR) << "Invalid Elder Council vote: " << Common::podToHex(vote.voterPublicKey);
        return false;
    }
    
    // Check if voter can vote
    if (!canElderfierVote(vote.voterPublicKey)) {
        logger(ERROR) << "Elderfier cannot vote: " << Common::podToHex(vote.voterPublicKey);
        return false;
    }
    
    // Check if voter has already voted
    if (hasVoted(vote.voterPublicKey, vote.targetPublicKey)) {
        logger(WARNING) << "Elderfier has already voted: " << Common::podToHex(vote.voterPublicKey);
        return false;
    }
    
    // Add vote
    m_elderCouncilVotes[vote.targetPublicKey].push_back(vote);
    
    // Update mempool buffer if applicable
    auto bufferIt = m_mempoolBuffer.begin();
    while (bufferIt != m_mempoolBuffer.end()) {
        if (bufferIt->second.elderfierPublicKey == vote.targetPublicKey) {
            bufferIt->second.addVote(vote.voterPublicKey);
            bufferIt->second.currentVotes++;
            break;
        }
        ++bufferIt;
    }
    
    logger(INFO) << "Elder Council vote submitted: " 
                << Common::podToHex(vote.voterPublicKey)
                << " -> " << Common::podToHex(vote.targetPublicKey)
                << " vote: " << (vote.voteFor ? "FOR" : "AGAINST");
    
    return true;
}

std::vector<ElderCouncilVote> EldernodeIndexManager::getElderCouncilVotes(const Crypto::PublicKey& targetPublicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderCouncilVotes.find(targetPublicKey);
    if (it == m_elderCouncilVotes.end()) {
        return std::vector<ElderCouncilVote>();
    }
    
    return it->second;
}

bool EldernodeIndexManager::hasElderCouncilQuorum(const Crypto::PublicKey& targetPublicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_elderCouncilVotes.find(targetPublicKey);
    if (it == m_elderCouncilVotes.end()) {
        return false;
    }
    
    uint32_t requiredQuorum = calculateRequiredQuorum();
    uint32_t currentVotes = static_cast<uint32_t>(it->second.size());
    
    return currentVotes >= requiredQuorum;
}

bool EldernodeIndexManager::canElderfierVote(const Crypto::PublicKey& voterPublicKey) const {
    // Check if Elderfier is active and in ENindex
    auto eldernodeIt = m_eldernodes.find(voterPublicKey);
    if (eldernodeIt == m_eldernodes.end()) {
        return false;
    }
    
    // Check if Elderfier is active
    if (!eldernodeIt->second.isActive) {
        return false;
    }
    
    // Check if Elderfier tier
    if (eldernodeIt->second.tier != EldernodeTier::ELDERFIER) {
        return false;
    }
    
    return true;
}

// Blockchain integration

void EldernodeIndexManager::setBlockchainExplorer(IBlockchainExplorer* explorer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blockchainExplorer = explorer;
    logger(INFO) << "Blockchain explorer set for Elderfier deposit monitoring";
}

void EldernodeIndexManager::setDepositIndex(const DepositIndex* depositIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_depositIndex = depositIndex;
    logger(INFO) << "Deposit index set for Elderfier deposit monitoring";
}

bool EldernodeIndexManager::isOutputSpent(uint32_t globalIndex) const {
    // This would integrate with the blockchain to check if a specific output is spent
    // For now, we'll implement a placeholder
    
    if (!m_blockchainExplorer) {
        logger(WARNING) << "Blockchain explorer not available for output spending check";
        return false;
    }
    
    // In real implementation, this would:
    // 1. Query the blockchain for transactions that spend this output
    // 2. Check if any transaction inputs reference this global index
    // 3. Return true if the output is spent
    
    // Placeholder implementation
    return false;
}

bool EldernodeIndexManager::isDepositTransactionSpent(const Crypto::Hash& depositHash) const {
    // Check if any outputs of the deposit transaction have been spent
    std::vector<uint32_t> outputIndices = getDepositOutputIndices(depositHash);
    
    for (uint32_t globalIndex : outputIndices) {
        if (isOutputSpent(globalIndex)) {
            return true;
        }
    }
    
    return false;
}

std::vector<uint32_t> EldernodeIndexManager::getDepositOutputIndices(const Crypto::Hash& depositHash) const {
    std::vector<uint32_t> outputIndices;
    
    if (!m_blockchainExplorer) {
        logger(WARNING) << "Blockchain explorer not available for deposit output check";
        return outputIndices;
    }
    
    // Get transaction details
    std::vector<Crypto::Hash> txHashes = {depositHash};
    std::vector<TransactionDetails> transactions;
    
    if (m_blockchainExplorer->getTransactions(txHashes, transactions)) {
        if (!transactions.empty()) {
            const TransactionDetails& tx = transactions[0];
            
            // Extract output global indices
            for (const auto& output : tx.outputs) {
                outputIndices.push_back(output.globalIndex);
            }
        }
    }
    
    return outputIndices;
}

// Mempool buffer helpers

bool EldernodeIndexManager::isTransactionInBuffer(const Crypto::Hash& transactionHash) const {
    return m_mempoolBuffer.find(transactionHash) != m_mempoolBuffer.end();
}

void EldernodeIndexManager::addTransactionToBuffer(const Crypto::Hash& transactionHash, const Crypto::PublicKey& elderfierPublicKey) {
    // This is called internally by processSpendingTransaction
    // Implementation is already in processSpendingTransaction method
}

void EldernodeIndexManager::removeTransactionFromBuffer(const Crypto::Hash& transactionHash) {
    auto it = m_mempoolBuffer.find(transactionHash);
    if (it != m_mempoolBuffer.end()) {
        m_mempoolBuffer.erase(it);
    }
}

bool EldernodeIndexManager::shouldRequireElderCouncilVote(const Crypto::PublicKey& elderfierPublicKey) const {
    auto depositIt = m_elderfierDeposits.find(elderfierPublicKey);
    if (depositIt == m_elderfierDeposits.end()) {
        return true; // Require vote if Elderfier not found
    }
    
    const ElderfierDepositData& deposit = depositIt->second;
    
    // Require Elder Council vote if:
    // 1. Last signature was invalid
    // 2. Elderfier has been offline too long
    // 3. Elderfier has suspicious activity
    
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Check if last signature is invalid
    if (deposit.lastSignatureTimestamp == 0 || 
        (currentTimestamp - deposit.lastSignatureTimestamp) > SecurityWindow::DEFAULT_DURATION_SECONDS) {
        return true;
    }
    
    // Check if Elderfier has been offline too long
    if ((currentTimestamp - deposit.lastSeenTimestamp) > SecurityWindow::MAX_OFFLINE_TIME) {
        return true;
    }
    
    return false;
}

// Elder Council voting helpers

bool EldernodeIndexManager::validateVote(const ElderCouncilVote& vote) const {
    // Basic validation
    if (vote.voterPublicKey == vote.targetPublicKey) {
        return false; // Cannot vote for yourself
    }
    
    // Check timestamp is reasonable
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (vote.timestamp > currentTimestamp || 
        (currentTimestamp - vote.timestamp) > m_monitoringConfig.votingWindowDuration) {
        return false;
    }
    
    // Validate vote hash
    Crypto::Hash calculatedHash = vote.calculateVoteHash();
    if (calculatedHash != vote.voteHash) {
        return false;
    }
    
    return true;
}

bool EldernodeIndexManager::hasVoted(const Crypto::PublicKey& voterPublicKey, const Crypto::PublicKey& targetPublicKey) const {
    auto it = m_elderCouncilVotes.find(targetPublicKey);
    if (it == m_elderCouncilVotes.end()) {
        return false;
    }
    
    for (const auto& vote : it->second) {
        if (vote.voterPublicKey == voterPublicKey) {
            return true;
        }
    }
    
    return false;
}

uint32_t EldernodeIndexManager::calculateRequiredQuorum() const {
    // Calculate quorum based on active Elderfiers
    uint32_t activeElderfiers = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            activeElderfiers++;
        }
    }
    
    // Require at least 5 votes or 50% of active Elderfiers, whichever is smaller
    uint32_t quorum = std::min(m_monitoringConfig.elderCouncilQuorumSize, activeElderfiers / 2);
    return std::max(quorum, 1U); // At least 1 vote required
}

// Elderfier voting interface helpers

Crypto::Hash EldernodeIndexManager::generateMessageId(const MisbehaviorEvidence& evidence) const {
    std::string messageData = Common::podToHex(evidence.elderfierPublicKey) + 
                             std::to_string(evidence.invalidSignatures) + 
                             std::to_string(evidence.totalAttempts) + 
                             std::to_string(evidence.firstInvalidSignature) + 
                             evidence.misbehaviorType;
    
    Crypto::Hash messageId;
    Crypto::cn_fast_hash(messageData.data(), messageData.size(), messageId);
    return messageId;
}

std::string EldernodeIndexManager::generateVotingSubject(const MisbehaviorEvidence& evidence) const {
    std::stringstream ss;
    ss << "New Elder Council Vote Needed - Elderfier " 
       << Common::podToHex(evidence.elderfierPublicKey).substr(0, 8) 
       << " Misbehavior Detected";
    return ss.str();
}

std::string EldernodeIndexManager::generateVotingDescription(const MisbehaviorEvidence& evidence) const {
    std::stringstream ss;
    ss << "Elderfier [" << Common::podToHex(evidence.elderfierPublicKey).substr(0, 8) 
       << "] is trying to unlock stake after providing " 
       << evidence.invalidSignatures << " invalid signatures out of the last " 
       << evidence.totalAttempts << " attempts.\n\n";
    
    ss << "Misbehavior Type: " << evidence.misbehaviorType << "\n";
    ss << "Evidence Description: " << evidence.evidenceDescription << "\n\n";
    
    ss << "Voting Options:\n";
    ss << "1. SLASH ALL - Burn all of Elderfier's stake for invalid signatures\n";
    ss << "2. SLASH HALF - Burn half of Elderfier's stake for invalid signatures\n";
    ss << "3. SLASH NONE - Allow Elderfier to keep all stake\n\n";
    
    ss << "Please vote based on the severity of the misbehavior and the Elderfier's history.";
    
    return ss.str();
}

bool EldernodeIndexManager::addMessageToElderfierInbox(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) {
    m_elderfierMessageInbox[elderfierPublicKey].push_back(messageId);
    return true;
}

bool EldernodeIndexManager::removeMessageFromElderfierInbox(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) {
    auto inboxIt = m_elderfierMessageInbox.find(elderfierPublicKey);
    if (inboxIt != m_elderfierMessageInbox.end()) {
        auto& inbox = inboxIt->second;
        inbox.erase(std::remove(inbox.begin(), inbox.end(), messageId), inbox.end());
        return true;
    }
    return false;
}

bool EldernodeIndexManager::isMessageInElderfierInbox(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) const {
    auto inboxIt = m_elderfierMessageInbox.find(elderfierPublicKey);
    if (inboxIt != m_elderfierMessageInbox.end()) {
        const auto& inbox = inboxIt->second;
        return std::find(inbox.begin(), inbox.end(), messageId) != inbox.end();
    }
    return false;
}

bool EldernodeIndexManager::hasElderfierReadMessage(const Crypto::Hash& messageId, const Crypto::PublicKey& elderfierPublicKey) const {
    auto readIt = m_elderfierReadMessages.find(elderfierPublicKey);
    if (readIt != m_elderfierReadMessages.end()) {
        const auto& readMessages = readIt->second;
        return std::find(readMessages.begin(), readMessages.end(), messageId) != readMessages.end();
    }
    return false;
}

// Implementation of MempoolSecurityWindow methods

bool MempoolSecurityWindow::isSecurityWindowActive() const {
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return currentTimestamp < securityWindowEnd;
}

bool MempoolSecurityWindow::hasQuorumReached() const {
    return currentVotes >= requiredVotes;
}

bool MempoolSecurityWindow::canReleaseTransaction() const {
    // Can release if:
    // 1. Security window has expired AND
    // 2. Either signature is valid OR Elder Council has reached quorum
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    bool windowExpired = currentTimestamp >= securityWindowEnd;
    bool canRelease = signatureValidated || (elderCouncilVoteRequired && hasQuorumReached());
    
    return windowExpired && canRelease;
}

void MempoolSecurityWindow::addVote(const Crypto::PublicKey& voter) {
    votes.push_back(voter);
}

std::string MempoolSecurityWindow::toString() const {
    std::stringstream ss;
    ss << "MempoolSecurityWindow{"
       << "txHash=" << Common::podToHex(transactionHash)
       << ", elderfier=" << Common::podToHex(elderfierPublicKey)
       << ", timestamp=" << timestamp
       << ", windowEnd=" << securityWindowEnd
       << ", signatureValid=" << signatureValidated
       << ", councilVoteRequired=" << elderCouncilVoteRequired
       << ", votes=" << currentVotes << "/" << requiredVotes
       << "}";
    return ss.str();
}

// Implementation of ElderCouncilVote methods

bool ElderCouncilVote::isValid() const {
    return voterPublicKey != targetPublicKey && // Cannot vote for yourself
           timestamp > 0 &&
           !signature.empty();
}

Crypto::Hash ElderCouncilVote::calculateVoteHash() const {
    std::string voteData = Common::podToHex(voterPublicKey) + 
                          Common::podToHex(targetPublicKey) + 
                          std::to_string(timestamp) + 
                          (voteFor ? "FOR" : "AGAINST");
    
    Crypto::Hash hash;
    Crypto::cn_fast_hash(voteData.data(), voteData.size(), hash);
    return hash;
}

std::string ElderCouncilVote::toString() const {
    std::stringstream ss;
    ss << "ElderCouncilVote{"
       << "voter=" << Common::podToHex(voterPublicKey)
       << ", target=" << Common::podToHex(targetPublicKey)
       << ", vote=" << (voteFor ? "FOR" : "AGAINST")
       << ", timestamp=" << timestamp
       << ", hash=" << Common::podToHex(voteHash)
       << "}";
    return ss.str();
}

// Implementation of ElderCouncilVotingMessage methods

bool ElderCouncilVotingMessage::isVotingActive() const {
    uint64_t currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return currentTimestamp <= votingDeadline;
}

bool ElderCouncilVotingMessage::hasQuorumReached() const {
    return currentVotes >= requiredVotes;
}

std::string ElderCouncilVotingMessage::getVotingStatus() const {
    std::stringstream ss;
    ss << "Votes: " << currentVotes << "/" << requiredVotes;
    
    if (hasQuorumReached()) {
        ss << " (QUORUM REACHED)";
    } else {
        ss << " (PENDING)";
    }
    
    if (!isVotingActive()) {
        ss << " (VOTING CLOSED)";
    }
    
    return ss.str();
}

std::string ElderCouncilVotingMessage::toString() const {
    std::stringstream ss;
    ss << "ElderCouncilVotingMessage{"
       << "messageId=" << Common::podToHex(messageId)
       << ", targetElderfier=" << Common::podToHex(targetElderfier)
       << ", subject=" << subject
       << ", timestamp=" << timestamp
       << ", deadline=" << votingDeadline
       << ", isRead=" << isRead
       << ", hasVoted=" << hasVoted
       << ", hasConfirmedVote=" << hasConfirmedVote
       << ", pendingVoteType=" << static_cast<int>(pendingVoteType)
       << ", confirmedVoteType=" << static_cast<int>(confirmedVoteType)
       << ", " << getVotingStatus()
       << "}";
    return ss.str();
}

// Implementation of MisbehaviorEvidence methods

bool MisbehaviorEvidence::isValid() const {
    return elderfierPublicKey != Crypto::PublicKey() && // Valid public key
           invalidSignatures > 0 &&                       // At least one invalid signature
           totalAttempts >= invalidSignatures &&          // Invalid signatures <= total attempts
           !misbehaviorType.empty() &&                    // Has misbehavior type
           !evidenceDescription.empty();                  // Has evidence description
}

std::string MisbehaviorEvidence::getSummary() const {
    std::stringstream ss;
    ss << "Elderfier [" << Common::podToHex(elderfierPublicKey).substr(0, 8) 
       << "] provided " << invalidSignatures << " invalid signatures out of " 
       << totalAttempts << " attempts (" 
       << (invalidSignatures * 100 / totalAttempts) << "% failure rate)";
    return ss.str();
}

std::string MisbehaviorEvidence::toString() const {
    std::stringstream ss;
    ss << "MisbehaviorEvidence{"
       << "elderfier=" << Common::podToHex(elderfierPublicKey)
       << ", invalidSignatures=" << invalidSignatures
       << ", totalAttempts=" << totalAttempts
       << ", firstInvalid=" << firstInvalidSignature
       << ", lastInvalid=" << lastInvalidSignature
       << ", type=" << misbehaviorType
       << ", description=" << evidenceDescription
       << "}";
    return ss.str();
}

// Monitoring configuration and control

void EldernodeIndexManager::setMonitoringConfig(const ElderfierMonitoringConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!config.isValid()) {
        logger(ERROR) << "Invalid monitoring configuration provided";
        return;
    }
    
    m_monitoringConfig = config;
    logger(INFO) << "Monitoring configuration updated: block-based=" << config.enableBlockBasedMonitoring 
                << ", mempool-buffer=" << config.enableMempoolBuffer 
                << ", elder-council-voting=" << config.enableElderCouncilVoting;
}

ElderfierMonitoringConfig EldernodeIndexManager::getMonitoringConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_monitoringConfig;
}

void EldernodeIndexManager::startMonitoring() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_monitoringActive) {
        logger(WARNING) << "Monitoring is already active";
        return;
    }
    
    if (!m_monitoringConfig.enableBlockBasedMonitoring) {
        logger(INFO) << "Block-based monitoring is disabled in configuration";
        return;
    }
    
    m_shouldStopMonitoring = false;
    m_monitoringActive = true;
    
    // Start monitoring thread
    m_monitoringThread = std::thread([this]() {
        logger(INFO) << "Elderfier monitoring thread started with block-based monitoring";
        
        while (!m_shouldStopMonitoring) {
            try {
                // Perform monitoring
                bool changesMade = monitorElderfierDeposits();
                
                if (changesMade) {
                    logger(INFO) << "Elderfier monitoring detected changes";
                    // In real implementation, this would notify the network
                    // broadcastENindexUpdate();
                }
                
                // Sleep for a reasonable interval (block-based monitoring doesn't need frequent polling)
                std::this_thread::sleep_for(std::chrono::seconds(30));
                
            } catch (const std::exception& e) {
                logger(ERROR) << "Exception in monitoring thread: " << e.what();
                // Continue monitoring despite exceptions
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        
        logger(INFO) << "Elderfier monitoring thread stopped";
    });
    
    logger(INFO) << "Elderfier monitoring started successfully";
}

void EldernodeIndexManager::stopMonitoring() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_monitoringActive) {
        logger(WARNING) << "Monitoring is not active";
        return;
    }
    
    m_shouldStopMonitoring = true;
    m_monitoringActive = false;
    
    // Wait for thread to finish
    if (m_monitoringThread.joinable()) {
        m_monitoringThread.join();
    }
    
    logger(INFO) << "Elderfier monitoring stopped successfully";
}

bool EldernodeIndexManager::isMonitoringActive() const {
    return m_monitoringActive;
}

bool EldernodeIndexManager::addConsensusParticipant(const EldernodeConsensusParticipant& participant) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_consensusParticipants[participant.publicKey] = participant;
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (participant.tier == EldernodeTier::ELDERFIER) ? "Basic" : "Elderfier";
    logger(INFO) << "Added " << tierName << " consensus participant: " << Common::podToHex(participant.publicKey);
    
    return true;
}

bool EldernodeIndexManager::removeConsensusParticipant(const Crypto::PublicKey& publicKey) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_consensusParticipants.find(publicKey);
    if (it == m_consensusParticipants.end()) {
        return false;
    }
    
    m_consensusParticipants.erase(it);
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Removed consensus participant: " << Common::podToHex(publicKey);
    return true;
}

std::vector<EldernodeConsensusParticipant> EldernodeIndexManager::getConsensusParticipants() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<EldernodeConsensusParticipant> result;
    result.reserve(m_consensusParticipants.size());
    
    for (const auto& pair : m_consensusParticipants) {
        result.push_back(pair.second);
    }
    
    return result;
}

EldernodeConsensusResult EldernodeIndexManager::reachConsensus(const std::vector<uint8_t>& data, const ConsensusThresholds& thresholds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    EldernodeConsensusResult result;
    result.consensusReached = false;
    result.requiredThreshold = thresholds.requiredAgreement;
    result.actualVotes = 0;
    result.consensusTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Get active participants (prioritize Elderfier nodes)
    std::vector<EldernodeConsensusParticipant> activeParticipants;
    for (const auto& pair : m_consensusParticipants) {
        if (pair.second.isActive) {
            activeParticipants.push_back(pair.second);
        }
    }
    
    // Sort by tier (Elderfier first) and stake amount
    std::sort(activeParticipants.begin(), activeParticipants.end());
    
    if (activeParticipants.size() < thresholds.minimumEldernodes) {
        logger(WARNING) << "Insufficient active Eldernodes for consensus: " 
                       << activeParticipants.size() << "/" << thresholds.minimumEldernodes;
        return result;
    }
    
    // Simulate consensus voting (prioritize Elderfier nodes)
    std::vector<std::vector<uint8_t>> signatures;
    for (const auto& participant : activeParticipants) {
        // Simulate signature generation
        Crypto::Hash dataHash;
        Crypto::cn_fast_hash(data.data(), data.size(), dataHash);
        
        // For now, we'll simulate a simple agreement
        // In real implementation, each Eldernode would sign the data
        std::vector<uint8_t> signature(64, 0); // Placeholder signature
        signatures.push_back(signature);
        result.participatingEldernodes.push_back(participant.publicKey);
    }
    
    result.actualVotes = static_cast<uint32_t>(signatures.size());
    
    // Check if consensus threshold is met (4/5 instead of 3/5 as requested)
    if (result.actualVotes >= thresholds.requiredAgreement) {
        result.consensusReached = true;
        result.aggregatedSignature = aggregateSignatures(signatures);
        logger(INFO) << "Consensus reached: " << result.actualVotes << "/" << thresholds.requiredAgreement;
    } else {
        logger(WARNING) << "Consensus failed: " << result.actualVotes << "/" << thresholds.requiredAgreement;
    }
    
    return result;
}

uint32_t EldernodeIndexManager::getTotalEldernodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32_t>(m_eldernodes.size());
}

uint32_t EldernodeIndexManager::getActiveEldernodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint32_t count = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            count++;
        }
    }
    
    return count;
}

uint32_t EldernodeIndexManager::getElderfierNodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint32_t count = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            count++;
        }
    }
    
    return count;
}

uint64_t EldernodeIndexManager::getTotalStakeAmount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint64_t total = 0;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.isActive) {
            total += pair.second.stakeAmount;
        }
    }
    
    return total;
}

std::chrono::system_clock::time_point EldernodeIndexManager::getLastUpdate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastUpdate;
}

bool EldernodeIndexManager::saveToStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ofstream file("eldernode_index.dat", std::ios::binary);
        if (!file) {
            logger(ERROR) << "Failed to open storage file for writing";
            return false;
        }
        
        // Write Eldernodes
        uint32_t eldernodeCount = static_cast<uint32_t>(m_eldernodes.size());
        file.write(reinterpret_cast<const char*>(&eldernodeCount), sizeof(eldernodeCount));
        
        for (const auto& pair : m_eldernodes) {
            const auto& entry = pair.second;
            file.write(reinterpret_cast<const char*>(&entry.eldernodePublicKey), sizeof(entry.eldernodePublicKey));
            uint32_t feeAddressSize = static_cast<uint32_t>(entry.feeAddress.size());
            file.write(reinterpret_cast<const char*>(&feeAddressSize), sizeof(feeAddressSize));
            file.write(entry.feeAddress.c_str(), feeAddressSize);
            file.write(reinterpret_cast<const char*>(&entry.stakeAmount), sizeof(entry.stakeAmount));
            file.write(reinterpret_cast<const char*>(&entry.registrationTimestamp), sizeof(entry.registrationTimestamp));
            file.write(reinterpret_cast<const char*>(&entry.isActive), sizeof(entry.isActive));
            file.write(reinterpret_cast<const char*>(&entry.tier), sizeof(entry.tier));
            
            // Write Elderfier service ID if applicable
            if (entry.tier == EldernodeTier::ELDERFIER) {
                file.write(reinterpret_cast<const char*>(&entry.serviceId.type), sizeof(entry.serviceId.type));
                uint32_t identifierSize = static_cast<uint32_t>(entry.serviceId.identifier.size());
                file.write(reinterpret_cast<const char*>(&identifierSize), sizeof(identifierSize));
                file.write(entry.serviceId.identifier.c_str(), identifierSize);
                uint32_t displayNameSize = static_cast<uint32_t>(entry.serviceId.displayName.size());
                file.write(reinterpret_cast<const char*>(&displayNameSize), sizeof(displayNameSize));
                file.write(entry.serviceId.displayName.c_str(), displayNameSize);
                uint32_t linkedAddressSize = static_cast<uint32_t>(entry.serviceId.linkedAddress.size());
                file.write(reinterpret_cast<const char*>(&linkedAddressSize), sizeof(linkedAddressSize));
                file.write(entry.serviceId.linkedAddress.c_str(), linkedAddressSize);
                uint32_t hashedAddressSize = static_cast<uint32_t>(entry.serviceId.hashedAddress.size());
                file.write(reinterpret_cast<const char*>(&hashedAddressSize), sizeof(hashedAddressSize));
                file.write(entry.serviceId.hashedAddress.c_str(), hashedAddressSize);
            }
            
            // Write constant stake proof data
            file.write(reinterpret_cast<const char*>(&entry.constantProofType), sizeof(entry.constantProofType));
            uint32_t crossChainAddressSize = static_cast<uint32_t>(entry.crossChainAddress.size());
            file.write(reinterpret_cast<const char*>(&crossChainAddressSize), sizeof(crossChainAddressSize));
            file.write(entry.crossChainAddress.c_str(), crossChainAddressSize);
            file.write(reinterpret_cast<const char*>(&entry.constantStakeAmount), sizeof(entry.constantStakeAmount));
            file.write(reinterpret_cast<const char*>(&entry.constantProofExpiry), sizeof(entry.constantProofExpiry));
        }
        
        logger(INFO) << "Saved " << eldernodeCount << " Eldernodes to storage";
        return true;
    } catch (const std::exception& e) {
        logger(ERROR) << "Exception during save: " << e.what();
        return false;
    }
}

bool EldernodeIndexManager::loadFromStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ifstream file("eldernode_index.dat", std::ios::binary);
        if (!file) {
            logger(INFO) << "No existing storage file found";
            return true; // Not an error if file doesn't exist
        }
        
        // Read Eldernodes
        uint32_t eldernodeCount;
        file.read(reinterpret_cast<char*>(&eldernodeCount), sizeof(eldernodeCount));
        
        for (uint32_t i = 0; i < eldernodeCount; ++i) {
            ENindexEntry entry;
            file.read(reinterpret_cast<char*>(&entry.eldernodePublicKey), sizeof(entry.eldernodePublicKey));
            
            uint32_t feeAddressSize;
            file.read(reinterpret_cast<char*>(&feeAddressSize), sizeof(feeAddressSize));
            entry.feeAddress.resize(feeAddressSize);
            file.read(&entry.feeAddress[0], feeAddressSize);
            
            file.read(reinterpret_cast<char*>(&entry.stakeAmount), sizeof(entry.stakeAmount));
            file.read(reinterpret_cast<char*>(&entry.registrationTimestamp), sizeof(entry.registrationTimestamp));
            file.read(reinterpret_cast<char*>(&entry.isActive), sizeof(entry.isActive));
            file.read(reinterpret_cast<char*>(&entry.tier), sizeof(entry.tier));
            
            // Read Elderfier service ID if applicable
            if (entry.tier == EldernodeTier::ELDERFIER) {
                file.read(reinterpret_cast<char*>(&entry.serviceId.type), sizeof(entry.serviceId.type));
                uint32_t identifierSize;
                file.read(reinterpret_cast<char*>(&identifierSize), sizeof(identifierSize));
                entry.serviceId.identifier.resize(identifierSize);
                file.read(&entry.serviceId.identifier[0], identifierSize);
                uint32_t displayNameSize;
                file.read(reinterpret_cast<char*>(&displayNameSize), sizeof(displayNameSize));
                entry.serviceId.displayName.resize(displayNameSize);
                file.read(&entry.serviceId.displayName[0], displayNameSize);
                uint32_t linkedAddressSize;
                file.read(reinterpret_cast<char*>(&linkedAddressSize), sizeof(linkedAddressSize));
                entry.serviceId.linkedAddress.resize(linkedAddressSize);
                file.read(&entry.serviceId.linkedAddress[0], linkedAddressSize);
                uint32_t hashedAddressSize;
                file.read(reinterpret_cast<char*>(&hashedAddressSize), sizeof(hashedAddressSize));
                entry.serviceId.hashedAddress.resize(hashedAddressSize);
                file.read(&entry.serviceId.hashedAddress[0], hashedAddressSize);
            }
            
            // Read constant stake proof data
            file.read(reinterpret_cast<char*>(&entry.constantProofType), sizeof(entry.constantProofType));
            uint32_t crossChainAddressSize;
            file.read(reinterpret_cast<char*>(&crossChainAddressSize), sizeof(crossChainAddressSize));
            entry.crossChainAddress.resize(crossChainAddressSize);
            file.read(&entry.crossChainAddress[0], crossChainAddressSize);
            file.read(reinterpret_cast<char*>(&entry.constantStakeAmount), sizeof(entry.constantStakeAmount));
            file.read(reinterpret_cast<char*>(&entry.constantProofExpiry), sizeof(entry.constantProofExpiry));
            
            m_eldernodes[entry.eldernodePublicKey] = entry;
        }
        
        logger(INFO) << "Loaded " << eldernodeCount << " Eldernodes from storage";
        return true;
    } catch (const std::exception& e) {
        logger(ERROR) << "Exception during load: " << e.what();
        return false;
    }
}

bool EldernodeIndexManager::clearStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_eldernodes.clear();
    m_stakeProofs.clear();
    m_consensusParticipants.clear();
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Cleared all Eldernode data";
    return true;
}

void EldernodeIndexManager::setConsensusThresholds(const ConsensusThresholds& thresholds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consensusThresholds = thresholds;
}

ConsensusThresholds EldernodeIndexManager::getConsensusThresholds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_consensusThresholds;
}

void EldernodeIndexManager::setElderfierConfig(const ElderfierServiceConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_elderfierConfig = config;
}

ElderfierServiceConfig EldernodeIndexManager::getElderfierConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_elderfierConfig;
}

bool EldernodeIndexManager::generateFreshProof(const Crypto::PublicKey& publicKey, const std::string& feeAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for proof generation: " << Common::podToHex(publicKey);
        return false;
    }
    
    const auto& entry = it->second;
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    EldernodeStakeProof proof;
    proof.eldernodePublicKey = publicKey;
    proof.stakeAmount = entry.stakeAmount;
    proof.timestamp = timestamp;
    proof.feeAddress = feeAddress;
    proof.tier = entry.tier;
    proof.serviceId = entry.serviceId;
    proof.stakeHash = calculateStakeHash(publicKey, entry.stakeAmount, timestamp);
    
    // Generate signature (placeholder for now)
    proof.proofSignature.resize(64, 0);
    
    m_stakeProofs[publicKey].push_back(proof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string tierName = (entry.tier == EldernodeTier::ELDERFIER) ? "Basic" : "Elderfier";
    logger(INFO) << "Generated fresh proof for " << tierName << " Eldernode: " << Common::podToHex(publicKey);
    
    return true;
}

bool EldernodeIndexManager::regenerateAllProofs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    bool success = true;
    for (const auto& pair : m_eldernodes) {
        if (!generateFreshProof(pair.first, pair.second.feeAddress)) {
            success = false;
        }
    }
    
    logger(INFO) << "Regenerated proofs for all Eldernodes, success: " << success;
    return success;
}

// Constant stake proof management for cross-chain validation
bool EldernodeIndexManager::createConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                                     ConstantStakeProofType proofType,
                                                     const std::string& crossChainAddress,
                                                     uint64_t stakeAmount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for constant proof creation: " << Common::podToHex(publicKey);
        return false;
    }
    
    const auto& entry = it->second;
    
    // Only Elderfier nodes can create constant stake proofs
    if (entry.tier != EldernodeTier::ELDERFIER) {
        logger(ERROR) << "Only Elderfier nodes can create constant stake proofs: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Validate constant proof type
    if (proofType == ConstantStakeProofType::NONE) {
        logger(ERROR) << "Invalid constant proof type: NONE";
        return false;
    }
    
    // Check if constant proof type is enabled
    if (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR && 
        !m_elderfierConfig.constantProofConfig.enableElderadoC0DL3Validator) {
        logger(ERROR) << "Elderado C0DL3 validator constant proofs are disabled";
        return false;
    }
    
    // Validate stake amount
    uint64_t requiredStake = m_elderfierConfig.constantProofConfig.getRequiredStakeAmount(proofType);
    if (stakeAmount < requiredStake) {
        logger(ERROR) << "Insufficient stake for constant proof: " << stakeAmount 
                     << " < " << requiredStake << " required";
        return false;
    }
    
    // Check if Eldernode has sufficient total stake
    if (entry.stakeAmount < stakeAmount) {
        logger(ERROR) << "Eldernode total stake insufficient for constant proof: " 
                     << entry.stakeAmount << " < " << stakeAmount;
        return false;
    }
    
    // Check for existing constant proof of same type
    auto existingProofs = getConstantStakeProofs(publicKey);
    for (const auto& existingProof : existingProofs) {
        if (existingProof.constantProofType == proofType && !existingProof.isConstantProofExpired()) {
            logger(ERROR) << "Constant proof of type " << static_cast<int>(proofType) 
                         << " already exists for Eldernode: " << Common::podToHex(publicKey);
            return false;
        }
    }
    
    // Create constant stake proof
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    EldernodeStakeProof constantProof;
    constantProof.eldernodePublicKey = publicKey;
    constantProof.stakeAmount = stakeAmount;
    constantProof.timestamp = timestamp;
    constantProof.feeAddress = entry.feeAddress;
    constantProof.tier = entry.tier;
    constantProof.serviceId = entry.serviceId;
    constantProof.constantProofType = proofType;
    constantProof.crossChainAddress = crossChainAddress;
    constantProof.constantStakeAmount = stakeAmount;
    
    // Set expiry based on configuration
    if (m_elderfierConfig.constantProofConfig.constantProofValidityPeriod > 0) {
        constantProof.constantProofExpiry = timestamp + m_elderfierConfig.constantProofConfig.constantProofValidityPeriod;
    } else {
        constantProof.constantProofExpiry = 0; // Never expires
    }
    
    constantProof.stakeHash = calculateStakeHash(publicKey, stakeAmount, timestamp);
    
    // Generate signature
    constantProof.proofSignature.resize(64, 0);
    
    // Add to stake proofs
    m_stakeProofs[publicKey].push_back(constantProof);
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string proofTypeName = (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) 
                               ? "Elderado C0DL3 Validator" : "Unknown";
    
    logger(INFO) << "Created constant stake proof for " << proofTypeName 
                << " - Eldernode: " << Common::podToHex(publicKey)
                << " Amount: " << stakeAmount << " XFG"
                << " Cross-chain address: " << crossChainAddress;
    
    return true;
}

bool EldernodeIndexManager::renewConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                                    ConstantStakeProofType proofType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_elderfierConfig.constantProofConfig.allowConstantProofRenewal) {
        logger(ERROR) << "Constant proof renewal is disabled";
        return false;
    }
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for constant proof renewal: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Find existing constant proof
    auto existingProofs = getConstantStakeProofs(publicKey);
    EldernodeStakeProof* existingProof = nullptr;
    
    for (auto& proof : m_stakeProofs[publicKey]) {
        if (proof.constantProofType == proofType) {
            existingProof = &proof;
            break;
        }
    }
    
    if (!existingProof) {
        logger(ERROR) << "No existing constant proof of type " << static_cast<int>(proofType) 
                     << " found for Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Update timestamp and expiry
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    existingProof->timestamp = timestamp;
    existingProof->stakeHash = calculateStakeHash(publicKey, existingProof->stakeAmount, timestamp);
    
    if (m_elderfierConfig.constantProofConfig.constantProofValidityPeriod > 0) {
        existingProof->constantProofExpiry = timestamp + m_elderfierConfig.constantProofConfig.constantProofValidityPeriod;
    }
    
    m_lastUpdate = std::chrono::system_clock::now();
    
    std::string proofTypeName = (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) 
                               ? "Elderado C0DL3 Validator" : "Unknown";
    
    logger(INFO) << "Renewed constant stake proof for " << proofTypeName 
                << " - Eldernode: " << Common::podToHex(publicKey);
    
    return true;
}

bool EldernodeIndexManager::revokeConstantStakeProof(const Crypto::PublicKey& publicKey, 
                                                     ConstantStakeProofType proofType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_stakeProofs.find(publicKey);
    if (it == m_stakeProofs.end()) {
        logger(ERROR) << "No stake proofs found for Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    // Find and remove constant proof
    auto& proofs = it->second;
    auto proofIt = proofs.begin();
    bool found = false;
    
    while (proofIt != proofs.end()) {
        if (proofIt->constantProofType == proofType) {
            found = true;
            std::string proofTypeName = (proofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) 
                                       ? "Elderado C0DL3 Validator" : "Unknown";
            
            logger(INFO) << "Revoked constant stake proof for " << proofTypeName 
                        << " - Eldernode: " << Common::podToHex(publicKey);
            
            proofIt = proofs.erase(proofIt);
            break;
        } else {
            ++proofIt;
        }
    }
    
    if (!found) {
        logger(ERROR) << "No constant proof of type " << static_cast<int>(proofType) 
                     << " found for Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    m_lastUpdate = std::chrono::system_clock::now();
    return true;
}

std::vector<EldernodeStakeProof> EldernodeIndexManager::getConstantStakeProofs(const Crypto::PublicKey& publicKey) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_stakeProofs.find(publicKey);
    if (it == m_stakeProofs.end()) {
        return {};
    }
    
    std::vector<EldernodeStakeProof> constantProofs;
    for (const auto& proof : it->second) {
        if (proof.isConstantProof()) {
            constantProofs.push_back(proof);
        }
    }
    
    return constantProofs;
}

std::vector<EldernodeStakeProof> EldernodeIndexManager::getConstantStakeProofsByType(ConstantStakeProofType proofType) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<EldernodeStakeProof> constantProofs;
    for (const auto& pair : m_stakeProofs) {
        for (const auto& proof : pair.second) {
            if (proof.constantProofType == proofType && proof.isConstantProof()) {
                constantProofs.push_back(proof);
            }
        }
    }
    
    return constantProofs;
}

// Slashing functionality
bool EldernodeIndexManager::slashEldernode(const Crypto::PublicKey& publicKey, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_elderfierConfig.slashingConfig.enableSlashing) {
        logger(WARNING) << "Slashing is disabled";
        return false;
    }
    
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
        logger(ERROR) << "Eldernode not found for slashing: " << Common::podToHex(publicKey);
        return false;
    }
    
    const auto& entry = it->second;
    if (entry.tier != EldernodeTier::ELDERFIER) {
        logger(ERROR) << "Cannot slash Basic Eldernode: " << Common::podToHex(publicKey);
        return false;
    }
    
    uint64_t slashedAmount = (entry.stakeAmount * m_elderfierConfig.slashingConfig.slashingPercentage) / 100;
    
    // Update the Eldernode entry with reduced stake
    ENindexEntry updatedEntry = entry;
    updatedEntry.stakeAmount -= slashedAmount;
    
    // Handle slashed amount based on destination
    switch (m_elderfierConfig.slashingConfig.destination) {
        case SlashingDestination::BURN:
            logger(INFO) << "Burned " << slashedAmount << " XFG from Eldernode: " << Common::podToHex(publicKey);
            break;
            
        case SlashingDestination::TREASURY:
            logger(INFO) << "Sent " << slashedAmount << " XFG to treasury from Eldernode: " << Common::podToHex(publicKey);
            break;
            
        case SlashingDestination::REDISTRIBUTE:
            redistributeSlashedStake(slashedAmount);
            logger(INFO) << "Redistributed " << slashedAmount << " XFG to other Eldernodes from: " << Common::podToHex(publicKey);
            break;
            
        case SlashingDestination::CHARITY:
            logger(INFO) << "Sent " << slashedAmount << " XFG to charity from Eldernode: " << Common::podToHex(publicKey);
            break;
    }
    
    // Update the Eldernode
    it->second = updatedEntry;
    m_lastUpdate = std::chrono::system_clock::now();
    
    logger(INFO) << "Slashed Eldernode: " << Common::podToHex(publicKey) 
                << " amount: " << slashedAmount << " reason: " << reason;
    
    return true;
}

// Private helper methods

bool EldernodeIndexManager::validateEldernodeEntry(const ENindexEntry& entry) const {
    if (entry.feeAddress.empty()) {
        return false;
    }
    
    // Check tier-specific requirements
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (entry.stakeAmount < m_elderfierConfig.minimumStakeAmount) {
            logger(ERROR) << "Elderfier node stake too low: " << entry.stakeAmount 
                         << " < " << m_elderfierConfig.minimumStakeAmount << " (800 XFG)";
            return false;
        }
    } else if (entry.tier == EldernodeTier::ELDERFIER) {
        // Basic Eldernodes have no stake requirement (--set-fee-address only)
        if (entry.stakeAmount != 0) {
            logger(ERROR) << "Basic Eldernode should have no stake: " << entry.stakeAmount;
            return false;
        }
    }
    
    return true;
}

bool EldernodeIndexManager::validateStakeProof(const EldernodeStakeProof& proof) const {
    if (proof.feeAddress.empty()) {
        return false;
    }
    
    if (proof.proofSignature.empty()) {
        return false;
    }
    
    // Check tier-specific requirements
    if (proof.tier == EldernodeTier::ELDERFIER) {
        if (proof.stakeAmount < m_elderfierConfig.minimumStakeAmount) {
            return false;
        }
        if (!proof.serviceId.isValid()) {
            return false;
        }
    } else if (proof.tier == EldernodeTier::ELDERFIER) {
        // Basic Eldernodes have no stake requirement
        if (proof.stakeAmount != 0) {
            return false;
        }
    }
    
    // Validate constant proof if applicable
    if (proof.isConstantProof()) {
        // Check if constant proof type is enabled
        if (proof.constantProofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR && 
            !m_elderfierConfig.constantProofConfig.enableElderadoC0DL3Validator) {
            return false;
        }
        
        // Validate constant stake amount
        uint64_t requiredStake = m_elderfierConfig.constantProofConfig.getRequiredStakeAmount(proof.constantProofType);
        if (proof.constantStakeAmount < requiredStake) {
            return false;
        }
        
        // Validate cross-chain address
        if (proof.crossChainAddress.empty()) {
            return false;
        }
        
        // Check if constant proof is expired
        if (proof.isConstantProofExpired()) {
            return false;
        }
    }
    
    // Verify stake hash
    Crypto::Hash expectedHash = calculateStakeHash(proof.eldernodePublicKey, proof.stakeAmount, proof.timestamp);
    if (proof.stakeHash != expectedHash) {
        return false;
    }
    
    return true;
}

bool EldernodeIndexManager::validateElderfierServiceId(const ElderfierServiceId& serviceId) const {
    if (!serviceId.isValid()) {
        return false;
    }
    
    switch (serviceId.type) {
        case ServiceIdType::CUSTOM_NAME:
            if (!m_elderfierConfig.isValidCustomName(serviceId.identifier)) {
                logger(ERROR) << "Invalid custom name: " << serviceId.identifier;
                return false;
            }
            if (m_elderfierConfig.isCustomNameReserved(serviceId.identifier)) {
                logger(ERROR) << "Custom name is reserved: " << serviceId.identifier;
                return false;
            }
            break;
            
        case ServiceIdType::HASHED_ADDRESS:
            if (!m_elderfierConfig.allowHashedAddresses) {
                logger(ERROR) << "Hashed addresses not allowed";
                return false;
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

bool EldernodeIndexManager::hasServiceIdConflict(const ElderfierServiceId& serviceId, const Crypto::PublicKey& excludeKey) const {
    for (const auto& pair : m_eldernodes) {
        if (pair.first != excludeKey && 
            pair.second.tier == EldernodeTier::ELDERFIER &&
            pair.second.serviceId.identifier == serviceId.identifier) {
            return true;
        }
    }
    return false;
}

Crypto::Hash EldernodeIndexManager::calculateStakeHash(const Crypto::PublicKey& publicKey, uint64_t amount, uint64_t timestamp) const {
    std::string data = Common::podToHex(publicKey) + std::to_string(amount) + std::to_string(timestamp);
    Crypto::Hash hash;
    Crypto::cn_fast_hash(data.data(), data.size(), hash);
    return hash;
}

std::vector<uint8_t> EldernodeIndexManager::aggregateSignatures(const std::vector<std::vector<uint8_t>>& signatures) const {
    // Simple aggregation - concatenate all signatures
    std::vector<uint8_t> result;
    for (const auto& sig : signatures) {
        result.insert(result.end(), sig.begin(), sig.end());
    }
    return result;
}

void EldernodeIndexManager::redistributeSlashedStake(uint64_t slashedAmount) {
    // Simple redistribution to active Elderfier nodes
    std::vector<ENindexEntry> activeElderfierNodes;
    for (const auto& pair : m_eldernodes) {
        if (pair.second.tier == EldernodeTier::ELDERFIER && pair.second.isActive) {
            activeElderfierNodes.push_back(pair.second);
        }
    }
    
    if (activeElderfierNodes.empty()) {
        logger(WARNING) << "No active Elderfier nodes for stake redistribution";
        return;
    }
    
    uint64_t amountPerNode = slashedAmount / activeElderfierNodes.size();
    uint64_t remainder = slashedAmount % activeElderfierNodes.size();
    
    for (size_t i = 0; i < activeElderfierNodes.size(); ++i) {
        uint64_t bonus = (i < remainder) ? 1 : 0;
        uint64_t totalBonus = amountPerNode + bonus;
        
        // Update the Eldernode's stake
        auto it = m_eldernodes.find(activeElderfierNodes[i].eldernodePublicKey);
        if (it != m_eldernodes.end()) {
            it->second.stakeAmount += totalBonus;
        }
    }
    
    logger(INFO) << "Redistributed " << slashedAmount << " XFG to " << activeElderfierNodes.size() << " active Elderfier nodes";
}

// Elderfier deposit monitoring helpers

bool EldernodeIndexManager::checkDepositSpending(const Crypto::PublicKey& publicKey) const {
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        return false;
    }
    
    // Check if the deposit transaction outputs have been spent
    bool isSpent = checkIfDepositOutputsSpent(it->second.depositHash);
    
    if (isSpent && !it->second.isSpent) {
        logger(WARNING) << "Elderfier deposit spent - invalidating Elderfier status for: " 
                        << Common::podToHex(publicKey);
        
        // Mark deposit as spent
        const_cast<ElderfierDepositData&>(it->second).isSpent = true;
        const_cast<ElderfierDepositData&>(it->second).isActive = false;
    }
    
    return isSpent;
}

bool EldernodeIndexManager::isDepositStillValid(const Crypto::PublicKey& publicKey) const {
    auto it = m_elderfierDeposits.find(publicKey);
    if (it == m_elderfierDeposits.end()) {
        return false;
    }
    
    // Check if deposit is still valid (not spent)
    return it->second.isActive && !it->second.isSpent;
}

bool EldernodeIndexManager::checkIfDepositOutputsSpent(const Crypto::Hash& depositHash) const {
    // Use blockchain integration to check if deposit outputs are spent
    return isDepositTransactionSpent(depositHash);
}

ENindexEntry EldernodeIndexManager::createENindexEntryFromDeposit(const ElderfierDepositData& deposit) const {
    ENindexEntry entry;
    entry.eldernodePublicKey = deposit.elderfierPublicKey;
    entry.feeAddress = deposit.elderfierAddress;
    entry.stakeAmount = deposit.depositAmount;
    entry.registrationTimestamp = deposit.depositTimestamp;
    entry.isActive = true;
    entry.tier = EldernodeTier::ELDERFIER;
    entry.serviceId = deposit.serviceId;
    
    return entry;
}

// Security window helpers

void EldernodeIndexManager::updateSecurityWindowForDeposit(ElderfierDepositData& deposit, uint64_t currentTimestamp) const {
    // Update security window end time
    deposit.securityWindowEnd = calculateSecurityWindowEnd(deposit.lastSignatureTimestamp);
    
    // Check if currently in security window
    deposit.isInSecurityWindow = (currentTimestamp < deposit.securityWindowEnd);
    
    // Update unlock status
    deposit.isUnlocked = !deposit.isInSecurityWindow;
    
    // Update security window duration
    deposit.securityWindowDuration = SecurityWindow::DEFAULT_DURATION_SECONDS;
}

bool EldernodeIndexManager::validateSignatureTimestamp(uint64_t lastSignature, uint64_t currentTimestamp) const {
    // Prevent signature spam - minimum interval between signatures
    if (lastSignature > 0 && (currentTimestamp - lastSignature) < SecurityWindow::MINIMUM_SIGNATURE_INTERVAL) {
        return false;
    }
    
    // Prevent future timestamps
    if (currentTimestamp > (lastSignature + SecurityWindow::GRACE_PERIOD_SECONDS)) {
        return false;
    }
    
    return true;
}

uint64_t EldernodeIndexManager::calculateSecurityWindowEnd(uint64_t lastSignatureTimestamp) const {
    return lastSignatureTimestamp + SecurityWindow::DEFAULT_DURATION_SECONDS;
}

bool EldernodeIndexManager::validateUnlockRequest(const ElderfierDepositData& deposit, uint64_t timestamp) const {
    // Check if Elderfier is currently in security window
    if (deposit.isInSecurityWindow) {
        logger(WARNING) << "Cannot request unlock while in security window";
        return false;
    }
    
    // Check if unlock was already requested
    if (deposit.unlockRequested) {
        logger(WARNING) << "Unlock already requested";
        return false;
    }
    
    // Check if deposit is valid and active
    if (!deposit.isValid() || !deposit.isActive || deposit.isSpent) {
        logger(WARNING) << "Cannot unlock invalid deposit";
        return false;
    }
    
    return true;
}

} // namespace CryptoNote
