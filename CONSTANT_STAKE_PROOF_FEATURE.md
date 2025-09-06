# Constant Stake Proof Feature for Elderado C0DL3 Validator

## Overview

This document describes the new **Constant Stake Proof** feature that allows Elderfier nodes to create persistent stake proofs for cross-chain validation, specifically for **Elderado validator stake on C0DL3 (zkSync)**.

## Feature Summary

- **Purpose**: Enable Elderfier nodes to create constant (persistent) stake proofs for cross-chain validation
- **Target**: Elderado validator stake on C0DL3 (zkSync) network
- **Required Stake**: 8000 XFG minimum for Elderado C0DL3 validator
- **Duration**: Constant (never expires by default, configurable)
- **Access**: Only available to Elderfier tier nodes (800 XFG minimum)

## Architecture

### Constant Stake Proof Types

```cpp
enum class ConstantStakeProofType : uint8_t {
    NONE = 0,                    // No constant stake proof
    ELDERADO_C0DL3_VALIDATOR = 1 // Elderado validator stake for C0DL3 (zkSync) - 8000 XFG
};
```

### Enhanced Stake Proof Structure

```cpp
struct EldernodeStakeProof {
    // ... existing fields ...
    ConstantStakeProofType constantProofType; // Constant stake proof type
    std::string crossChainAddress;             // Address on target chain (C0DL3/zkSync)
    uint64_t constantStakeAmount;              // Amount locked for constant proof (8000 XFG)
    uint64_t constantProofExpiry;              // Expiry timestamp (0 = never expires)
    
    bool isConstantProof() const;
    bool isConstantProofExpired() const;
};
```

### Enhanced ENindex Entry Structure

```cpp
struct ENindexEntry {
    // ... existing fields ...
    ConstantStakeProofType constantProofType; // Constant stake proof type
    std::string crossChainAddress;             // Address on target chain
    uint64_t constantStakeAmount;              // Amount locked for constant proof
    uint64_t constantProofExpiry;              // Expiry timestamp
    
    bool hasConstantProof() const;
    bool isConstantProofExpired() const;
};
```

## Configuration

### Constant Stake Proof Configuration

```cpp
struct ConstantStakeProofConfig {
    bool enableElderadoC0DL3Validator;     // Enable Elderado validator stake for C0DL3
    uint64_t elderadoC0DL3StakeAmount;     // Required stake amount (8000 XFG)
    uint64_t constantProofValidityPeriod; // Validity period (0 = never expires)
    std::string c0dl3NetworkId;           // C0DL3 network identifier
    std::string c0dl3ContractAddress;     // C0DL3 validator contract address
    bool allowConstantProofRenewal;       // Allow renewal of constant proofs
    
    static ConstantStakeProofConfig getDefault();
    bool isValid() const;
    uint64_t getRequiredStakeAmount(ConstantStakeProofType type) const;
};
```

### Default Configuration

```cpp
ConstantStakeProofConfig ConstantStakeProofConfig::getDefault() {
    ConstantStakeProofConfig config;
    config.enableElderadoC0DL3Validator = true;  // Enabled by default
    config.elderadoC0DL3StakeAmount = 8000000000; // 8000 XFG (8000 * 1,000,000)
    config.constantProofValidityPeriod = 0;      // 0 = never expires (constant)
    config.c0dl3NetworkId = "C0DL3_MAINNET";     // C0DL3 network identifier
    config.c0dl3ContractAddress = "0x0000000000000000000000000000000000000000"; // Placeholder
    config.allowConstantProofRenewal = true;      // Allow renewal
    return config;
}
```

## API Reference

### Creating Constant Stake Proof

```cpp
bool createConstantStakeProof(
    const Crypto::PublicKey& publicKey,
    ConstantStakeProofType proofType,
    const std::string& crossChainAddress,
    uint64_t stakeAmount
);
```

