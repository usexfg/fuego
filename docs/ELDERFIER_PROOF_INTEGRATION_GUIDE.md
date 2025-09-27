# Elderfier Proof Integration Dev Guide

This guide outlines the remaining development tasks to fully integrate the Elderfier burn-proof process (FastPass → Fallback → Elder Council) into the Fuego system.

## Implementation Status (2025-09-27)
- **Recommended Approach**: Use threshold signatures with single burn+fee transactions instead of complex per-proof multisig
- **Architecture**: Leverage existing service node quorum engine with minimal new P2P messages
- **Security**: On-chain proof storage enhances security through immutable audit trails
- **Escalation**: Failed consensus cases escalate to Elder Council for human review

## Elegant Implementation Architecture

### 1. Single Burn+Fee Transaction Structure
Replace complex multisig with atomic burn+fee transactions:

```cpp
struct BurnDepositTransaction {
    std::vector<TransactionInput> inputs;
    
    // Two outputs in one transaction:
    std::vector<TransactionOutput> outputs = {
        // Output 0: Burn commitment (zero-value to burn address)
        {
            amount: burn_amount,           // 0.8 XFG or 8 XFG  
            target: burn_commitment_key    // Provably unspendable
        },
        
        // Output 1: Fier fee (locked until proof consensus)
        {
            amount: fier_fee,              // 0.008 XFG or 0.8 XFG
            target: threshold_signature_key // Spendable by Elderfier consensus
        }
    };
    
    TransactionExtra extra = {
        burn_type: SMALL_BURN | LARGE_BURN,
        proof_request_id: H(tx_hash),
        elderfier_quorum_keys: [P_FastPass, P_Fallback, P_Council]
    };
}
```

**User Cost Structure:**
- **Small burn**: 0.008 XFG (burn) + 0.008 XFG (fier_fee) + 0.008 XFG (network) = **0.024 XFG total**
- **Large burn**: 0.8 XFG (burn) + 0.8 XFG (fier_fee) + 0.008 XFG (network) = **1.608 XFG total**

### 2. Threshold Signature Consensus Paths

**Progressive Consensus with Elder Council Escalation:**
```
FastPass (3/3) → Fallback (6/8) → Elder Council Review
     ↓              ↓                    ↓
   Success        Success            Quorum Decision
     ↓              ↓                    ↓
  Distribute    Distribute         Action/Reject
```

**Implementation:**
- Use MuSig2/FROST for aggregate public keys per consensus path
- Deterministic Elder selection from `hash(burn_txid) mod ActiveElders`
- Single unlock transaction with threshold signature (64 bytes vs 640+ for individual sigs)

### 3. Fake Request Prevention

**Multi-layer validation prevents abuse:**

```cpp
bool validateBurnTransaction(const Crypto::Hash& tx_hash) {
    // 1. Transaction must exist on-chain
    if (!blockchain.hasTransaction(tx_hash)) return false;
    
    // 2. Must be type 0x08 burn_deposit
    if (tx.type != BURN_DEPOSIT_TYPE) return false;
    
    // 3. Must have valid burn commitment
    if (!validateBurnCommitment(tx.extra)) return false;
    
    // 4. Must have required confirmations (3+ blocks)
    if (getCurrentHeight() - tx.block_height < 3) return false;
    
    return true;
}
```

**Rate limiting and economic deterrents:**
- Users pay fier_fee upfront (lost if submitting fake requests)
- Max 10 proof requests per hour per IP
- Exponential backoff for repeated fake requests

### 4. Elder Council Oversight

**Failed consensus escalation:**
```cpp
struct FailedConsensusCase {
    Crypto::Hash burn_tx_hash;
    uint8_t failure_reason;  // INVALID_PROOF | NETWORK_SYNC | BAD_ACTOR
    std::vector<Crypto::PublicKey> non_responding_nodes;
    uint64_t timestamp;
    std::vector<ElderfierVote> council_votes;  // Requires >8 votes same choice
};
```

**Elder Council Quorum Rules:**
- Minimum 8/10 Elderfiers must vote same choice
- Voting options: `INVALID_PROOF | NETWORK_ISSUE | BAD_ACTOR | ALL_GOOD`
- Council decisions trigger automatic actions (slashing, network alerts, etc.)

## Implementation Components

### Core/Daemon Requirements
- Extend service node quorum engine with `PROOF_QUORUM` type
- Add quorum selection hook: `core::getProofQuorum(height, path)`
- Implement threshold signature validation in transaction processing

### P2P Message Extensions
**Minimal new messages (reuse existing infrastructure):**
```
ID   Direction     Purpose
---- ------------- -----------------------------------------
0x52 wallet → node RequestProofSign(tx_hash, path=FAST|FB|COUNCIL)
0x53 node   → node ProofSignature(tx_hash, partial_sig, pubkey)
0x54 node   → node CouncilVote(case_id, vote_choice, signature)
```

### Wallet RPC Endpoints
```cpp
// Create atomic burn+fee transaction
TransactionHash createBurnProofRequest(uint64_t burn_amount, BurnType type);

// Start consensus process
bool submitProofBurn(const Crypto::Hash& txid);

// Check consensus status
ProofStatus getProofStatus(const Crypto::Hash& txid);
```

### On-Chain Proof Storage
**Compact proof format (99 bytes vs 640+ for individual signatures):**
```cpp
struct CompactElderfierProof {
    Crypto::Hash burn_tx_hash;           // 32 bytes
    uint8_t consensus_path;              // 1 byte (FastPass/Fallback/Council)
    Crypto::Signature threshold_sig;     // 64 bytes  
    std::vector<uint8_t> winner_bitmap;  // ~2 bytes for 10 Elderfiers
};
```

## Security Benefits

1. **Immutable Audit Trail**: Prevents retroactive manipulation of consensus results
2. **Double-Spend Prevention**: On-chain proofs can't be replayed
3. **Economic Deterrent**: Upfront fier_fee payment deters fake requests
4. **Elder Council Oversight**: Human review for edge cases and bad actors
5. **Compact Footprint**: Threshold signatures minimize chain bloat

## Migration Path

**Phase 1**: Implement threshold signature consensus with single-path payouts
**Phase 2**: Add Elder Council voting and failed consensus escalation  
**Phase 3**: Optimize with batched payouts and enhanced metrics

This approach eliminates multisig complexity while maintaining all security guarantees and providing a clear escalation path for disputed cases.
