# 🎯 Eldernode Stake Requirement: 800 XFG

## 🔒 **Economic Security Through Stake**

The Eldernode service now requires a **minimum stake of 800 XFG** to participate in consensus validation. This creates a strong economic barrier that ensures only serious, committed participants can become Eldernodes, significantly enhancing network security and trust.

## 🔧 **How Stake Verification Works**

### **🎯 Automatic Stake Checking**
```bash
# When starting Eldernode service, stake is automatically verified
./fuegod \
  --enable-eldernode \
  --set-fee-address YOUR_FEE_ADDRESS \
  --view-key YOUR_VIEW_KEY
```

**What happens during startup:**
1. ✅ **Stake verification** - Checks if fee address has ≥800 XFG
2. ✅ **Balance validation** - Confirms current balance meets requirement
3. ✅ **Service initialization** - Only starts if stake requirement met
4. ✅ **Ongoing monitoring** - Continuously verifies stake remains sufficient

### **🚨 Stake Verification Failure**
```bash
# ❌ ERROR: Insufficient stake
./fuegod --enable-eldernode --set-fee-address ADDRESS --view-key KEY

# Output:
# [ERROR] Eldernode requires minimum stake of 800 XFG
# [ERROR] Fee address ADDRESS has insufficient balance
# [ERROR] Current balance: 500000000000 atomic units
# [ERROR] Required: 800000000000 atomic units (80.0 XFG)
# [ERROR] Daemon startup failed
```

### **✅ Successful Stake Verification**
```bash
# ✅ SUCCESS: Sufficient stake
./fuegod --enable-eldernode --set-fee-address ADDRESS --view-key KEY

# Output:
# [INFO] Stake verification passed: ADDRESS has sufficient balance
# [INFO] Current balance: 1200000000000 atomic units
# [INFO] Required stake: 800000000000 atomic units (80.0 XFG)
# [INFO] Starting Eldernode service...
# [INFO] Eldernode service integrated into main RPC server
```

---

## 💰 **Stake Requirements**

### **📊 Minimum Stake: 800 XFG**
```cpp
// Hardcoded stake requirement
static const uint64_t MINIMUM_STAKE_ATOMIC = 800000000000; // 800 XFG
static const double MINIMUM_STAKE_XFG = 800.0; // 800 XFG
```

**Atomic Units:** 800,000,000,000 (7 decimal places)
**XFG Amount:** 800.0 XFG
**Purpose:** Economic barrier for Eldernode participation

### **🔍 What Counts as Stake**
- ✅ **Confirmed balance** in the fee address
- ✅ **Unlocked funds** (not locked in transactions)
- ✅ **Spendable XFG** (not reserved for fees)
- ❌ **Pending transactions** (not yet confirmed)
- ❌ **Locked funds** (in escrow or time-locked)

---

## 🛡️ **Security Benefits of Stake Requirement**

### **1. Sybil Attack Prevention**
```cpp
// Creating fake Eldernodes now costs real money
// Each fake node requires 800 XFG stake
// Attack cost: Number of fake nodes × 800 XFG
```

**Benefits:**
- ✅ **Economic barrier** - Fake nodes cost 800 XFG each
- ✅ **Attack cost** - Large-scale attacks become prohibitively expensive
- ✅ **Real commitment** - Only serious participants can join
- ✅ **Network stability** - Reduces malicious node creation

### **2. Quality Assurance**
```cpp
// Nodes with 800 XFG stake have proven:
// - Economic interest in network health
// - Ability to maintain significant funds
// - Commitment to long-term participation
```

**Benefits:**
- ✅ **Proven commitment** - 800 XFG shows serious intent
- ✅ **Economic alignment** - Validators benefit from network success
- ✅ **Reduced churn** - Stake holders less likely to abandon network
- ✅ **Higher quality** - Only committed participants become validators

### **3. Accountability Mechanism**
```cpp
// Bad behavior can result in:
// - Loss of fee income
// - Potential stake slashing (future feature)
// - Reputation damage affecting future earnings
```

**Benefits:**
- ✅ **Economic incentives** - Good behavior maximizes returns
- ✅ **Stake at risk** - Bad behavior affects their investment
- ✅ **Reputation system** - Quality validators earn more fees
- ✅ **Self-policing** - Validators monitor each other

---

## 🔄 **Stake Verification Process**

