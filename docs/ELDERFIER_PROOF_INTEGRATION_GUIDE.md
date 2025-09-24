# Elderfier Proof Integration Dev Guide

This guide outlines the remaining development tasks to fully integrate the Elderfier burn-proof process (FastPass → Fallback → Full Quorum) into the Fuego system.

## 1. Block Confirmation Gating
- Use `fastPassConfirmationBlocks`, `fallbackConfirmationBlocks`, and `fullConfirmationBlocks` from `BurnDepositConfig` to defer consensus requests.
- Hook into the blockchain height updates (e.g., via `core::onNewBlock`) or poll `getCurrentBlockchainHeight()`.
- Only invoke `requestEldernodeConsensus` after reaching the configured confirmation count for each consensus path.

## 2. P2P Message Plumbing
- Define new P2P commands in `P2pProtocolDefinitions.h` for:
  - Proof request messages (client → Eldernodes)
  - Proof response messages (Eldernodes → client)
- Implement handlers in `NetNode` or `MessageProcessor` to serialize/deserialize and dispatch these messages.
- Ensure `NodeRpcProxy` or light-wallet code can send and receive these messages over the network.

## 3. UI/UX & Automatic Fallback Logic
- Surface progress indicators: "Waiting for X/3 blocks", "FastPass consensus...", etc.
- After `fastPassConfirmationBlocks`:
  - Trigger 3/3 FastPass consensus.
  - If successful, complete the proof flow.
  - If unsuccessful, wait for `fallbackConfirmationBlocks`, then trigger 5/7 fallback consensus.
- If fallback fails, wait for `fullConfirmationBlocks`, then trigger 7/10 full-quorum consensus.

## 4. Test Coverage
- Unit-test each consensus threshold path:
  - Mock/block height simulation to validate triggering at 3, 6, and 9 confirmations.
  - Simulate partial signature responses to test FastPass failure and fallback logic.
- Integration tests:
  - Spin up multiple `Eldernode` stubs and verify end-to-end proof submission, consensus, and result verification.

## 5. Timeouts & Proof Expiration
- Respect `proofExpirationSeconds` (from `BurnDepositConfig`) to avoid infinite waiting.
- Implement timeout cleanup and error handling:
  - If consensus is not reached within expiration, abort and return an error.

## 6. Cleanup & Configuration
- Expose `BurnDepositConfig` parameters in configuration files and wallet UIs.
- Provide sensible defaults:
  - `fastPassConfirmationBlocks = 3`
  - `fallbackConfirmationBlocks = 6`
  - `fullConfirmationBlocks = 9`
  - `proofExpirationSeconds = 3600`

---

## 7. Fee Splits (fier_fees)

To reward Eldernodes for participating in consensus, configure and distribute proof fees as follows:

1. **Configure proof fee amounts** in `BurnDepositConfig`:
   - `smallBurnProofFee = 80000;`      // 0.008 XFG for standard burns
   - `largeBurnProofFee = 8000000;`     // 0.8 XFG for large burns

2. **After consensus verification** (in `verifyEldernodeConsensus` or immediately after):
   - Retrieve the list of Eldernode IDs whose signatures matched: `consensus.eldernodeIds`.
   - Determine which fee applies based on the burn deposit type:
     - If `proof.burnAmount == BURN_DEPOSIT_STANDARD_AMOUNT`, use `smallBurnProofFee`.
     - If `proof.burnAmount == BURN_DEPOSIT_LARGE_AMOUNT`, use `largeBurnProofFee`.

3. **Compute per-node fee**:
   ```cpp
   uint64_t totalFee = (proof.burnAmount == BURN_DEPOSIT_LARGE_AMOUNT)
       ? config.largeBurnProofFee
       : config.smallBurnProofFee;
   size_t winners = consensus.eldernodeIds.size();
   uint64_t perNodeFee = totalFee / winners;
   uint64_t remainder = totalFee % winners;  // if needed
   ```

4. **Distribute fees**:
   - For each matching Eldernode ID:
     - Issue a payment of `perNodeFee` XFG via the payment service or wallet RPC.
   - Optionally allocate the `remainder` to the treasury or burn it.

5. **Record distributions**:
   - Log or store a record of which nodes were paid and the amounts.
   - Expose this data via metrics or a UI component for transparency.

With this in place, proof fees are split evenly among all Eldernodes whose proofs matched consensus, ensuring dynamic and fair reward distribution based on actual network size.

## 8. Ephemeral Per-Proof Multisig Workflow
1) Query the current active Eldernode public keys (N nodes) via IEldernodeIndexManager.
2) Initiate the CryptoNote multisig key-exchange protocol among those N nodes to derive a one-time K-of-N multisig address.
3) Build the burn-deposit transaction with two outputs:
   - The standard burn output (commitment tag, zero-XFG burn).
   - A second output sending the fier-fee into the new multisig address.
4) Wait for the configured confirmations (FastPass/Fallback/FullQuorum) on the burn-deposit TX and complete the P2P proof signature round.
5) Once consensus is reached, collect at least K-of-N multisig spending shares from the agreeing nodes.
6) Perform the CryptoNote multisig signing protocol to assemble the payout transaction, splitting the fee UTXO evenly among the agreeing Eldernodes.
7) Broadcast the payout transaction on-chain; all fee distributions are then immutable and auditable.

This guide serves as a checklist for developers to complete the proof flow implementation. Feel free to expand with code snippets or IDE-specific instructions as needed.