**Parameters:**
- `publicKey`: Eldernode public key
- `proofType`: Type of constant proof (ELDERADO_C0DL3_VALIDATOR)
- `crossChainAddress`: Address on target chain (C0DL3/zkSync)
- `stakeAmount`: Amount to lock for constant proof (minimum 8000 XFG)

**Returns:** `true` if successful, `false` otherwise

**Requirements:**
- Eldernode must be Elderfier tier (800 XFG minimum)
- Eldernode must have sufficient total stake
- Constant proof type must be enabled
- No existing constant proof of same type

### Renewing Constant Stake Proof

```cpp
bool renewConstantStakeProof(
    const Crypto::PublicKey& publicKey,
    ConstantStakeProofType proofType
);
```

**Parameters:**
- `publicKey`: Eldernode public key
- `proofType`: Type of constant proof to renew

**Returns:** `true` if successful, `false` otherwise

**Requirements:**
- Constant proof renewal must be enabled
- Existing constant proof must exist
- Constant proof must not be expired

### Revoking Constant Stake Proof

```cpp
bool revokeConstantStakeProof(
    const Crypto::PublicKey& publicKey,
    ConstantStakeProofType proofType
);
```

**Parameters:**
- `publicKey`: Eldernode public key
- `proofType`: Type of constant proof to revoke

**Returns:** `true` if successful, `false` otherwise

### Retrieving Constant Stake Proofs

```cpp
// Get all constant proofs for an Eldernode
std::vector<EldernodeStakeProof> getConstantStakeProofs(const Crypto::PublicKey& publicKey) const;

// Get constant proofs by type
std::vector<EldernodeStakeProof> getConstantStakeProofsByType(ConstantStakeProofType proofType) const;
```

## Usage Examples

### Creating Elderado C0DL3 Validator Constant Proof

```cpp
EldernodeIndexManager manager;

// Ensure Elderfier node has sufficient stake
ENindexEntry elderfierEntry;
elderfierEntry.tier = EldernodeTier::ELDERFIER;
elderfierEntry.stakeAmount = 10000000000; // 10000 XFG (more than 8000 XFG required)
// ... other fields ...

bool added = manager.addEldernode(elderfierEntry);
assert(added);

// Create constant stake proof for Elderado C0DL3 validator
std::string c0dl3Address = "0x742d35Cc6634C0532925a3b8D4C9db96C4b4d8b7";
uint64_t constantStakeAmount = 8000000000; // 8000 XFG

bool created = manager.createConstantStakeProof(
    elderfierEntry.eldernodePublicKey,
    ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR,
    c0dl3Address,
    constantStakeAmount
);

if (created) {
    std::cout << "Constant stake proof created successfully!" << std::endl;
} else {
    std::cout << "Failed to create constant stake proof" << std::endl;
}
```

### Verifying Constant Proof

```cpp
// Get constant proofs for Eldernode
auto constantProofs = manager.getConstantStakeProofs(elderfierEntry.eldernodePublicKey);

for (const auto& proof : constantProofs) {
    if (proof.constantProofType == ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR) {
        std::cout << "Elderado C0DL3 Validator proof found:" << std::endl;
        std::cout << "  Cross-chain address: " << proof.crossChainAddress << std::endl;
        std::cout << "  Constant stake amount: " << proof.constantStakeAmount << " XFG" << std::endl;
        std::cout << "  Is constant proof: " << (proof.isConstantProof() ? "Yes" : "No") << std::endl;
        std::cout << "  Is expired: " << (proof.isConstantProofExpired() ? "Yes" : "No") << std::endl;
    }
}
```

### Renewing Constant Proof

```cpp
// Renew constant proof (updates timestamp and expiry)
bool renewed = manager.renewConstantStakeProof(
    elderfierEntry.eldernodePublicKey,
    ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR
);

if (renewed) {
    std::cout << "Constant stake proof renewed successfully!" << std::endl;
} else {
    std::cout << "Failed to renew constant stake proof" << std::endl;
}
```

