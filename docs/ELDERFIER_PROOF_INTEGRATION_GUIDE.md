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

This guide serves as a checklist for developers to complete the proof flow implementation. Feel free to expand with code snippets or IDE-specific instructions as needed.
