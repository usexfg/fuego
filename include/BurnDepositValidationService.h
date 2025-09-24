#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>

#include "crypto/hash.h"
#include "crypto/crypto.h"
#include "Common/StringTools.h"
#include "EldernodeIndexTypes.h"

namespace CryptoNote {

struct BurnDepositValidationResult {
    bool isValid;
    std::string errorMessage;
    uint64_t validatedAmount;
    Crypto::Hash burnProofHash;
    uint64_t timestamp;
    bool commitmentMatch;
    bool burnAmountMatch;
    std::string txExtraCommitment;
    uint64_t txBurnAmount;
    static BurnDepositValidationResult success(uint64_t amount, const Crypto::Hash& hash, uint64_t time, bool commitMatch, bool amountMatch, const std::string& txCommitment, uint64_t txAmount);
    static BurnDepositValidationResult failure(const std::string& error);
};

struct BurnDepositConfig {
    uint64_t minimumBurnAmount;           // Minimum burn amount (atomic units)
    uint64_t maximumBurnAmount;           // Maximum burn amount (atomic units)
    uint32_t proofExpirationSeconds;      // Proof expiration time
    bool requireProofValidation;          // Require proof validation
    std::string treasuryAddress;          // Treasury address for proofs
    // Consensus thresholds
    uint32_t fastPassConsensusThreshold;  // 3/3 FastPass consensus threshold
    uint32_t fallbackConsensusThreshold;  // 5/7 fallback consensus threshold
    uint32_t fullConsensusThreshold;      // 7/10 full quorum consensus threshold
    uint32_t totalEldernodes;             // Total Eldernodes (10)
    // Enable flags
    bool enableDualValidation;            // Both commitment and burn amount validation
    bool enableFastPass;                  // Enable FastPass consensus
    // Confirmation block counts per consensus path
    uint32_t fastPassConfirmationBlocks;  // 3 block confirmations for FastPass
    uint32_t fallbackConfirmationBlocks;  // 6 block confirmations for fallback consensus
    uint32_t fullConfirmationBlocks;      // 9 block confirmations for full quorum consensus
    // Dynamic quorum fractions (0.0 - 1.0)
    double fastPassFraction;              // Fraction of active Eldernodes required for FastPass (typically 1.0)
    double fallbackFraction;              // Fraction of active Eldernodes queried for fallback (e.g. 0.5)
    double fallbackMatchFraction;         // Fraction of total Eldernodes that must agree for fallback (e.g. 0.80)
    double fullQuorumFraction;            // Fraction of active Eldernodes queried for full quorum (typically 1.0)
    double fullQuorumMatchFraction;       // Fraction of total Eldernodes that must agree for full quorum (e.g. 0.69)
    // Proof fee configuration (atomic units)
    uint64_t smallBurnProofFee;           // Fee for standard burn proofs (0.008 XFG = 80000 atomic units)
    uint64_t largeBurnProofFee;           // Fee for large burn proofs (0.8 XFG = 8000000 atomic units)
    static BurnDepositConfig getDefault();
    bool isValid() const;
};

struct BurnProofData {
    Crypto::Hash burnHash;
    uint64_t burnAmount;
    uint64_t timestamp;
    std::vector<uint8_t> proofSignature;
    std::string depositorAddress;
    std::string treasuryAddress;
    std::string commitment;       // 32-byte hex string commitment
    std::string txHash;          // Fuego transaction hash
    bool isValid() const;
    std::string toString() const;
};

struct EldernodeConsensus {
    std::vector<std::string> eldernodeIds;
    std::vector<std::string> signatures;
    std::vector<std::string> agreeingEldernodeIds; // Eldernodes whose proofs matched consensus
    std::string messageHash;
    uint64_t timestamp;
    uint32_t fastPassConsensusThreshold;     // 2/2 fast pass threshold
    uint32_t fallbackConsensusThreshold; // 4/5 fallback threshold
    uint32_t totalEldernodes;
    bool fastPassUsed;                   // Whether fast pass (2/2) was used
    bool fallbackPathUsed;               // Whether fallback path (4/5) was used
    BurnProofData verifiedInputs;
    std::string txExtraCommitment;  // Commitment extracted from tx_extra
    uint64_t txBurnAmount;          // Burn amount from transaction (undefined output key)
    bool commitmentMatch;           // Whether commitments match
    bool burnAmountMatch;          // Whether burn amounts match
    uint64_t totalFeeDistributed;   // Total fee distributed among agreeing Eldernodes
    uint64_t perEldernodeFee;       // Fee paid per agreeing Eldernode
    bool isValid() const;
    std::string toString() const;
};

struct EldernodeVerificationInputs {
    std::string txHash;           // Fuego transaction hash
    std::string commitment;       // The commitment as a whole (32-byte hex string)
    uint64_t burnAmount;          // Burn amount (amount with undefined output key)
    bool isValid() const;
    std::string toString() const;
};

class core;
class IEldernodeIndexManager;

class IBurnDepositValidationService {
public:
    virtual ~IBurnDepositValidationService() = default;
    virtual BurnDepositValidationResult validateBurnDeposit(const BurnProofData& proof) = 0;
    virtual bool verifyBurnProof(const BurnProofData& proof) = 0;
    virtual std::optional<BurnProofData> generateBurnProof(uint64_t amount, const std::string& depositorAddress, const std::string& commitment, const std::string& txHash) = 0;
    virtual void setBurnDepositConfig(const BurnDepositConfig& config) = 0;
    virtual BurnDepositConfig getBurnDepositConfig() const = 0;
    virtual uint64_t getTotalBurnedAmount() const = 0;
    virtual uint32_t getTotalBurnProofs() const = 0;
    virtual std::vector<BurnProofData> getRecentBurnProofs(uint32_t count) const = 0;
    
