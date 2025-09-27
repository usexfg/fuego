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

**Progressive Consensus with Async Elder Council Review:**
```
FastPass (3/3) → Fallback (6/8) → REJECT + Async Council Review
     ↓              ↓                    ↓
   Success        Success            Immediate Rejection
     ↓              ↓                    ↓
  Distribute    Distribute         + Council Review (async)
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

**Consensus Flow:**
1. **FastPass (3/3)**: Attempt consensus with 3 Eldernodes
2. **Fallback (6/8)**: If FastPass fails, attempt with 8 Eldernodes (6 required)
3. **REJECT + Async Council Review**: If Fallback fails, immediately reject the proof and send detailed failure case to Elder Council inbox for review (doesn't block the rejection)

**Strike-Based Governance System:**
- **Strike Tracking**: Elderfiers accumulate strikes for providing proofs that conflict with majority consensus
- **3 Strikes Rule**: Elderfiers with 3+ strikes are flagged for slashing
- **Detailed Records**: System tracks total proofs submitted, strike rate, and timestamps
- **Council Review**: Failed consensus cases include detailed logs of which Elderfiers provided conflicting proofs

**Strike Management:**
- Strikes are recorded for Elderfiers who submit proofs that conflict with majority consensus
- Participation is tracked for all Elderfiers who submit proofs (both correct and incorrect)
- Strike rate = (strikes / total_proofs_submitted) × 100%
- When an Elderfier reaches 3+ strikes, a council review message is automatically generated

**Elder Council Voting (Manual Process):**
- **Minimum 8/10 Elderfiers** must vote same choice
- **Anonymous Review**: Council reviews evidence without knowing which specific Elderfier is being reviewed
- **Aggregate Voting**: Council votes on slashing actions based on strike patterns, not targeting individuals
- **Voting Options:**
  - `SLASH_ALL`: Slash all Elderfiers with 3+ strikes
  - `SLASH_HALF`: Slash 50% of Elderfiers with 3+ strikes
  - `SLASH_NONE`: No slashing, continue monitoring
  - `REVIEW_MORE`: Request additional investigation
- **Evidence-Based Review**: Messages include strike counts, participation rates, and consensus failure patterns without revealing Elderfier identities

**Example Council Review Message:**
```
ELDERFIER COUNCIL REVIEW REQUIRED
================================

An Elderfier has provided proof AGAINST consensus 3 times in 15 total rounds participated.

Strike Count: 3
Total Rounds Participated: 15
Strike Rate: 20.00%

Evidence of conflicting proof submissions in consensus rounds:
- Round participation with incorrect consensus proofs
- Strike accumulation over multiple consensus attempts

Please vote your decision for action:
a) SLASH_ALL - Slash all Elderfiers with 3+ strikes
b) SLASH_HALF - Slash 50% of Elderfiers with 3+ strikes
c) SLASH_NONE - No slashing, continue monitoring
d) REVIEW_MORE - Request additional investigation

Reply with your vote (a/b/c/d) signed with your Elderfier key.
```

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
