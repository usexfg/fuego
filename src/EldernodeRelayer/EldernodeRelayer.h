// Copyright (c) 2017-2025 Elderfire Privacy Council
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "../../include/IEldernodeRelayer.h"
#include "../../include/INode.h"
#include "../../include/IBlockchainExplorer.h"
#include "../Common/BlockingQueue.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace CryptoNote {

class EldernodeRelayer : public IEldernodeRelayer {
public:
    explicit EldernodeRelayer(
        INode& node,
        IBlockchainExplorer& explorer,
        const std::string& bridgeContractAddress,
        const std::string& arbitrumRpcUrl
    );
    
    ~EldernodeRelayer() override;

    // IEldernodeRelayer interface
    void startMonitoring(std::function<void(const BurnDeposit&)> callback) override;
    void stopMonitoring() override;
    MerkleProof generateMerkleProof(const std::string& txnHash, uint64_t blockHeight) override;
    bool submitToBridge(const BridgeProof& proof);
    bool isMonitoring() const override;
    std::string getStatistics() const override;

private:
    // Monitoring thread function
    void monitoringThread();
    
    // Process new blocks for burn deposits
    void processBlock(uint64_t blockHeight);
    
    // Check if transaction is a burn deposit
    bool isBurnDeposit(const Transaction& tx);
    
    // Extract burn deposit data from transaction
    BurnDeposit extractBurnDeposit(const Transaction& tx, uint64_t blockHeight, const std::string& blockHash);
    
    // Generate Merkle tree for a block
    std::vector<std::string> generateMerkleTree(const std::vector<Transaction>& transactions);
    
    // Submit proof to Arbitrum bridge
    bool submitToArbitrumBridge(const BridgeProof& proof);
    
    // HTTP client for Arbitrum communication
    bool sendHttpRequest(const std::string& url, const std::string& data, std::string& response);

    // Helper methods for validation proof generation
    bool verifyTransactionExists(const std::string& txnHash);
    BurnDeposit extractBurnDeposit(const std::string& txnHash);
    bool verifyBurnAmount(uint64_t amount);
    bool verifySecretExists(const std::string& txnHash, const std::vector<uint8_t>& secret);
    uint64_t getBlockHeight(const std::string& txnHash);
    std::string getBlockHash(uint64_t blockHeight);
    uint64_t getCurrentHeight();
    MerkleProof generateValidationMerkleProof(const std::string& txnHash, uint64_t blockHeight);
    std::string signValidationData(const std::string& txnHash, uint64_t amount, uint64_t blockHeight, const std::string& blockHash);

private:
    INode& m_node;
    IBlockchainExplorer& m_explorer;
    std::string m_bridgeContractAddress;
    std::string m_arbitrumRpcUrl;
    
    // Monitoring state
    std::atomic<bool> m_isMonitoring{false};
    std::thread m_monitoringThread;
    std::mutex m_monitoringMutex;
    std::condition_variable m_monitoringCondition;
    
    // Callback for new burn deposits
    std::function<void(const BurnDeposit&)> m_callback;
    
    // Statistics
    std::atomic<uint64_t> m_totalProofsProcessed{0};
    std::atomic<uint64_t> m_totalProofsSubmitted{0};
    std::atomic<uint64_t> m_totalProofsFailed{0};
    
    // Queue for pending proofs
    std::queue<BridgeProof> m_pendingProofs;
    std::mutex m_queueMutex;
    
    // Last processed block height
    std::atomic<uint64_t> m_lastProcessedBlock{0};
    
    // Eldernode cryptographic keys and identification
    Crypto::SecretKey m_eldernodePrivateKey;
    Crypto::PublicKey m_eldernodePublicKey;
    std::string m_eldernodeId;
    std::string m_eldernodeVersion;
    
    // Key management methods
    bool loadKeysFromFile(const std::string& privateKeyPath, const std::string& passphrase);
    bool generateNewKeyPair();
    std::string getEldernodeId() const;
    std::string getEldernodeVersion() const;
    
    // Consensus configuration
    uint64_t m_consensusThreshold;
    uint64_t m_minEldernodes;
    std::vector<std::string> m_knownEldernodes;
    
    // Consensus methods
    bool loadEldernodeRegistry();
    std::vector<std::string> selectRandomEldernodes(uint64_t count);
    bool validateConsensusProofs(const std::vector<ValidationProof>& proofs);
    
    // Enhanced signing methods
    std::string signValidationData(const std::string& txnHash, uint64_t amount, uint64_t blockHeight, const std::string& blockHash);
    std::string createSignaturePayload(const std::string& txnHash, uint64_t amount, uint64_t blockHeight, const std::string& blockHash) const;
    bool verifySignature(const std::string& data, const std::string& signature, const std::string& publicKey) const;
    
    // Merkle tree calculation helper
    std::vector<std::string> calculateMerklePath(const std::vector<Crypto::Hash>& hashes, size_t position);
};

} // namespace CryptoNote
