# Elderfier Service Nodes - Higher Tier Eldernodes

## Overview

Elderfier service nodes represent a new **higher tier** of Eldernodes in the Fuego blockchain, offering enhanced functionality and flexible service identification options. Unlike basic Eldernodes that use public wallet addresses for service identification, Elderfier operators have multiple options for their service ID in the network registry.

## Tier System

### Eldernode Tiers

```cpp
enum class EldernodeTier : uint8_t {
    BASIC = 0,           // Basic Eldernode (original tier)
    ELDERFIER = 1        // Elderfier service node (higher tier)
};
```

### Tier Requirements

| Tier | Minimum Stake | Service ID Options | Priority |
|------|---------------|-------------------|----------|
| **Basic** | 1 FUEGO | Public wallet address only | Standard |
| **Elderfier** | 5 FUEGO | Custom name, hashed address, or standard address | **High** |

## Service ID Options for Elderfier Nodes

Elderfier operators can choose from three different service ID types:

### 1. **Custom Name** (Up to 8 characters)
```cpp
ElderfierServiceId serviceId = ElderfierServiceId::createCustomName("FuegoNode");
```

**Features:**
- Human-readable custom names (1-8 characters)
- Alphanumeric characters, underscores, and hyphens allowed
- Reserved name protection (admin, root, system, etc.)
- Network registry visibility

**Example:**
```cpp
ENindexEntry entry;
entry.tier = EldernodeTier::ELDERFIER;
entry.serviceId = ElderfierServiceId::createCustomName("MyNode");
// Service ID: "MyNode"
```

### 2. **Hashed Public Fee Address** (Privacy Option)
```cpp
ElderfierServiceId serviceId = ElderfierServiceId::createHashedAddress("FUEGO123456789abcdef");
```

**Features:**
- SHA256 hash of the public fee address
- Privacy protection for fee address
- Masked display name (e.g., "FUEG...def")
- Network registry shows hash, not original address

**Example:**
```cpp
ENindexEntry entry;
entry.tier = EldernodeTier::ELDERFIER;
entry.serviceId = ElderfierServiceId::createHashedAddress("FUEGO123456789abcdef");
// Service ID: "a1b2c3d4e5f6..." (64-character hash)
// Display Name: "FUEG...def"
```

### 3. **Standard Address** (Like Basic Eldernodes)
```cpp
ElderfierServiceId serviceId = ElderfierServiceId::createStandardAddress("FUEGO123456789abcdef");
```

**Features:**
- Same as basic Eldernodes (public wallet address)
- Full transparency
- Compatible with existing systems
- No additional privacy features

## Implementation Details

### Service ID Structure

```cpp
struct ElderfierServiceId {
    ServiceIdType type;           // STANDARD_ADDRESS, CUSTOM_NAME, HASHED_ADDRESS
    std::string identifier;       // Raw identifier (address, name, or hash)
    std::string displayName;      // Human-readable display name
    
    bool isValid() const;
    std::string toString() const;
    
    // Factory methods
    static ElderfierServiceId createStandardAddress(const std::string& address);
    static ElderfierServiceId createCustomName(const std::string& name);
    static ElderfierServiceId createHashedAddress(const std::string& address);
};
```

### Configuration

```cpp
struct ElderfierServiceConfig {
    uint64_t minimumStakeAmount = 5000000;      // 5 FUEGO minimum
    uint64_t maximumCustomNameLength = 8;        // Max 8 characters
    bool allowHashedAddresses = true;            // Enable privacy option
    std::vector<std::string> reservedNames;     // Protected names
};
```

### Reserved Names Protection

The following names are reserved and cannot be used as custom names:
- `admin`, `root`, `system`, `fuego`, `elder`, `node`
- `test`, `dev`, `main`, `prod`

## Enhanced Features

### 1. **Tier Prioritization**
Elderfier nodes are automatically prioritized in consensus operations:

```cpp
// Elderfier nodes appear first in sorted lists
std::sort(activeParticipants.begin(), activeParticipants.end());
// ELDERFIER > BASIC (regardless of stake amount)
```

### 2. **Service ID Conflict Detection**
Prevents duplicate service IDs across Elderfier nodes:

```cpp
bool hasServiceIdConflict(const ElderfierServiceId& serviceId, const Crypto::PublicKey& excludeKey) const;
```

### 3. **Enhanced Validation**
Tier-specific validation rules:

```cpp
bool validateEldernodeEntry(const ENindexEntry& entry) const {
    if (entry.tier == EldernodeTier::ELDERFIER) {
        if (entry.stakeAmount < m_elderfierConfig.minimumStakeAmount) {
            return false; // Insufficient stake for Elderfier
        }
        if (!entry.serviceId.isValid()) {
            return false; // Invalid service ID
        }
    }
    return true;
}
```

### 4. **Service ID Lookup**
Find Elderfier nodes by their service ID:

```cpp
std::optional<ENindexEntry> getEldernodeByServiceId(const ElderfierServiceId& serviceId) const;
```

## Usage Examples

### Creating an Elderfier Node with Custom Name

```cpp
EldernodeIndexManager manager;

ENindexEntry elderfierEntry;
Crypto::generate_keys(elderfierEntry.eldernodePublicKey, elderfierEntry.eldernodeSecretKey);
elderfierEntry.feeAddress = "FUEGO987654321fedcba";
elderfierEntry.stakeAmount = 5000000; // 5 FUEGO minimum
elderfierEntry.tier = EldernodeTier::ELDERFIER;
elderfierEntry.serviceId = ElderfierServiceId::createCustomName("FuegoNode");
elderfierEntry.isActive = true;

bool success = manager.addEldernode(elderfierEntry);
```

### Creating an Elderfier Node with Hashed Address

```cpp
ENindexEntry privacyEntry;
Crypto::generate_keys(privacyEntry.eldernodePublicKey, privacyEntry.eldernodeSecretKey);
privacyEntry.feeAddress = "FUEGO555666777888999";
privacyEntry.stakeAmount = 10000000; // 10 FUEGO
privacyEntry.tier = EldernodeTier::ELDERFIER;
privacyEntry.serviceId = ElderfierServiceId::createHashedAddress("FUEGO555666777888999");
privacyEntry.isActive = true;

bool success = manager.addEldernode(privacyEntry);
```

### Looking Up Elderfier Nodes

```cpp
// Get all Elderfier nodes
auto elderfierNodes = manager.getElderfierNodes();

// Get specific Elderfier by service ID
auto serviceId = ElderfierServiceId::createCustomName("FuegoNode");
auto node = manager.getEldernodeByServiceId(serviceId);

// Get statistics
uint32_t elderfierCount = manager.getElderfierNodeCount();
```

## Network Registry Integration

Elderfier service IDs are automatically added to the network registry with the following information:

### Registry Entry Structure
```json
{
  "tier": "ELDERFIER",
  "serviceId": {
    "type": "CUSTOM_NAME",
    "identifier": "FuegoNode",
    "displayName": "FuegoNode"
  },
  "publicKey": "a1b2c3d4...",
  "stakeAmount": 5000000,
  "feeAddress": "FUEGO987654321fedcba",
  "isActive": true
}
```

### Privacy Considerations

1. **Custom Names**: Fully public, human-readable identifiers
2. **Hashed Addresses**: Privacy-preserving, original address not visible
3. **Standard Addresses**: Same transparency as basic Eldernodes

## Benefits

### For Elderfier Operators
- **Flexibility**: Choose service ID type based on privacy needs
- **Branding**: Use custom names for recognition
- **Privacy**: Option to hide fee addresses
- **Priority**: Higher priority in consensus operations

### For Network
- **Enhanced Security**: Higher stake requirements
- **Better Organization**: Human-readable service identifiers
- **Privacy Options**: Support for privacy-conscious operators
- **Scalability**: Tiered system for future expansion

## Testing

Comprehensive test suite covers:

1. **Basic Eldernode Operations**: Standard tier functionality
2. **Elderfier Service Node Creation**: All three service ID types
3. **Service ID Validation**: Custom name length, reserved names, hashed addresses
4. **Tier Prioritization**: Elderfier nodes prioritized in consensus
5. **Conflict Detection**: Duplicate service ID prevention

## Future Enhancements

1. **Service ID Transfer**: Allow service ID transfer between operators
2. **Custom Name Auctions**: Auction system for premium custom names
3. **Service ID Expiration**: Time-limited service IDs
4. **Enhanced Privacy**: Additional privacy features for hashed addresses
5. **Service ID Categories**: Categorize service IDs by type or region

## Conclusion

Elderfier service nodes provide a powerful enhancement to the Fuego Eldernode system, offering:

- **Higher tier** with increased stake requirements
- **Flexible service identification** with three distinct options
- **Privacy support** through hashed addresses
- **Enhanced prioritization** in network operations
- **Comprehensive validation** and conflict prevention

This implementation successfully addresses the requirement for Elderfier operators to have additional service ID options beyond public wallet addresses, while maintaining compatibility with the existing basic Eldernode system.