    // Eldernode consensus methods
    virtual std::optional<EldernodeConsensus> requestEldernodeConsensus(const EldernodeVerificationInputs& inputs) = 0;
    virtual bool verifyEldernodeConsensus(const EldernodeConsensus& consensus) = 0;
    virtual std::string extractCommitmentFromTxExtra(const std::string& txHash) = 0;
    virtual uint64_t extractBurnAmountFromTransaction(const std::string& txHash) = 0;
    virtual bool verifyCommitmentMatch(const std::string& providedCommitment, const std::string& txExtraCommitment) = 0;
    virtual bool verifyBurnAmountMatch(uint64_t providedAmount, uint64_t txBurnAmount) = 0;
};

class BurnDepositValidationService : public IBurnDepositValidationService {
public:
    explicit BurnDepositValidationService(core& core, std::shared_ptr<IEldernodeIndexManager> eldernodeManager);
    ~BurnDepositValidationService();
    
    BurnDepositValidationResult validateBurnDeposit(const BurnProofData& proof) override;
    bool verifyBurnProof(const BurnProofData& proof) override;
    std::optional<BurnProofData> generateBurnProof(uint64_t amount, const std::string& depositorAddress, const std::string& commitment, const std::string& txHash) override;
    void setBurnDepositConfig(const BurnDepositConfig& config) override;
    BurnDepositConfig getBurnDepositConfig() const override;
    uint64_t getTotalBurnedAmount() const override;
    uint32_t getTotalBurnProofs() const override;
    std::vector<BurnProofData> getRecentBurnProofs(uint32_t count) const override;
    
    // Eldernode consensus methods
    std::optional<EldernodeConsensus> requestEldernodeConsensus(const EldernodeVerificationInputs& inputs) override;
    bool verifyEldernodeConsensus(const EldernodeConsensus& consensus) override;
    std::string extractCommitmentFromTxExtra(const std::string& txHash) override;
    uint64_t extractBurnAmountFromTransaction(const std::string& txHash) override;
    bool verifyCommitmentMatch(const std::string& providedCommitment, const std::string& txExtraCommitment) override;
    bool verifyBurnAmountMatch(uint64_t providedAmount, uint64_t txBurnAmount) override;

private:
    core& m_core;
    std::shared_ptr<IEldernodeIndexManager> m_eldernodeManager;
    BurnDepositConfig m_config;
    std::vector<BurnProofData> m_burnProofs;
    uint64_t m_totalBurnedAmount;
    uint64_t distributeProofFees(const BurnProofData& proof, const std::vector<std::string>& agreeingEldernodes);
    
    bool validateBurnAmount(uint64_t amount) const;
    bool validateBurnProofSignature(const BurnProofData& proof) const;
    Crypto::Hash calculateBurnHash(uint64_t amount, const std::string& depositorAddress, uint64_t timestamp) const;
    bool isProofExpired(const BurnProofData& proof) const;
    
    // Eldernode consensus helpers
    std::vector<EldernodeConsensusParticipant> getEldernodeConsensusParticipants() const;
    bool validateEldernodeSignatures(const EldernodeConsensus& consensus) const;
    std::string calculateConsensusMessageHash(const EldernodeVerificationInputs& inputs) const;
    bool checkConsensusThreshold(const EldernodeConsensus& consensus) const;
};

} // namespace CryptoNote
