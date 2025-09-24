# Fuego's 𝞝lderfiers: 
## A Comprehensive Guide to On-Chain Verifiers

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Tier System](#tier-system)
4. [Service ID Types](#service-id-types)
5. [On-Chain Verification Process](#on-chain-verification-process)
6. [Implementation Details](#implementation-details)
7. [Consensus Mechanism](#consensus-mechanism)
8. [Security Features](#security-features)
9. [Usage Examples](#usage-examples)
10. [Integration Guide](#integration-guide)
11. [Configuration](#configuration)
12. [Monitoring & Metrics](#monitoring--metrics)
13. [Troubleshooting](#troubleshooting)
14. [Future Roadmap](#future-roadmap)

---

## Overview

**𝞝lderfiers** are Fuego's advanced on-chain input verifiers that represent a higher tier of Eldernodes in the Fuego blockchain ecosystem. They serve as distributed validators that provide cryptographic verification of blockchain transactions, particularly for cross-chain operations like the HEAT bridge system.

### Key Characteristics
- **Higher Tier**: Require 800 XFG minimum deposit (vs. 0 XFG for basic Eldernodes)
- **Deposit-Based System**: Uses 0x06 tag transactions for immediate unlock with spending validation
- **Flexible Service Identification**: Support custom names, hashed addresses, or standard addresses
- **On-Chain Verification**: Provide cryptographic proofs for transaction validation
- **Fast Consensus**: 2/2 fastpass with 4/5 fallback for robust validation
- **Enhanced Security**: Multi-layered validation with deposit-based incentives and Elder Council voting

### Core Purpose
𝞝lderfiers act as **on-chain input verifiers** by:
1. **Validating Transaction Data**: Cryptographically verify transaction integrity
2. **Forming Consensus**: Participate in distributed consensus for critical operations
3. **Generating Proofs**: Create verifiable proofs for cross-chain operations
4. **Maintaining Security**: Provide deposit-based security guarantees with Elder Council governance

---

## Architecture

### System Components

```
┌───────────────────────────────────────────────────────────┐
│                    Fuego Blockchain                       │
├───────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │   Basic     │  │ Elderfier   │  │ Elderfier   │        │
│  │ Eldernodes  │  │   Nodes     │  │   Nodes     │        │
│  │ (0 XFG)     │  │ (800+ XFG)  │  │ (800+ XFG)  │        │
│  │             │  │ 0x06 Tag    │  │ 0x06 Tag    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
├───────────────────────────────────────────────────────────┤
├───────────────────────────────────────────────────────────┤
├───────────────────────────────────────────────────────────┤
│              EldernodeIndexManager                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │   Service   │  │  Deposit    │  │ Consensus   │        │
│  │ ID Manager  │  │ Verifier    │  │ Manager     │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
├───────────────────────────────────────────────────────────┤
│              On-Chain Verification Layer                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ Transaction │  │   Proof     │  │ Consensus   │        │
│  │ Validator   │  │ Generator   │  │ Verifier    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└───────────────────────────────────────────────────────────┘
```

### Data Flow

```
User Transaction → Basic Validation → Elderfier Verification → Consensus Formation → On-Chain Proof
```

---

## Tier System

### Eldernode Tiers

| Tier | Minimum Deposit | Deposit Type | Service ID Options | Priority | Use Case |
|------|----------------|--------------|-------------------|----------|----------|
| **Basic** | **0 XFG** | None | Public wallet address | Standard | Lite-wallet (SPV) remote connection services |
| **𝞝lderfier** | **800 XFG** | 0x06 Tag Transaction | Custom name, hashed address, or standard address | **High** | Advanced verification and consensus |

### Tier Requirements

#### Basic Eldernodes
- **Deposit**: 0 XFG (fee address only)
- **Deposit Type**: None required
- **Service ID**: Public wallet address
- **Functionality**: Basic remote network service operations
- **Consensus**: Standard participation

#### 𝞝lderfier Nodes
- **Deposit**: 800 XFG minimum
- **Deposit Type**: 0x06 tag transaction (immediately unlocked)
- **Service ID**: Flexible options (custom name, hashed address, standard address)
- **Functionality**: Advanced verification, consensus leadership
- **Consensus**: Prioritized participation (2/2 fastpass, 4/5 fallback)
- **Unstaking**: 8-hour security window after last signature
- **Governance**: Elder Council voting for slashing decisions

---

## Service ID Types

𝞝lderfier nodes support three distinct service identification methods:

### 1. Custom Name (Exactly 8 Letters, All Caps)

```cpp
ElderfierServiceId serviceId = ElderfierServiceId::createCustomName("FIRENODE", walletAddress);
```

**Features:**
- **Exactly 8 letters** (no more, no less)
- **All uppercase letters only** (A-Z)
- **Alphabetic characters only** (no numbers, underscores, or hyphens)
- Reserved name protection (ADMIN, ROOT, SYSTEM, etc.)
- **Linked to actual wallet address** for verification
- Network registry visibility

**Validation Rules:**
- Must be exactly 8 characters
- Must be all uppercase letters (A-Z)
- Must not be a reserved name
- Must link to actual wallet address

**Example:**
```cpp
ENindexEntry entry;
entry.tier = EldernodeTier::ELDERFIER;
entry.serviceId = ElderfierServiceId::createCustomName("MYNODE00", "fire123456789abcdef");
// Service ID: "MYNODE00" (padded to 8 letters if needed)
// Linked Address: "fire123456789abcdef"
```

### 2. Hashed Public Fee Address (Privacy Option)

```cpp
ElderfierServiceId serviceId = ElderfierServiceId::createHashedAddress("fire123456789abcdef");
```

**Features:**
- SHA256 hash of the public fee address
- Privacy protection for fee address
- Masked display name (e.g., "FUEG...def")
- **Linked to actual wallet address** for verification
- Network registry shows hash, not original address

**Example:**
```cpp
ENindexEntry entry;
entry.tier = EldernodeTier::ELDERFIER;
entry.serviceId = ElderfierServiceId::createHashedAddress("fire123456789abcdef");
// Service ID: "a1b2c3d4e5f6..." (64-character hash)
// Display Name: "FUEG...def"
// Linked Address: "fire123456789abcdef"
```

### 3. Standard Address (Like Basic Eldernodes)

```cpp
ElderfierServiceId serviceId = ElderfierServiceId::createStandardAddress("fire123456789abcdef");
```

**Features:**
- Same as basic Eldernodes (public wallet address)
- Full transparency
- Compatible with existing systems
- No additional privacy features

---

## On-Chain Verification Process

### 1. Transaction Monitoring

𝞝lderfiers continuously monitor the Fuego blockchain for transactions requiring verification:

```cpp
class EldernodeTransactionMonitor {
    void monitorBurnTransactions() {
        // Monitor Fuego blockchain for new blocks
        for (const auto& block : blockchain.getNewBlocks()) {
            for (const auto& transaction : block.transactions) {
                if (hasHeatCommitment(transaction)) {
                    // Found a burn transaction with HEAT commitment
                    processBurnTransaction(transaction);
                }
            }
        }
    }
    
    bool hasHeatCommitment(const Transaction& tx) {
        // Check if transaction has 0x08 HEAT commitment in tx_extra
        for (const auto& extra : tx.extra) {
            if (extra.type == TX_EXTRA_HEAT_COMMITMENT) {
                return true;
            }
        }
        return false;
    }
};
```

### 2. Data Extraction and Validation

```cpp
void processBurnTransaction(const Transaction& tx) {
    // Extract HEAT commitment from tx_extra
    HeatCommitmentData commitment = extractHeatCommitment(tx.extra);
    
    // Validate transaction data
    if (!validateBurnTransaction(tx, commitment)) {
        return; // Invalid transaction, skip
    }
    
    // Verify on-chain data matches commitment
    if (!verifyOnChainData(tx, commitment)) {
        return; // Data mismatch, skip
    }
    
    // Queue for consensus formation
    queueForConsensus(tx, commitment);
}
```

### 3. Blockchain State Verification

```cpp
bool verifyBlockchainState(const Transaction& tx) {
    // Verify block is in main chain
    Block block = blockchain.getBlock(tx.blockHash);
    if (!blockchain.isMainChain(block)) {
        return false;
    }
    
    // Verify block has sufficient confirmations
    uint64_t currentHeight = blockchain.getCurrentHeight();
    if (currentHeight - block.height < MIN_CONFIRMATIONS) {
        return false;
    }
    
    // Verify block timestamp is reasonable
    uint64_t currentTime = getCurrentTimestamp();
    if (currentTime - block.timestamp > MAX_BLOCK_AGE) {
        return false;
    }
    
    // Verify transaction is included in block
    if (!block.containsTransaction(tx.hash)) {
        return false;
    }
    
    return true;
}
```

---

## Implementation Details

### Core Data Structures

#### ElderfierServiceId Structure

```cpp
struct ElderfierServiceId {
    ServiceIdType type;           // STANDARD_ADDRESS, CUSTOM_NAME, HASHED_ADDRESS
    std::string identifier;        // Raw identifier (address, name, or hash)
    std::string displayName;       // Human-readable display name
    std::string linkedAddress;     // Actual wallet address (for verification)
    std::string hashedAddress;     // SHA256 hash of the wallet address (for all Elderfier nodes)
    
    bool isValid() const;
    std::string toString() const;
    
    // Factory methods
    static ElderfierServiceId createStandardAddress(const std::string& address);
    static ElderfierServiceId createCustomName(const std::string& name, const std::string& walletAddress);
    static ElderfierServiceId createHashedAddress(const std::string& address);
};
```

#### ENindexEntry Structure

```cpp
struct ENindexEntry {
    Crypto::PublicKey eldernodePublicKey;
    std::string feeAddress;
    uint64_t stakeAmount;
    uint64_t registrationTimestamp;
    bool isActive;
    uint32_t consensusParticipationCount;
    std::chrono::system_clock::time_point lastActivity;
    EldernodeTier tier;
    ElderfierServiceId serviceId;  // Only used for ELDERFIER tier
    
    bool operator==(const ENindexEntry& other) const;
    bool operator<(const ENindexEntry& other) const;
};
```

#### Stake Proof Structure

```cpp
struct EldernodeStakeProof {
    Crypto::Hash stakeHash;
    Crypto::PublicKey eldernodePublicKey;
    uint64_t stakeAmount;
    uint64_t timestamp;
    std::vector<uint8_t> proofSignature;
    std::string feeAddress;
    EldernodeTier tier;
    ElderfierServiceId serviceId;  // Only used for ELDERFIER tier
    
    bool isValid() const;
    std::string toString() const;
};
```

### EldernodeIndexManager

The core manager class handles all 𝞝lderfier operations:

```cpp
class EldernodeIndexManager : public IEldernodeIndexManager {
public:
    // Core ENindex management
    bool addEldernode(const ENindexEntry& entry) override;
    bool removeEldernode(const Crypto::PublicKey& publicKey) override;
    bool updateEldernode(const ENindexEntry& entry) override;
    std::optional<ENindexEntry> getEldernode(const Crypto::PublicKey& publicKey) const override;
    
    // Elderfier-specific management
    std::vector<ENindexEntry> getElderfierNodes() const override;
    std::optional<ENindexEntry> getEldernodeByServiceId(const ElderfierServiceId& serviceId) const override;
    
    // Stake proof management
    bool addStakeProof(const EldernodeStakeProof& proof) override;
    bool verifyStakeProof(const EldernodeStakeProof& proof) const override;
    
    // Consensus management
    EldernodeConsensusResult reachConsensus(const std::vector<uint8_t>& data, const ConsensusThresholds& thresholds) override;
    
    // Statistics and monitoring
    uint32_t getElderfierNodeCount() const override;
    uint64_t getTotalStakeAmount() const override;
    
    // Slashing functionality
    bool slashEldernode(const Crypto::PublicKey& publicKey, const std::string& reason) override;
};
```

---

## Elderfier Deposit System

### 0x06 Tag Transaction Structure

Elderfier deposits use a special transaction extra tag (0x06) that provides immediate unlock with spending validation:

```cpp
struct TransactionExtraElderfierDeposit {
    Crypto::Hash depositHash;           // Unique deposit identifier
    uint64_t depositAmount;             // Deposit amount (800 XFG minimum)
    uint64_t timestamp;                 // Deposit creation timestamp
    std::string elderfierAddress;       // Elderfier wallet address
    std::vector<uint8_t> metadata;     // Additional metadata
    std::vector<uint8_t> signature;     // Deposit signature
    bool isUnlocked;                    // Always true - deposits are immediately unlocked
    
    bool serialize(ISerializer& serializer);
    bool isValid() const;
    std::string toString() const;
};
```

### Deposit Lifecycle

#### 1. Deposit Creation
```cpp
// Create Elderfier deposit transaction
bool createElderfierDeposit(const std::string& elderfierAddress, uint64_t amount) {
    TransactionExtraElderfierDeposit deposit;
    deposit.depositHash = generateDepositHash(elderfierAddress, amount);
    deposit.depositAmount = amount;
    deposit.timestamp = getCurrentTimestamp();
    deposit.elderfierAddress = elderfierAddress;
    deposit.isUnlocked = true; // Always immediately unlocked
    
    // Create transaction with 0x06 tag
    Transaction tx = createTransactionWithExtra(deposit);
    return broadcastTransaction(tx);
}
```

#### 2. Deposit Monitoring
```cpp
class ElderfierDepositMonitor {
    void monitorDepositSpending() {
        // Monitor for 0x06 tag spending transactions
        for (const auto& tx : getPendingTransactions()) {
            if (hasElderfierDepositTag(tx)) {
                processSpendingTransaction(tx);
            }
        }
    }
    
    void processSpendingTransaction(const Transaction& tx) {
        // Check if Elderfier is trying to spend deposit
        if (isElderfierSpendingDeposit(tx)) {
            // Trigger 8-hour security window
            triggerSecurityWindow(tx);
        }
    }
};
```

#### 3. Security Window System
```cpp
struct SecurityWindow {
    static constexpr uint64_t DURATION_HOURS = 8;
    static constexpr uint64_t DURATION_SECONDS = DURATION_HOURS * 3600;
    
    Crypto::Hash elderfierId;
    uint64_t startTime;
    uint64_t endTime;
    bool isActive;
    
    bool isExpired() const {
        return getCurrentTimestamp() > endTime;
    }
    
    uint64_t getRemainingTime() const {
        if (isExpired()) return 0;
        return endTime - getCurrentTimestamp();
    }
};
```

### Unstaking Process

#### 1. Unstaking Request
```cpp
bool requestUnstaking(const std::string& elderfierAddress) {
    // Check if Elderfier has valid deposit
    if (!hasValidDeposit(elderfierAddress)) {
        return false;
    }
    
    // Check last signature validity
    if (!validateLastSignature(elderfierAddress)) {
        // Trigger Elder Council vote
        triggerElderCouncilVote(elderfierAddress);
        return false;
    }
    
    // Start 8-hour security window
    startSecurityWindow(elderfierAddress);
    return true;
}
```

#### 2. Security Window Validation
```cpp
void validateSecurityWindow(const std::string& elderfierAddress) {
    SecurityWindow window = getSecurityWindow(elderfierAddress);
    
    if (window.isExpired()) {
        // Security window passed, allow unstaking
        allowUnstaking(elderfierAddress);
    } else {
        // Still in security window, check for invalid signatures
        if (hasInvalidSignatures(elderfierAddress)) {
            // Trigger Elder Council vote
            triggerElderCouncilVote(elderfierAddress);
        }
    }
}
```

### Elder Council Voting System

#### 1. Voting Message Structure
```cpp
struct ElderCouncilVotingMessage {
    std::string messageId;                    // Unique message identifier
    std::string targetElderfier;             // Elderfier being voted on
    std::string subject;                     // Vote subject line
    std::string description;                  // Detailed description
    uint64_t votingDeadline;                  // Voting deadline timestamp
    bool isRead;                              // Whether message has been read
    bool hasVoted;                            // Whether Elderfier has voted
    bool hasConfirmedVote;                    // Whether vote has been confirmed
    ElderCouncilVoteType pendingVoteType;     // Pending vote (before confirmation)
    ElderCouncilVoteType confirmedVoteType;   // Confirmed vote (after confirmation)
    std::vector<ElderCouncilVote> votes;     // All votes cast
    uint32_t requiredVotes;                  // Required votes for quorum
    uint32_t currentVotes;                    // Current vote count
    
    bool isValid() const;
    std::string toString() const;
};
```

#### 2. Vote Types
```cpp
enum class ElderCouncilVoteType : uint8_t {
    SLASH_ALL = 1,      // Slash/burn ALL of Elderfier's stake
    SLASH_HALF = 2,     // Slash/burn HALF of Elderfier's stake  
    SLASH_NONE = 3      // Slash/burn NONE of Elderfier's stake
};
```

#### 3. Two-Step Vote Confirmation
```cpp
class ElderCouncilVoting {
    bool submitVote(const std::string& messageId, ElderCouncilVoteType voteType) {
        // Set pending vote (not yet confirmed)
        m_pendingVotes[messageId] = voteType;
        return true;
    }
    
    bool confirmVote(const std::string& messageId) {
        auto it = m_pendingVotes.find(messageId);
        if (it == m_pendingVotes.end()) {
            return false; // No pending vote
        }
        
        // Confirm the vote
        ElderCouncilVote vote;
        vote.voteType = it->second;
        vote.voterId = getCurrentElderfierId();
        vote.timestamp = getCurrentTimestamp();
        
        // Add to voting message
        addVoteToMessage(messageId, vote);
        
        // Remove from pending votes
        m_pendingVotes.erase(it);
        return true;
    }
    
    bool cancelPendingVote(const std::string& messageId) {
        return m_pendingVotes.erase(messageId) > 0;
    }
};
```

---

## Consensus Mechanism

### 1. Fast Consensus Formation Process

```cpp
class EldernodeConsensusManager {
    void formConsensus(const Transaction& tx, const HeatCommitmentData& commitment) {
        // Create consensus request
        ConsensusRequest request = createConsensusRequest(tx, commitment);
        
        // Try 2/2 fastpass first (Elderfier nodes only)
        std::vector<ConsensusResponse> fastpassResponses = tryFastPassConsensus(request);
        
        if (fastpassResponses.size() >= 2) {
            // Fastpass consensus achieved
            EldernodeConsensusProof proof = createConsensusProof(request, fastpassResponses);
            proof.consensusType = ConsensusType::FASTPASS_2_2;
            storeConsensusProof(proof);
            return;
        }
        
        // Fallback to 4/5 robust consensus
        std::vector<ConsensusResponse> robustResponses = tryRobustConsensus(request);
        
        if (robustResponses.size() >= 4) {
            // Robust consensus achieved
            EldernodeConsensusProof proof = createConsensusProof(request, robustResponses);
            proof.consensusType = ConsensusType::ROBUST_4_5;
            storeConsensusProof(proof);
            return;
        }
        
        // Consensus failed
        handleConsensusFailure(request);
    }
    
    std::vector<ConsensusResponse> tryFastPassConsensus(const ConsensusRequest& request) {
        // Only query Elderfier nodes for fastpass
        std::vector<ElderfierNode> elderfierNodes = getActiveElderfierNodes();
        
        std::vector<ConsensusResponse> responses;
        for (const auto& node : elderfierNodes) {
            ConsensusResponse response = queryNode(node, request);
            if (response.isValid()) {
                responses.push_back(response);
            }
            
            // Stop at 2 responses for fastpass
            if (responses.size() >= 2) {
                break;
            }
        }
        
        return responses;
    }
    
    std::vector<ConsensusResponse> tryRobustConsensus(const ConsensusRequest& request) {
        // Query all active Eldernodes for robust consensus
        std::vector<Eldernode> allNodes = getActiveEldernodes();
        
        std::vector<ConsensusResponse> responses;
        for (const auto& node : allNodes) {
            ConsensusResponse response = queryNode(node, request);
            if (response.isValid()) {
                responses.push_back(response);
            }
            
            // Stop at 5 responses for robust consensus
            if (responses.size() >= 5) {
                break;
            }
        }
        
        return responses;
    }
};
```

### 2. Consensus Thresholds

```cpp
enum class ConsensusType : uint8_t {
    FASTPASS_2_2 = 1,    // 2/2 Elderfier fastpass consensus
    ROBUST_4_5 = 2       // 4/5 robust fallback consensus
};

struct ConsensusThresholds {
    uint32_t fastpassMinElderfiers;    // Minimum Elderfiers for fastpass (2)
    uint32_t fastpassRequiredAgreement; // Required fastpass agreement (2)
    uint32_t robustMinEldernodes;      // Minimum Eldernodes for robust (5)
    uint32_t robustRequiredAgreement;  // Required robust agreement (4)
    uint32_t fastpassTimeoutSeconds;   // Fastpass timeout (10 seconds)
    uint32_t robustTimeoutSeconds;     // Robust timeout (30 seconds)
    uint32_t retryAttempts;            // Retry attempts (3)
    
    static ConsensusThresholds getDefault();
    bool isValid() const;
};
```

**Default Configuration:**
- **Fastpass Consensus**: 2 out of 2 Elderfier nodes (100% consensus, 10s timeout)
- **Robust Fallback**: 4 out of 5 Eldernodes (80% consensus, 30s timeout)
- **Retry Attempts**: 3
- **Priority**: Elderfier nodes prioritized in both consensus types

### 3. Consensus Result Structure

```cpp
struct EldernodeConsensusResult {
    bool consensusReached;
    ConsensusType consensusType;           // FASTPASS_2_2 or ROBUST_4_5
    uint32_t requiredThreshold;           // Required votes for this consensus type
    uint32_t actualVotes;                 // Actual votes received
    std::vector<Crypto::PublicKey> participatingEldernodes;
    std::vector<uint8_t> aggregatedSignature;
    uint64_t consensusTimestamp;
    uint64_t fastpassAttemptTime;         // Time spent on fastpass attempt
    uint64_t robustAttemptTime;           // Time spent on robust attempt
    
    bool isValid() const;
    std::string toString() const;
    bool isFastpass() const { return consensusType == ConsensusType::FASTPASS_2_2; }
    bool isRobust() const { return consensusType == ConsensusType::ROBUST_4_5; }
};
```

### 4. Tier Prioritization

𝞝lderfier nodes are automatically prioritized in consensus operations:

```cpp
// Elderfier nodes appear first in sorted lists
std::sort(activeParticipants.begin(), activeParticipants.end());
// ELDERFIER > BASIC (regardless of stake amount)
```

---

## Security Features

### 1. Cryptographic Security

- **ECDSA Signatures**: Standard cryptographic signatures with recovery
- **Hash Functions**: Keccak256 for message hashing
- **Key Management**: Secure private key storage for Eldernodes
- **Signature Verification**: On-chain verification of all signatures

### 2. Deposit-Based Security

- **Minimum Deposit**: 800 XFG required for 𝞝lderfier nodes (0x06 tag transaction)
- **Immediate Unlock**: Deposits are immediately unlocked but monitored for spending
- **Security Window**: 8-hour buffer period after last signature before unstaking
- **Spending Validation**: Continuous monitoring of 0x06 tag spending transactions
- **Elder Council Slashing**: Decentralized voting system for slashing decisions
- **Two-Step Confirmation**: Vote submission followed by explicit confirmation

### 3. Consensus Security

- **Threshold Requirements**: Minimum consensus threshold prevents manipulation
- **Active Eldernode Validation**: Only currently active Eldernodes can participate
- **Timestamp Validation**: Prevents replay of old consensus proofs
- **Deposit Verification**: Eldernodes must maintain valid 0x06 tag deposits

### 4. Network Security

- **Sybil Resistance**: Deposit-based Eldernode selection
- **Byzantine Fault Tolerance**: Consensus threshold provides fault tolerance
- **Network Partition Handling**: Graceful degradation with insufficient consensus
- **Replay Protection**: Network ID and timestamp validation

### 5. Slashing Configuration

```cpp
enum class SlashingDestination : uint8_t {
    BURN = 0,           // Burn slashed stakes (remove from circulation)
    TREASURY = 1,       // Send to network treasury address
    REDISTRIBUTE = 2,   // Redistribute to other Eldernodes
    CHARITY = 3         // Send to charity/community fund address
};

struct SlashingConfig {
    SlashingDestination destination;
    std::string destinationAddress; // Address for treasury/charity
    uint64_t slashingPercentage;    // Percentage of stake to slash (e.g., 50 = 50%)
    bool enableSlashing;            // Whether slashing is enabled
    
    static SlashingConfig getDefault();
    bool isValid() const;
};
```

---

## Usage Examples

### 1. Creating an 𝞝lderfier Node with Custom Name

```cpp
EldernodeIndexManager manager;

ENindexEntry elderfierEntry;
Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
elderfierEntry.feeAddress = "fire987654321fedcba";
elderfierEntry.stakeAmount = 800000000; // 800 XFG minimum deposit
elderfierEntry.tier = EldernodeTier::ELDERFIER;
elderfierEntry.serviceId = ElderfierServiceId::createCustomName("FIRENODE", "fire987654321fedcba");
elderfierEntry.isActive = true;

bool success = manager.addEldernode(elderfierEntry);
```

### 2. Creating an 𝞝lderfier Node with Hashed Address

```cpp
ENindexEntry privacyEntry;
Crypto::generate_keys(privacyEntry.eldernodePublicKey, privacyEntry.eldernodeSecretKey);
privacyEntry.feeAddress = "fire555666777888999";
privacyEntry.stakeAmount = 1000000000; // 1000 XFG deposit
privacyEntry.tier = EldernodeTier::ELDERFIER;
privacyEntry.serviceId = ElderfierServiceId::createHashedAddress("fire555666777888999");
privacyEntry.isActive = true;

bool success = manager.addEldernode(privacyEntry);
```

### 3. Creating a Basic Eldernode (No Stake)

```cpp
ENindexEntry basicEntry;
Crypto::generate_keys(basicEntry.eldernodePublicKey, basicEntry.eldernodeSecretKey);
basicEntry.feeAddress = "fire123456789abcdef";
basicEntry.stakeAmount = 0; // No deposit required for basic Eldernodes
basicEntry.tier = EldernodeTier::BASIC;
basicEntry.isActive = true;

bool success = manager.addEldernode(basicEntry);
```

### 4. Looking Up 𝞝lderfier Nodes

```cpp
// Get all 𝞝lderfier nodes
auto elderfierNodes = manager.getElderfierNodes();

// Get specific 𝞝lderfier by service ID
auto serviceId = ElderfierServiceId::createCustomName("FIRENODE", "fire987654321fedcba");
auto node = manager.getEldernodeByServiceId(serviceId);

// Get statistics
uint32_t elderfierCount = manager.getElderfierNodeCount();
```

### 5. Stake Proof Generation

```cpp
bool generateFreshProof(const Crypto::PublicKey& publicKey, const std::string& feeAddress) {
    auto it = m_eldernodes.find(publicKey);
    if (it == m_eldernodes.end()) {
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
    
    // Generate signature
    proof.proofSignature.resize(64, 0);
    
    m_stakeProofs[publicKey].push_back(proof);
    return true;
}
```

### 6. Consensus Formation

```cpp
EldernodeConsensusResult reachConsensus(const std::vector<uint8_t>& data, const ConsensusThresholds& thresholds) {
    EldernodeConsensusResult result;
    result.consensusReached = false;
    result.requiredThreshold = thresholds.requiredAgreement;
    
    // Get active participants (prioritize 𝞝lderfier nodes)
    std::vector<EldernodeConsensusParticipant> activeParticipants;
    for (const auto& pair : m_consensusParticipants) {
        if (pair.second.isActive) {
            activeParticipants.push_back(pair.second);
        }
    }
    
    // Sort by tier (𝞝lderfier first) and stake amount
    std::sort(activeParticipants.begin(), activeParticipants.end());
    
    if (activeParticipants.size() < thresholds.minimumEldernodes) {
        return result;
    }
    
    // Simulate consensus voting (prioritize 𝞝lderfier nodes)
    std::vector<std::vector<uint8_t>> signatures;
    for (const auto& participant : activeParticipants) {
        // Generate signature
        Crypto::Hash dataHash;
        Crypto::cn_fast_hash(data.data(), data.size(), dataHash);
        
        std::vector<uint8_t> signature(64, 0); // Placeholder signature
        signatures.push_back(signature);
        result.participatingEldernodes.push_back(participant.publicKey);
    }
    
    result.actualVotes = static_cast<uint32_t>(signatures.size());
    
    // Check if consensus threshold is met
    if (result.actualVotes >= thresholds.requiredAgreement) {
        result.consensusReached = true;
        result.aggregatedSignature = aggregateSignatures(signatures);
    }
    
    return result;
}
```

---

## Integration Guide

### 1. HEAT Bridge Integration

𝞝lderfiers provide verification for the HEAT bridge system:

```solidity
function verifyEldernodeConsensus(bytes32 commitment, bytes calldata eldernodeProof) 
    internal view returns (bool) {
    
    // Parse the consensus proof
    (bytes32[] memory eldernodeIds, 
     bytes[] memory signatures, 
     bytes32 messageHash, 
     uint64 timestamp) = eldernodeVerifier.parseEldernodeProof(eldernodeProof);
    
    // Verify consensus threshold
    require(eldernodeIds.length >= MIN_ELDERNODE_CONSENSUS, "Insufficient consensus");
    
    // Verify each signature
    for (uint i = 0; i < eldernodeIds.length; i++) {
        require(
            eldernodeVerifier.verifyEldernodeSignature(
                eldernodeIds[i], 
                messageHash, 
                signatures[i]
            ),
            "Invalid Eldernode signature"
        );
        
        require(
            eldernodeVerifier.isEldernodeActive(eldernodeIds[i]),
            "Inactive Eldernode"
        );
    }
    
    // Verify timestamp freshness
    require(block.timestamp - timestamp <= MAX_CONSENSUS_AGE, "Stale consensus");
    
    return true;
}
```

### 2. Multi-Layer Validation

The complete verification process includes:

1. **STARK Proof Verification**: Mathematical proof of computational integrity
2. **𝞝lderfier Consensus**: Distributed validation by trusted network nodes
3. **Nullifier Check**: Prevention of double-spending
4. **Network ID Validation**: Cross-chain replay protection

### 3. Combined Verification in L2 Contract

```solidity
function claimHEAT(
    bytes32 secret,
    bytes calldata proof,           // STARK proof
    bytes32[] calldata publicInputs,
    address recipient,
    bool isLargeBurn,
    bytes calldata eldernodeProof,  // 𝞝lderfier consensus proof
    uint256 l1GasFee
) external payable {
    
    // 1. Verify STARK proof (computational integrity)
    bool starkValid = verifyStarkProof(proof, publicInputs);
    require(starkValid, "Invalid STARK proof");
    
    // 2. Verify 𝞝lderfier consensus (distributed validation)
    if (eldernodeVerificationRequired) {
        bool eldernodeValid = verifyEldernodeConsensus(commitment, eldernodeProof);
        require(eldernodeValid, "𝞝lderfier consensus failed");
    }
    
    // 3. Proceed with HEAT minting
    // ...
}
```

---

## Configuration

### 1. ElderfierServiceConfig

```cpp
struct ElderfierServiceConfig {
    uint64_t minimumDepositAmount = 800000000;     // 800 XFG minimum (800 * 1,000,000)
    uint64_t customNameLength = 8;                 // Exactly 8 letters
    bool allowHashedAddresses = true;              // Enable privacy option
    std::vector<std::string> reservedNames;        // Protected names
    SlashingConfig slashingConfig;                 // Slashing configuration
    SecurityWindowConfig securityWindowConfig;     // Security window configuration
    ElderCouncilConfig elderCouncilConfig;         // Elder Council voting configuration
    
    static ElderfierServiceConfig getDefault();
    bool isValid() const;
    bool isCustomNameReserved(const std::string& name) const;
    bool isValidCustomName(const std::string& name) const;
};
```

### 2. Reserved Names Protection

The following names are reserved and cannot be used as custom names:
- `ADMIN`, `ROOT`, `SYSTEM`, `FUEGO`, `ELDER`, `NODE`
- `TEST`, `DEV`, `MAIN`, `PROD`, `SERVER`, `CLIENT`
- `MASTER`, `SLAVE`, `BACKUP`, `CACHE`, `DB`, `API`, `WEB`, `APP`

### 3. Configuration Management

```cpp
void EldernodeIndexManager::setElderfierConfig(const ElderfierServiceConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_elderfierConfig = config;
}

ElderfierServiceConfig EldernodeIndexManager::getElderfierConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_elderfierConfig;
}
```

---

## Monitoring & Metrics

### 1. Consensus Metrics

- **Participation Rate**: Percentage of active 𝞝lderfiers participating
- **Consensus Time**: Time to reach consensus threshold
- **Failure Rate**: Rate of consensus verification failures
- **Network Health**: Overall 𝞝lderfier network health

### 2. Security Metrics

- **Signature Verification Rate**: Rate of successful signature verifications
- **Stake Distribution**: Distribution of stake across 𝞝lderfiers
- **Attack Detection**: Detection of potential consensus attacks
- **Network Resilience**: Ability to maintain consensus under stress

### 3. Performance Metrics

- **Gas Optimization**: Gas usage for consensus verification
- **Network Performance**: Consensus formation speed
- **Throughput**: Transactions verified per second
- **Latency**: Time from request to consensus

### 4. Monitoring Implementation

```cpp
class EldernodeMetrics {
public:
    void recordConsensusParticipation(uint32_t participating, uint32_t total);
    void recordConsensusTime(uint64_t timeMs);
    void recordVerificationFailure(const std::string& reason);
    void recordStakeDistribution(const std::map<uint64_t, uint32_t>& distribution);
    
    MetricsReport generateReport() const;
    
private:
    std::atomic<uint64_t> m_totalConsensusAttempts{0};
    std::atomic<uint64_t> m_successfulConsensus{0};
    std::atomic<uint64_t> m_totalConsensusTime{0};
    std::atomic<uint64_t> m_verificationFailures{0};
};
```

---

## Troubleshooting

### 1. Common Issues

#### Insufficient Deposit
**Problem**: Eldernode rejected due to insufficient deposit
**Solution**: Ensure deposit amount is at least 800 XFG (800,000,000 atomic units)

```cpp
if (entry.depositAmount < m_elderfierConfig.minimumDepositAmount) {
    logger(ERROR) << "Elderfier node deposit too low: " << entry.depositAmount
                 << " < " << m_elderfierConfig.minimumDepositAmount << " (800 XFG)";
    return false;
}
```

#### Invalid Service ID
**Problem**: Custom name validation failed
**Solution**: Ensure custom name is exactly 8 letters, all caps, and not reserved

```cpp
bool isValidCustomName(const std::string& name) const {
    if (name.length() != 8) return false;
    if (!std::all_of(name.begin(), name.end(), ::isupper)) return false;
    if (!std::all_of(name.begin(), name.end(), ::isalpha)) return false;
    return !isCustomNameReserved(name);
}
```

#### Service ID Conflict
**Problem**: Duplicate service ID detected
**Solution**: Choose a unique service ID or use a different type

```cpp
bool hasServiceIdConflict(const ElderfierServiceId& serviceId, const Crypto::PublicKey& excludeKey) const {
    for (const auto& pair : m_eldernodes) {
        if (pair.first != excludeKey && 
            pair.second.tier == EldernodeTier::ELDERFIER &&
            pair.second.serviceId.identifier == serviceId.identifier) {
            return true;
        }
    }
    return false;
}
```

#### Consensus Failure
**Problem**: Insufficient consensus reached
**Solution**: Check network connectivity and Eldernode availability

```cpp
if (activeParticipants.size() < thresholds.minimumEldernodes) {
    logger(WARNING) << "Insufficient active Eldernodes for consensus: " 
                   << activeParticipants.size() << "/" << thresholds.minimumEldernodes;
    return result;
}
```

### 2. Debugging Tools

#### Logging Configuration

```cpp
class EldernodeLogger {
public:
    void setLogLevel(LogLevel level);
    void enableDebugMode(bool enable);
    void logConsensusDetails(const EldernodeConsensusResult& result);
    void logStakeVerification(const EldernodeStakeProof& proof);
};
```

#### Health Check

```cpp
class EldernodeHealthCheck {
public:
    HealthStatus checkNetworkHealth() const;
    HealthStatus checkConsensusHealth() const;
    HealthStatus checkStakeHealth() const;
    
    struct HealthStatus {
        bool isHealthy;
        std::string status;
        std::vector<std::string> issues;
        std::map<std::string, double> metrics;
    };
};
```

### 3. Recovery Procedures

#### Automatic Recovery

```cpp
class EldernodeRecovery {
public:
    bool recoverFromConsensusFailure();
    bool recoverFromStakeVerificationFailure();
    bool recoverFromNetworkPartition();
    
private:
    bool retryConsensusWithBackupNodes();
    bool regenerateStakeProofs();
    bool reconnectToNetwork();
};
```

---

## Future Roadmap

### 1. Advanced Consensus

- **Weighted Consensus**: Stake-weighted voting instead of equal voting
- **Dynamic Thresholds**: Adaptive consensus thresholds based on network conditions
- **Multi-Round Consensus**: Multi-round consensus for complex decisions
- **Consensus Committees**: Rotating committees for improved efficiency

### 2. Enhanced Security

- **Threshold Signatures**: BLS threshold signatures for improved efficiency
- **Zero-Knowledge Proofs**: ZK proofs for Eldernode consensus
- **Advanced Cryptography**: Post-quantum cryptographic algorithms
- **Formal Verification**: Formal verification of consensus protocols

### 3. Service ID Enhancements

- **Service ID Transfer**: Allow service ID transfer between operators
- **Custom Name Auctions**: Auction system for premium custom names
- **Service ID Expiration**: Time-limited service IDs
- **Enhanced Privacy**: Additional privacy features for hashed addresses
- **Service ID Categories**: Categorize service IDs by type or region

### 4. Performance Improvements

- **Parallel Processing**: Parallel consensus formation
- **Caching**: Advanced caching mechanisms
- **Compression**: Efficient data compression
- **Batching**: Batch processing capabilities

### 5. Integration Enhancements

- **Multi-Chain Support**: Support for additional blockchain networks
- **Cross-Chain Consensus**: Consensus across multiple chains
- **Advanced APIs**: Enhanced API capabilities
- **SDK Development**: Comprehensive SDK for developers

---

## Conclusion

Fuego's 𝞝lderfiers represent a sophisticated on-chain input verification system that provides:

- **Higher Tier Security**: 800 XFG minimum deposit requirement
- **Flexible Service Identification**: Multiple service ID options
- **Advanced Consensus**: Prioritized participation in consensus mechanisms
- **Comprehensive Validation**: Multi-layered transaction verification
- **Robust Security**: Stake-based incentives and slashing mechanisms

The system is designed to be:
- **Secure**: Cryptographic verification of all consensus data
- **Scalable**: Efficient processing of consensus proofs
- **Resilient**: Graceful handling of network failures
- **Transparent**: Clear audit trail of all consensus decisions
- **Upgradable**: Support for future enhancements and improvements

𝞝lderfiers serve as the backbone of Fuego's on-chain verification infrastructure, providing the security and reliability needed for critical operations like cross-chain bridges and distributed consensus mechanisms.

---

## Additional Resources

- [Elderfier Service Nodes Documentation](docs/ELDERFIER_SERVICE_NODES.md)
- [Eldernode Proof Requirements](docs/ELDERNODE_PROOF_REQUIREMENTS.md)
- [Eldernode Implementation Summary](docs/ELDERNODE_IMPLEMENTATION_SUMMARY.md)
- [Test Suite](tests/ENindexTest/EldernodeIndexTest.cpp)

For technical support and questions, please refer to the Fuego documentation or contact the development team.
