## Elderfiers: Role, Architecture, Operations, and Security

### What is an Elderfier?
Elderfiers are specialized service nodes that provide fast, high-assurance validation services for the network. They prioritize low-latency responses (fastpass) while preserving robust safety through a broader committee fallback. Elderfiers are identity-bound to a fee address and operate under strict economic and governance constraints to discourage abuse and ensure correct behavior.

### Why Elderfiers exist
- **Low-latency decisions**: Provide near-instant responses for high-traffic or UX-critical flows.
- **Progressive consensus**: Default to small, tightly controlled fast committees; fall back to a larger committee for contentious cases.
- **Economic alignment**: Require an on-chain deposit (or minimum-stake check in private deployments) to deter Sybil attacks and enforce accountability.
- **Accountable service**: Subject to community governance and slashing for malicious or negligent behavior.

## Architecture Overview

### Key components
- **Elderfier Node**: A daemon configured with a fee address (identity), optional view key, and Elderfier settings.
- **Deposit/Stake**: Economic requirement proving commitment:
  - Mainnet: deposit via 0x06 tag transactions.
  - Private/test deployments: minimum-stake checks may be used as a practical substitute.
- **Registry**: A directory of active Elderfiers (configurable via `--elderfier-registry-url`) used to discover peers and publish metadata.
- **Consensus**:
  - **Fastpass**: 2/2 Elderfiers (default) for low-latency acceptance when responses agree.
  - **Robust fallback**: 4/5 Eldernode committee for resilience under disagreement or elevated risk.

### Identity and configuration
- **Identity**: The Elderfier is bound to a fee address (starts with `fire...`).
- **Optional view key**: Enables receipt verification when required.
- **Example names**: Exactly 8 characters, e.g., `FIRENODE`, `MYNODE00`.

## Elderfier Deposit System (Mainnet)

### 0x06 Tag Transaction Structure
- Deposits are signaled using a tagged transaction extra entry with tag `0x06`.
- Fields include a reference to the Elderfier identity (fee address) and the deposit amount.
- Nodes validate:
  - The tag and fields are well-formed.
  - The amount meets the threshold.
  - Funds are locked for the deposit lifecycle.

### Deposit Lifecycle
1. **Create**: Submit a 0x06-tagged deposit transaction meeting the minimum amount.
2. **Confirm**: Wait for sufficient confirmations (per-chain policy) to enter the active set.
3. **Active**: Elderfier participates in fastpass responses and consensus.
4. **Unstake (request)**: Issue an unstake request; begins the security window.
5. **Security window**: Deposit remains locked for a fixed time (see below) to allow challenges.
6. **Release**: If no slash/challenge succeeds, funds unlock after the window.

### Security Window System
- A mandatory delay (e.g., 8 hours) from unstake request to final unlock.
- Purpose: allow detection and adjudication of misbehavior before funds are released.

## Unstaking Process
- Elderfier submits an unstake request.
- The request is announced and enters the security window.
- If no successful slashing or challenge occurs within the window, the deposit unlocks.
- If a slash is approved, funds are partially or fully burned/withheld per policy.

## Elder Council Voting System

### Vote types
- **Slash**: Penalize misbehavior (malicious responses, double-serve, censorship, etc.).
- **Unlock**: Approve release of funds after window if needed by policy.
- **Operational**: Non-economic votes (e.g., registry policy changes).

### Two-step vote confirmation
1. **Initiation**: A council member posts a signed vote (with evidence when applicable).
2. **Confirmation**: A second, independent member confirms the vote (threshold-based).
- Finality is reached when the configured threshold is met (e.g., 2/2 for fastpass, 4/5 for robust).

## Consensus: Fastpass + Robust Fallback
- **Fastpass (2/2)**: Two Elderfiers respond; if both agree, the result is accepted quickly.
- **Fallback (4/5)**: If fastpass disagrees or risk is elevated, escalate to a 5-node committee requiring 4 matching responses.
- This progressive model balances latency and safety.

## Operational Guide

### Minimum economic requirement
- Default minimum Elderfier requirement: 800 XFG (atomic units with 7 decimals).
- On mainnet, enforce via 0x06 deposits. On private/test deployments, a minimum-stake wallet check can be used.

### Daemon configuration
- Required:
  - `--set-fee-address <fire...>`: Elderfier identity address (must start with `fire`).
  - `--enable-elderfier`: Enable Elderfier mode where supported.
- Optional:
  - `--view-key <secret_view_key>`: Enables fee transaction verification, if needed.
  - `--elderfier-config <path>`: Local Elderfier configuration file.
  - `--elderfier-registry-url <url>`: Registry/discovery endpoint.

### Example
```
./fuegod \
  --set-fee-address fire1exampleelderfieraddressxxxxxxxxxxxxxxxxxxxxxxxx \
  --enable-elderfier \
  --elderfier-registry-url https://registry.example.org \
  --log-level 2
```
- If running in a private/test environment, ensure the Elderfier address wallet meets or exceeds 800 XFG.

### Naming
- Custom names must be exactly 8 characters, e.g., `FIRENODE`, `MYNODE00`.

## Security and Slashing
- **Detectable behaviors**: inconsistent responses, selective refusal, protocol violations, or cryptographic proof failures.
- **Evidence**: attach signed responses, transcripts, or block/tx proofs to slash proposals.
- **Penalties**: partial or full slashing; temporary or permanent suspension from registry.
- **Appeals**: optionally allowed per governance policy.

## Monitoring and SLOs
- Track:
  - Response latency and error rate.
  - Agreement rate with peers (fastpass vs fallback usage).
  - Deposit status (active/unstaking/window).
- Alert on:
  - Consensus escalations (frequent fallback usage).
  - Divergent responses vs peers.
  - Imminent security window expiry.

## Troubleshooting
- **Deposit not recognized**:
  - Ensure the 0x06 tag is formatted correctly and confirmations are sufficient.
  - Confirm the fee address begins with `fire` and matches your Elderfier identity.
- **Fails minimum requirement**:
  - Verify deposit/stake meets the 800 XFG minimum.
  - On private/test deployments, ensure the configured wallet has sufficient funds and scanning is complete.
- **Frequent fallback (4/5)**:
  - Check time sync, network conditions, and software version alignment with peers.
  - Inspect logs for inconsistent or slow responses.
- **Unstake delays**:
  - The security window (e.g., 8 hours) is enforced by design; ensure no active challenges.

## Appendix

### Glossary
- **Elderfier**: A service node providing fast, accountable validation.
- **Fastpass**: Small, low-latency committee (2/2) for quick acceptance.
- **Fallback**: Larger, robust committee (4/5) for safety under disagreement.
- **0x06 tag**: Transaction extra tag signaling an Elderfier deposit.
- **Security window**: Time buffer between unstake request and release to allow challenges.

### Notes
- Address format uses the `fire...` prefix for public addresses.
- Amounts: XFG uses 7 decimal places (atomic units). 800 XFG = 80,000,000,000 atomic units.
- This document reflects the current implementation: mainnet prefers 0x06 deposits; private/test deployments may rely on minimum-stake wallet verification with the same economic threshold.
