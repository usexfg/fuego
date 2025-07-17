# Fuego Forecast Architecture & Token Logic (2025 Update)

## Dual Contract Structure
- **O Contract:**
  - Supports O token, bonding curve, governance, and advanced features.
  - Handles O token minting, burning, and protocol upgrades.
  - Has its own protocol account for protocol fees.
- **HEAT Contract:**
  - Pure SPL token prediction market (no bonding, no O token, no minting).
  - Users stake HEAT into UP/DOWN pools for each epoch.
  - No supply changes, no governance, no bonding curve.
  - Has its own protocol account for protocol fees (separate from O contract).
  - Treasury account is shared with O contract.

## HEAT Contract Logic
- **Staking:** Users deposit HEAT into UP or DOWN vaults for each epoch.
- **Epoch Resolution:**
  - At the end of each epoch, the outcome is determined (UP, DOWN, or Neutral).
  - **Losing side:** Each user loses 80% of their stake (20% is returned to them).
  - **Of the 80% lost:**
    - 2% goes to the HEAT protocol account (for execution fees, etc.).
    - 8% goes to the shared treasury account (DAO, multisig, etc.).
    - 70% is distributed to the winners, proportional to their vault share.
  - **No burn:** No tokens are burned in the HEAT contract.
- **Claiming:**
  - Losing users can claim 20% of their original stake.
  - Winning users claim their original stake plus their share of the 70% pool.

## Accounts
- **Protocol Account:** Separate for O and HEAT contracts for clear accounting.
- **Treasury Account:** Shared between O and HEAT contracts.

## UI/UX (Fallout Terminal UI)
- Supports flipping between O and HEAT modes with a 3D animation.
- Each side has its own color themes and mirrored stats/functions.
- CLI terminal supports commands like `help`, `info`, `status`, `balance`, `clear`, `stop` (to stop system messages).
- CLI output uses the correct "unselected" color for each theme/side, and timestamps are only shown for system/heartbeat messages.
- All stats, balances, and token references are mirrored for O and HEAT.

## Key Decisions & Notes
- No bonding or minting in HEAT contract; all logic is pure SPL token transfer and vault accounting.
- All protocol and treasury fee logic is explicit and on-chain, with clear separation for audits.
- The architecture is designed for maximum clarity, auditability, and user experience.

---

(For full technical details, see the respective contract source files and UI code.)