### Revoking Constant Proof

```cpp
// Revoke constant proof
bool revoked = manager.revokeConstantStakeProof(
    elderfierEntry.eldernodePublicKey,
    ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR
);

if (revoked) {
    std::cout << "Constant stake proof revoked successfully!" << std::endl;
} else {
    std::cout << "Failed to revoke constant stake proof" << std::endl;
}
```

## Validation Rules

### Constant Proof Creation Validation

1. **Eldernode Tier**: Only Elderfier nodes can create constant proofs
2. **Sufficient Stake**: Eldernode must have sufficient total stake
3. **Minimum Amount**: Constant stake amount must meet minimum requirements (8000 XFG for Elderado)
4. **No Duplicates**: Cannot create duplicate constant proof of same type
5. **Feature Enabled**: Constant proof type must be enabled in configuration

### Constant Proof Validation

1. **Expiry Check**: Constant proof must not be expired (unless never expires)
2. **Amount Validation**: Constant stake amount must meet requirements
3. **Address Validation**: Cross-chain address must not be empty
4. **Type Validation**: Constant proof type must be valid and enabled

## Storage and Persistence

### Enhanced Storage Format

The storage format has been enhanced to include constant proof data:

```cpp
// Write constant stake proof data
file.write(reinterpret_cast<const char*>(&entry.constantProofType), sizeof(entry.constantProofType));
uint32_t crossChainAddressSize = static_cast<uint32_t>(entry.crossChainAddress.size());
file.write(reinterpret_cast<const char*>(&crossChainAddressSize), sizeof(crossChainAddressSize));
file.write(entry.crossChainAddress.c_str(), crossChainAddressSize);
file.write(reinterpret_cast<const char*>(&entry.constantStakeAmount), sizeof(entry.constantStakeAmount));
file.write(reinterpret_cast<const char*>(&entry.constantProofExpiry), sizeof(entry.constantProofExpiry));
```

### Backward Compatibility

- Existing storage files without constant proof data will load successfully
- New fields will be initialized with default values
- Constant proof fields are optional and don't break existing functionality

## Integration with C0DL3/zkSync

### Cross-Chain Address Format

The `crossChainAddress` field stores the address on the target chain (C0DL3/zkSync):

```cpp
// Example C0DL3/zkSync address
std::string c0dl3Address = "0x742d35Cc6634C0532925a3b8D4C9db96C4b4d8b7";
```

### Network Configuration

```cpp
ConstantStakeProofConfig config;
config.c0dl3NetworkId = "C0DL3_MAINNET";     // Network identifier
config.c0dl3ContractAddress = "0x...";       // Validator contract address
```

### Proof Verification on C0DL3

The constant stake proof can be used on C0DL3/zkSync to verify:

1. **Stake Amount**: 8000 XFG minimum stake
2. **Eldernode Identity**: Public key and service ID
3. **Proof Validity**: Cryptographic signature and timestamp
4. **Cross-Chain Link**: Connection between Fuego and C0DL3 addresses

## Security Considerations

### Stake Locking

- Constant stake proofs lock a portion of the Eldernode's stake
- Locked stake cannot be used for other purposes while constant proof is active
- Revoking constant proof releases the locked stake

### Cross-Chain Security

- Cross-chain addresses are validated but not cryptographically verified
- Constant proofs provide cryptographic proof of stake on Fuego chain
- C0DL3/zkSync contracts should verify proof signatures and timestamps

### Expiry Management

- Constant proofs can be configured to never expire (default)
- Expiry timestamps are validated during proof verification
- Expired proofs are automatically rejected

## Error Handling

### Common Error Cases

1. **Insufficient Stake**: Eldernode doesn't have enough total stake
2. **Wrong Tier**: Basic Eldernode trying to create constant proof
3. **Duplicate Proof**: Attempting to create duplicate constant proof
4. **Feature Disabled**: Constant proof type not enabled in configuration
5. **Invalid Address**: Empty or invalid cross-chain address

