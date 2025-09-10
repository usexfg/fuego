# CURA Colored-Coin Implementation (Transaction Extra Type 0x09)

## Overview

This document describes the implementation of the CURA colored-coin transaction extra type (0x09) in the Fuego blockchain. The CURA colored-coin is designed to handle curation metadata and curator verification for the DIGM platform.

## Transaction Extra Type Definitions

The following transaction extra types are now implemented:

- `TX_EXTRA_YIELD_COMMITMENT` = 0x07 (Yield commitment schemes)
- `TX_EXTRA_HEAT_COMMITMENT` = 0x08 (HEAT commitment schemes)  
- `TX_EXTRA_CURA_COLORED_COIN` = 0x09 (CURA colored-coin for curation)

## CURA Colored-Coin Structure

```cpp
struct TransactionExtraCuraColoredCoin {
  std::string curationData;        // Curation metadata (JSON string)
  Crypto::PublicKey curatorKey;    // Public key of the curator
  Crypto::Signature curatorSig;   // Curator's signature over curationData
  uint64_t timestamp;              // Unix timestamp of curation
  uint32_t version;                // Version of the CURA colored-coin protocol
  
  bool serialize(ISerializer& serializer);
};
```

## Fields Description

### curationData
- **Type**: `std::string`
- **Purpose**: Contains JSON-formatted curation metadata
- **Example**: `{"albumId": "album_001", "curatorId": "curator_abc", "rating": 5, "notes": "Excellent album"}`

### curatorKey
- **Type**: `Crypto::PublicKey`
- **Purpose**: Public key of the curator who performed the curation
- **Usage**: Used to verify the curator's signature and identity

### curatorSig
- **Type**: `Crypto::Signature`
- **Purpose**: Cryptographic signature by the curator over the curationData
- **Verification**: Ensures the curation data hasn't been tampered with

### timestamp
- **Type**: `uint64_t`
- **Purpose**: Unix timestamp when the curation was performed
- **Usage**: Provides temporal ordering of curation events

### version
- **Type**: `uint32_t`
- **Purpose**: Version of the CURA colored-coin protocol
- **Usage**: Allows for future protocol upgrades and backward compatibility

## API Functions

### Core Functions
```cpp
bool appendCuraColoredCoinToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraCuraColoredCoin& cura_tag);
bool getCuraColoredCoinFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraCuraColoredCoin& cura_tag);
```

### Usage Example
```cpp
// Create CURA colored-coin data
TransactionExtraCuraColoredCoin curaTag;
curaTag.curationData = "{\"albumId\": \"album_001\", \"rating\": 5}";
curaTag.curatorKey = curatorPublicKey;
curaTag.curatorSig = signatureOverCurationData;
curaTag.timestamp = getCurrentTimestamp();
curaTag.version = 1;

// Add to transaction extra
std::vector<uint8_t> txExtra;
appendCuraColoredCoinToExtra(txExtra, curaTag);

// Later, extract from transaction extra
TransactionExtraCuraColoredCoin extractedTag;
if (getCuraColoredCoinFromExtra(txExtra, extractedTag)) {
    // Process curation data
    std::cout << "Curation data: " << extractedTag.curationData << std::endl;
}
```

## Integration with DIGM Platform

The CURA colored-coin enables:

1. **Curator Verification**: Proves that curation was performed by a legitimate curator
2. **Data Integrity**: Ensures curation metadata hasn't been modified
3. **Temporal Ordering**: Provides timestamp for curation events
4. **Protocol Evolution**: Version field allows for future upgrades

## Implementation Details

### Files Modified
- `src/CryptoNoteCore/TransactionExtra.h` - Added struct definition and function declarations
- `src/CryptoNoteCore/TransactionExtra.cpp` - Added parsing, serialization, and helper functions

### Transaction Extra Variant
The CURA type has been added to the boost::variant:
```cpp
typedef boost::variant<
    TransactionExtraPadding, 
    TransactionExtraPublicKey, 
    TransactionExtraNonce, 
    TransactionExtraMergeMiningTag, 
    tx_extra_message, 
    TransactionExtraTTL, 
    TransactionExtraHeatCommitment, 
    TransactionExtraYieldCommitment, 
    TransactionExtraCuraColoredCoin
> TransactionExtraField;
```

### Parsing Implementation
The CURA type is handled in the `parseTransactionExtra` function switch statement:
```cpp
case TX_EXTRA_CURA_COLORED_COIN:
{
  TransactionExtraCuraColoredCoin curaTag;
  ar(curaTag, "cura_tag");
  transactionExtraFields.push_back(curaTag);
  break;
}
```

### Serialization Implementation
The visitor pattern handles CURA serialization:
```cpp
bool operator()(const TransactionExtraCuraColoredCoin &t)
{
  return appendCuraColoredCoinToExtra(extra, t);
}
```

## Security Considerations

1. **Signature Verification**: Always verify the curator signature against the public key
2. **Timestamp Validation**: Check timestamp is reasonable (not too far in past/future)
3. **Data Size Limits**: Ensure curationData doesn't exceed reasonable size limits
4. **Version Compatibility**: Handle unknown version numbers gracefully

## Future Enhancements

The version field enables future protocol improvements such as:
- Enhanced metadata schemas
- Additional signature schemes
- Multi-curator endorsements
- Cross-chain curation support

## Testing

To test the implementation:
1. Create test transactions with CURA colored-coin data
2. Verify parsing and serialization round-trip correctly
3. Test signature verification with valid/invalid signatures
4. Validate error handling for malformed data

## Status

- ✅ Basic structure implemented
- ✅ Parsing and serialization added
- ✅ Integration with transaction extra system
- ✅ Documentation completed
- ⏳ Testing and validation pending
- ⏳ Integration with DIGM curation system pending

## Related Documents

- [DIGM Development Guide](../../docs/DEV_IMPLEMENTATION_PLAN.md)
- [Transaction Extra System Documentation](./TransactionExtra.h)
- [Fuego Blockchain Architecture](../../../docs/)
