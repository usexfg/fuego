
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include "crypto/hash.h"
#include "crypto/crypto.h"

namespace CryptoNote {

// Burn deposit validation result
struct BurnDepositValidationResult {
    bool isValid;
    std::string errorMessage;
    uint64_t validatedAmount;
    Crypto::Hash burnProofHash;
    uint64_t timestamp;
    
    static BurnDepositValidationResult success(uint64_t amount, const Crypto::Hash& hash, uint64_t time);
    static BurnDepositValidationResult failure(const std::string& error);
};

// Burn deposit configuration
struct BurnDepositConfig {
    uint64_t minimumBurnAmount;      // Minimum burn amount required
    uint64_t maximumBurnAmount;      // Maximum burn amount allowed
    uint32_t proofExpirationSeconds; // How long burn proofs are valid
    bool requireProofValidation;     // Whether to require proof validation
    std::string treasuryAddress;     // Treasury address for burn deposits
    
    static BurnDepositConfig getDefault();
    bool isValid() const;
};

// Burn proof data structure
struct BurnProofData {
    Crypto::Hash burnHash;
    uint64_t burnAmount;
    uint64_t timestamp;
    std::vector<uint8_t> proofSignature;
    std::string depositorAddress;
    std::string treasuryAddress;
    
    bool isValid() const;
    std::string toString() const;
};

// Forward declarations
class core;

class IBurnDepositValidationService {
public:
    virtual ~IBurnDepositValidationService() = default;
    
    // Core validation methods
    virtual BurnDepositValidationResult validateBurnDeposit(const BurnProofData& proof) = 0;
    virtual bool verifyBurnProof(const BurnProofData& proof) = 0;
    virtual std::optional<BurnProofData> generateBurnProof(uint64_t amount, const std::string& depositorAddress) = 0;
    
    // Configuration
    virtual void setBurnDepositConfig(const BurnDepositConfig& config) = 0;
    virtual BurnDepositConfig getBurnDepositConfig() const = 0;
    
    // Statistics and monitoring
    virtual uint64_t getTotalBurnedAmount() const = 0;
    virtual uint32_t getTotalBurnProofs() const = 0;
    virtual std::vector<BurnProofData> getRecentBurnProofs(uint32_t count) const = 0;
};

class BurnDepositValidationService : public IBurnDepositValidationService {
public:
    explicit BurnDepositValidationService(core& core);
    ~BurnDepositValidationService();
    
    // Core validation methods
    BurnDepositValidationResult validateBurnDeposit(const BurnProofData& proof) override;
    bool verifyBurnProof(const BurnProofData& proof) override;
    std::optional<BurnProofData> generateBurnProof(uint64_t amount, const std::string& depositorAddress) override;
    
    // Configuration
    void setBurnDepositConfig(const BurnDepositConfig& config) override;
    BurnDepositConfig getBurnDepositConfig() const override;
    
    // Statistics and monitoring
    uint64_t getTotalBurnedAmount() const override;
    uint32_t getTotalBurnProofs() const override;
    std::vector<BurnProofData> getRecentBurnProofs(uint32_t count) const override;

private:
    core& m_core;
    BurnDepositConfig m_config;
    std::vector<BurnProofData> m_burnProofs;
    uint64_t m_totalBurnedAmount;
    
    // Helper methods
    bool validateBurnAmount(uint64_t amount) const;
    bool validateBurnProofSignature(const BurnProofData& proof) const;
    Crypto::Hash calculateBurnHash(uint64_t amount, const std::string& depositorAddress, uint64_t timestamp) const;
    bool isProofExpired(const BurnProofData& proof) const;
};

} // namespace CryptoNote