### **1. Startup Verification**
```cpp
bool verifyMinimumStake(const std::string& address, uint64_t minimumStake, 
                       const CryptoNote::core& ccore, const CryptoNote::Currency& currency) {
    // Parse address
    AccountPublicAddress acc = parseAddress(address);
    
    // Get current balance
    uint64_t currentBalance = getAddressBalance(address, ccore);
    
    // Verify minimum requirement
    return currentBalance >= minimumStake;
}
```

**Steps:**
1. **Address parsing** - Validate fee address format
2. **Balance query** - Get current balance from blockchain
3. **Stake verification** - Compare with 800 XFG requirement
4. **Service start** - Only proceed if requirement met

### **2. Ongoing Monitoring**
```cpp
// Eldernode service continuously monitors stake
void monitoringLoop() {
    while (m_isMonitoring) {
        // Check stake every block
        if (!verifyMinimumStake()) {
            logger(WARNING) << "Stake below minimum requirement, pausing validation";
            pauseValidation();
        }
        
        // Wait for next block
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}
```

**Continuous checks:**
- ✅ **Block-by-block** - Verify stake on each new block
- ✅ **Automatic pausing** - Stop validation if stake drops
- ✅ **Resume capability** - Resume when stake requirement met
- ✅ **Logging** - Record all stake-related events

---

## 📊 **Stake Status Information**

### **1. RPC Endpoint: `getEldernodeStatistics`**
```bash
curl -X POST http://localhost:8081/json_rpc \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"getEldernodeStatistics"}'
```

**Response includes stake information:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "isRunning": true,
    "isMonitoring": true,
    "eldernodeIdentity": "5KJvsngHeMsoVzqUfLqXEsmuL1um4ULDqFeyBSH4qD1n5vzq",
    "feeAddress": "5KJvsngHeMsoVzqUfLqXEsmuL1um4ULDqFeyBSH4qD1n5vzq",
    
    "stakeStatus": {
      "currentStake": 1200000000000,
      "currentStakeXFG": 120.0,
      "minimumRequired": 800000000000,
      "minimumRequiredXFG": 800.0,
      "stakeSufficient": true,
      "stakePercentage": 150.0
    },
    
    "totalProofsProcessed": 0,
    "totalProofsSubmitted": 0,
    "totalProofsFailed": 0,
    "lastProcessedBlock": 12345,
    "consensusThreshold": 2,
    "minEldernodes": 2
  }
}
```

### **2. Stake Status Fields**
- ✅ **`currentStake`** - Current balance in atomic units
- ✅ **`currentStakeXFG`** - Current balance in XFG
- ✅ **`minimumRequired`** - Required stake in atomic units (800,000,000,000)
- ✅ **`minimumRequiredXFG`** - Required stake in XFG (800.0)
- ✅ **`stakeSufficient`** - Whether current stake meets requirement
- ✅ **`stakePercentage`** - Current stake as percentage of requirement

---

## 🚨 **Stake-Related Error Handling**

### **1. Insufficient Stake at Startup**
```bash
# Error: Not enough XFG in fee address
./fuegod --enable-eldernode --set-fee-address ADDRESS --view-key KEY

# Solutions:
# 1. Transfer more XFG to fee address
# 2. Wait for pending transactions to confirm
# 3. Use a different address with sufficient balance
```

### **2. Stake Drops Below Requirement**
```bash
# During operation, if stake drops below 800 XFG:
# [WARNING] Stake below minimum requirement, pausing validation
# [WARNING] Current stake: 750.0 XFG, Required: 800.0 XFG

# Solutions:
# 1. Transfer more XFG to fee address
# 2. Wait for stake to increase through fees
# 3. Service automatically resumes when requirement met
```

### **3. Address Balance Issues**
```bash
# Error: Cannot verify address balance
# [ERROR] Failed to get balance for address: ADDRESS

# Solutions:
# 1. Check address format is correct
# 2. Ensure address is on correct network
# 3. Verify blockchain synchronization
```

---

## 🚀 **Stake Management Best Practices**

### **1. Initial Setup**
```bash
# 1. Ensure fee address has >800 XFG
# 2. Account for transaction fees and pending amounts
# 3. Use confirmed, unlocked balance only
# 4. Consider keeping extra buffer (e.g., 1000 XFG)

# Example: Transfer 1000 XFG to fee address
# This provides 200 XFG buffer above requirement
```

### **2. Ongoing Management**
```bash
# 1. Monitor stake through RPC endpoint
# 2. Set up alerts for low stake warnings
# 3. Maintain buffer above minimum requirement
# 4. Plan for fee income to sustain stake

