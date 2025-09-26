# Elderfier Proof Integration Dev Guide

This guide outlines the remaining development tasks to fully integrate the Elderfier burn-proof process (FastPass → Fallback → Full Quorum) into the Fuego system.

## Implementation Status (2025-09-25)
- **Foundation Complete**: Core data structures, consensus configuration, and service interfaces exist in `BurnDepositValidationService`
- **Elegant Path Available**: Can leverage existing P2P protocol framework, CryptoNote multisig, and transaction utilities
- **Missing Integration**: Real P2P message routing, block confirmation hooks, and multisig key exchange primitives
- **Recommended Approach**: Extend existing infrastructure rather than build new primitives

## Elegant Implementation Approach

Instead of creating entirely new primitives, here's how to elegantly implement the Elderfier proof system by extending existing infrastructure:

### Phase 1: P2P Protocol Extensions
**Leverage**: Existing `P2pProtocolDefinitions.h` framework
1. Add Elderfier message types to `P2pProtocolDefinitions.h`:
   ```cpp
   struct COMMAND_ELDERFIER_PROOF_REQUEST {
       enum { ID = P2P_COMMANDS_POOL_BASE + 100 };
       struct request {
           EldernodeVerificationInputs inputs;
           std::string requesterAddress;
           void serialize(ISerializer& s) { KV_MEMBER(inputs) KV_MEMBER(requesterAddress) }
       };
       struct response {
           std::vector<BurnProofData> proofs;
           std::vector<Crypto::Signature> signatures;
           void serialize(ISerializer& s) { KV_MEMBER(proofs) KV_MEMBER(signatures) }
       };
   };
   ```
2. Extend `NetNode` to handle Elderfier message routing
3. Use existing peer discovery for Eldernode selection

### Phase 2: Block Confirmation Hooks
**Leverage**: Existing `core::onNewBlock` callback system
1. Hook into `Core::onNewBlock` to monitor confirmation progress
2. Use existing blockchain height tracking: `getCurrentBlockchainHeight()`
3. Implement `ElderfierProofTracker` that subscribes to block events

### Phase 3: Multisig Integration
**Leverage**: Existing CryptoNote multisig capabilities
1. Extend transaction building utilities for per-proof multisig
2. Use existing key derivation and signing infrastructure
3. Create `ElderfierMultisigManager` that generates K-of-N addresses

### Phase 4: Fee Distribution
**Leverage**: Existing transaction utilities and wallet APIs
1. Build burn deposit transactions with fee escrow using `TransactionBuilder`
2. Create unlock transactions with proof embedding in `tx_extra`
3. Use existing fee calculation and distribution logic

## 1. Block Confirmation Gating
- Use `fastPassConfirmationBlocks`, `fallbackConfirmationBlocks`, and `fullConfirmationBlocks` from `BurnDepositConfig` to defer consensus requests.
- Hook into the blockchain height updates (e.g., via `core::onNewBlock`) or poll `getCurrentBlockchainHeight()`.
- Only invoke `requestEldernodeConsensus` after reaching the configured confirmation count for each consensus path.

## 2. P2P Message Plumbing (Elegant Extension)
- **Extend existing framework**: Add `COMMAND_ELDERFIER_PROOF_REQUEST` to `P2pProtocolDefinitions.h` (see Phase 1 above)
- **Leverage NetNode**: Add Elderfier message handlers to existing `NetNode::handleRequest` and `NetNode::handleResponse` methods
- **Use proven patterns**: Follow the same serialization approach as `COMMAND_REQUEST_STAT_INFO` and `COMMAND_REQUEST_NETWORK_STATE`
- **RPC integration**: Extend `NodeRpcProxy` with Elderfier proof methods that use existing HTTP client infrastructure

## 3. UI/UX & Automatic Fallback Logic (Event-Driven)
- **Leverage Core events**: Use existing `ICoreObserver` interface to listen for `onNewBlock` events
- **State tracking**: Implement `ElderfierProofTracker` that maintains proof state and triggers consensus at appropriate confirmation blocks
- **Automatic progression**: Built-in logic automatically attempts FastPass → Fallback → Full Quorum based on `BurnDepositConfig` thresholds
- **Status reporting**: Extend existing RPC status endpoints to include Elderfier proof progress

## 4. Test Coverage (Leverage Existing Testing Infrastructure)
- **Unit tests**: Use existing gtest framework to test consensus logic with mocked P2P and blockchain components
- **Integration tests**: Leverage existing `IntegrationTestLib` for end-to-end testing with multiple Eldernode instances
- **P2P testing**: Use existing `P2pNode` test infrastructure to verify message routing and consensus
- **Multisig testing**: Extend existing transaction validation tests for multisig fee distribution