### Error Messages

```cpp
logger(ERROR) << "Eldernode total stake insufficient for constant proof: " 
             << entry.stakeAmount << " < " << stakeAmount;

logger(ERROR) << "Only Elderfier nodes can create constant stake proofs: " 
             << Common::podToHex(publicKey);

logger(ERROR) << "Constant proof of type " << static_cast<int>(proofType) 
             << " already exists for Eldernode: " << Common::podToHex(publicKey);
```

## Testing

### Test Coverage

The implementation includes comprehensive tests covering:

1. **Creation**: Successful constant proof creation
2. **Validation**: Proof verification and validation
3. **Retrieval**: Getting constant proofs by Eldernode and type
4. **Renewal**: Renewing existing constant proofs
5. **Revocation**: Revoking constant proofs
6. **Error Cases**: Insufficient stake, wrong tier, duplicates
7. **Edge Cases**: Expired proofs, disabled features

### Test Example

```cpp
void testConstantStakeProofFunctionality() {
    EldernodeIndexManager manager;
    
    // Create Elderfier node with sufficient stake
    ENindexEntry elderfierEntry;
    elderfierEntry.tier = EldernodeTier::ELDERFIER;
    elderfierEntry.stakeAmount = 10000000000; // 10000 XFG
    // ... other fields ...
    
    // Test constant proof creation
    bool created = manager.createConstantStakeProof(
        elderfierEntry.eldernodePublicKey,
        ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR,
        "0x742d35Cc6634C0532925a3b8D4C9db96C4b4d8b7",
        8000000000 // 8000 XFG
    );
    assert(created);
    
    // Test retrieval
    auto proofs = manager.getConstantStakeProofs(elderfierEntry.eldernodePublicKey);
    assert(proofs.size() == 1);
    assert(proofs[0].isConstantProof());
    
    // Test revocation
    bool revoked = manager.revokeConstantStakeProof(
        elderfierEntry.eldernodePublicKey,
        ConstantStakeProofType::ELDERADO_C0DL3_VALIDATOR
    );
    assert(revoked);
}
```

## Future Enhancements

### Additional Constant Proof Types

The system is designed to support additional constant proof types:

```cpp
enum class ConstantStakeProofType : uint8_t {
    NONE = 0,
    ELDERADO_C0DL3_VALIDATOR = 1,
    // Future types can be added here
    ELDERADO_ARBITRUM_VALIDATOR = 2,
    ELDERADO_POLYGON_VALIDATOR = 3,
    // ...
};
```

### Enhanced Configuration

Future enhancements could include:

- **Dynamic Stake Amounts**: Configurable stake amounts per proof type
- **Multiple Proofs**: Support for multiple constant proofs per Eldernode
- **Proof Transfer**: Transfer constant proofs between Eldernodes
- **Proof Auctions**: Auction system for premium constant proof slots

### Cross-Chain Integration

Enhanced cross-chain integration could include:

- **Automated Verification**: Automatic proof verification on target chains
- **Proof Synchronization**: Real-time synchronization of proof status
- **Multi-Chain Support**: Support for multiple target chains simultaneously

## Conclusion

The Constant Stake Proof feature provides a robust foundation for cross-chain validation, specifically enabling Elderfier nodes to participate as validators on C0DL3/zkSync through Elderado. The implementation includes:

- **Secure Stake Locking**: 8000 XFG minimum stake requirement
- **Flexible Configuration**: Configurable proof types and parameters
- **Comprehensive Validation**: Multi-layer validation and security checks
- **Persistent Storage**: Enhanced storage format with backward compatibility
- **Extensive Testing**: Comprehensive test coverage for all functionality

This feature enables seamless integration between Fuego's Elderfier network and external blockchain networks, providing a secure and efficient mechanism for cross-chain validator participation.