# Monitor stake status:
watch -n 30 'curl -s -X POST http://localhost:8081/json_rpc \
  -H "Content-Type: application/json" \
  -d '"'"'{"jsonrpc":"2.0","id":1,"method":"getEldernodeStatistics"}'"'"' | jq ".result.stakeStatus"'
```

### **3. Stake Recovery**
```bash
# If stake drops below requirement:
# 1. Transfer additional XFG to fee address
# 2. Wait for transaction confirmation
# 3. Service automatically resumes validation
# 4. Monitor stake status to confirm recovery

# Quick stake check:
curl -X POST http://localhost:8081/json_rpc \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"getEldernodeStatistics"}' | jq ".result.stakeStatus"
```

---

## 🔧 **Configuration Options**

### **1. Hardcoded Stake Requirement**
```cpp
// Currently hardcoded for security
static const uint64_t MINIMUM_STAKE_ATOMIC = 800000000000; // 800 XFG

// Future: Could be configurable per network
// - Mainnet: 800 XFG
// - Testnet: 80 XFG  
// - Devnet: 8 XFG
```

### **2. Stake Verification Frequency**
```cpp
// Check stake every block (default: 2 seconds)
static const uint64_t STAKE_CHECK_INTERVAL_BLOCKS = 1;

// Can be adjusted for performance vs. security
// - Higher frequency: Better security, more overhead
// - Lower frequency: Less overhead, potential delay in detection
```

---

## 🎯 **Benefits of 800 XFG Stake Requirement**

### **For Network Security:**
- ✅ **Sybil resistance** - Fake nodes cost 800 XFG each
- ✅ **Economic barriers** - Large-scale attacks become expensive
- ✅ **Quality validators** - Only committed participants join
- ✅ **Stake-based trust** - Higher stakes = higher trust

### **For Validators:**
- ✅ **Fee income** - Earn fees for validation services
- ✅ **Network influence** - Participate in consensus decisions
- ✅ **Economic incentives** - Network success benefits validators
- ✅ **Reputation building** - Quality service increases earnings

### **For Users:**
- ✅ **Higher trust** - Validators have proven commitment
- ✅ **Better security** - Economic incentives prevent attacks
- ✅ **Quality validation** - Stake holders provide better service
- ✅ **Network stability** - Reduced malicious node creation

---

## 🚀 **Production Deployment with Stake**

### **Systemd Service**
```bash
sudo tee /etc/systemd/system/fuego-eldernode.service > /dev/null << 'EOF'
[Unit]
Description=Fuego Daemon with Integrated Eldernode Service (800 XFG Stake Required)
After=network.target

[Service]
Type=simple
User=fuego
WorkingDirectory=/home/fuego/fuego
ExecStart=/home/fuego/fuego/fuegod \
  --enable-eldernode \
  --set-fee-address 5KJvsngHeMsoVzqUfLqXEsmuL1um4ULDqFeyBSH4qD1n5vzq \
  --view-key 1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
sudo systemctl enable fuego-eldernode
sudo systemctl start fuego-eldernode
```

### **Docker Deployment**
```bash
docker run -d \
  --name fuego-eldernode \
  -p 8081:8081 \
  -v /path/to/config:/config \
  fuego/fuego:latest \
  --enable-eldernode \
  --set-fee-address 5KJvsngHeMsoVzqUfLqXEsmuL1um4ULDqFeyBSH4qD1n5vzq \
  --view-key 1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef \
  --eldernode-config /config/eldernode.conf
```

---

## 🎯 **Summary**

### **What We Achieved:**
1. **🔒 Economic Security** - 800 XFG stake requirement prevents Sybil attacks
2. **🛡️ Quality Assurance** - Only committed participants become validators
3. **💰 Economic Alignment** - Validators benefit from network success
4. **🔄 Automatic Verification** - Stake checked at startup and continuously
5. **📊 Transparent Monitoring** - Stake status available via RPC

### **Key Benefits:**
- ✅ **Sybil resistance** - Fake nodes cost 800 XFG each
- ✅ **Higher quality** - Only serious participants join
- ✅ **Economic incentives** - Good behavior maximizes returns
- ✅ **Network stability** - Reduced malicious node creation
- ✅ **Stake-based trust** - Higher stakes = higher trust

**🎯 The 800 XFG stake requirement creates a robust economic security model that ensures only committed, trustworthy participants can become Eldernodes, significantly enhancing the overall security and reliability of the consensus network!**