## 5. Timeouts & Proof Expiration (Built-in Configuration)
- **Leverage existing config**: Use `proofExpirationSeconds` from existing `BurnDepositConfig` structure
- **Event-driven cleanup**: Hook into existing timer and cleanup systems for automatic proof expiration
- **Error handling**: Extend existing RPC error codes for Elderfier-specific timeout scenarios

## 6. Cleanup & Configuration (Configuration Management)
- **Extend existing config**: Add Elderfier parameters to existing `CoreConfig` and wallet configuration systems
- **Sensible defaults**: Already implemented in `BurnDepositConfig::getDefault()`:
  - `fastPassConfirmationBlocks = 3`
  - `fallbackConfirmationBlocks = 6`
  - `fullConfirmationBlocks = 9`
  - `proofExpirationSeconds = 3600`

---

## 7. Fee Splits (fier_fees) - Elegant Implementation

To reward Eldernodes for participating in consensus, leverage existing transaction and multisig infrastructure:

### Implementation Approach:
1. **Transaction Building**: Use existing `TransactionBuilder` utilities to create burn deposit transactions with fee outputs
2. **Multisig Creation**: Extend existing multisig key derivation for per-proof K-of-N addresses
3. **Fee Distribution**: Leverage existing fee calculation and distribution logic with Elderfier-specific extensions

1. **Configure proof fee amounts** in `BurnDepositConfig` (already implemented):
   - `smallBurnProofFee = 80000;`      // 0.008 XFG for standard burns
   - `largeBurnProofFee = 8000000;`     // 0.8 XFG for large burns

2. **After consensus verification**: Use existing `verifyEldernodeConsensus` with enhanced fee distribution:
   ```cpp
   // Enhanced fee distribution in BurnDepositValidationService::distributeProofFees
   uint64_t totalFee = (proof.burnAmount == BURN_DEPOSIT_LARGE_AMOUNT)
       ? config.largeBurnProofFee : config.smallBurnProofFee;

   // Get agreeing Eldernodes from consensus
   size_t winners = consensus.agreeingEldernodeIds.size();
   uint64_t perNodeFee = totalFee / winners;

   // Create multisig transaction for fee distribution
   return createMultisigFeeDistribution(consensus, perNodeFee, totalFee % winners);
   ```

3. **Elegant distribution**:
   - **Leverage existing wallet APIs**: Use `WalletService::sendTransaction` for fee payments
   - **Multisig integration**: Create per-proof multisig addresses using existing key derivation
   - **Automatic unlock**: Generate unlock transactions with proof embedding in `tx_extra`

4. **User Cost Breakdown**: Already implemented in existing configuration:
   - **Small burn deposit**: 0.008 XFG proof fee + 0.008 XFG network fee = **0.016 XFG total**
   - **Large burn deposit**: 0.8 XFG proof fee + 0.008 XFG network fee = **0.808 XFG total**

5. **Record distributions**: Extend existing logging and metrics infrastructure:
   - Use existing `LoggerRef` for distribution logging
   - Extend existing RPC status endpoints for fee transparency

## 8. Ephemeral Per-Proof Multisig Workflow (Elegant Extension)

1) **Query Eldernodes**: Use existing `IEldernodeIndexManager` to get active Eldernode public keys (already implemented)
2) **Leverage CryptoNote multisig**: Extend existing multisig key-exchange protocol (CryptoNote already has multisig support)
3) **Transaction building**: Use existing `TransactionBuilder` utilities for burn-deposit with fee escrow:
   ```cpp
   // Create multisig address from agreeing Eldernodes
   auto multisigAddress = createEphemeralMultisig(consensus.agreeingEldernodeIds);

   // Build transaction with fee output to multisig
   auto tx = TransactionBuilder(m_core)
       .addBurnOutput(commitment, burnAmount)
       .addOutput(totalFee, multisigAddress)
       .build();
   ```
4) **Consensus integration**: Wait for confirmations using existing block event system, complete P2P proof round
5) **Automatic distribution**: Once consensus reached, existing multisig protocol handles K-of-N signing automatically
6) **On-chain finality**: Broadcast payout transaction using existing transaction pool infrastructure

## Key Insight: Elegant Architecture Already Exists

The "elegant" approach isn't about building new primitives—it's about **connecting the dots** between existing, well-designed systems:

- **P2P messaging** → Extend existing protocol framework
- **Block confirmation** → Hook into existing event system
- **Multisig workflows** → Leverage existing CryptoNote capabilities
- **Fee handling** → Use existing transaction and wallet APIs
- **Configuration** → Already implemented in `BurnDepositConfig`
- **Testing** → Use existing gtest and integration test infrastructure

The foundation is solid; the implementation is about **integration, not invention**.

This guide serves as a checklist for developers to complete the proof flow implementation. Feel free to expand with code snippets or IDE-specific instructions as needed.
