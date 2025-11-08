# Fuego Electron Wallet

A beautiful, feature-rich desktop wallet for Fuego (XFG) cryptocurrency built with Electron.

## Features

### 🔥 Core Functionality
- **Automated Node Management** - Auto-starts `fuegod` daemon on launch
- **Integrated Wallet** - Full `walletd` RPC integration
- **Real-time Sync** - Live balance and transaction updates
- **Cross-platform** - Works on macOS, Windows, and Linux

### 💰 Wallet Features
- Create new wallet with one click
- View balance (available + locked)
- Send XFG with custom fees
- Transaction history with confirmations
- Copy address to clipboard

### 🔐 Security & Backup
- **Seed Phrase Export** - Save 25-word mnemonic to file
- **Private Keys Export** - Export view & spend keys
- **Secure Storage** - Wallet files stored in app data directory
- **Delete Wallet** - Safe wallet removal with confirmation

### 🎨 User Experience
- Beautiful gradient UI with dark theme
- Tabbed interface (Wallet, Send, Transactions, Backup, Settings)
- Status indicators for node & wallet
- Real-time logs display
- Auto-refresh every 5 seconds

## Prerequisites

- **Node.js 16+** installed
- **Fuego binaries** (`fuegod`, `walletd`) in one of:
  - `../build/src/` (development)
  - Packaged in app resources (production)

## Development Setup

```bash
cd electron-wallet
npm install
```

## Run in Development Mode

```bash
npm start -- --dev
```

The `--dev` flag enables:
- Developer tools (Console)
- Binary path resolution from `../build/src/`

## Build Production App

```bash
# Build for current platform
npm run build

# Platform-specific builds
npm run build:mac    # macOS .dmg + .zip
npm run build:win    # Windows installer + portable
npm run build:linux  # AppImage + .deb
```

Output: `dist/` directory

## Architecture

### Process Management

```
Main Process (main.js)
├── fuegod process
│   ├── RPC: http://127.0.0.1:18081
│   └── Data: ~/Library/Application Support/fuego-wallet/fuego-data
│
└── walletd process
    ├── RPC: http://127.0.0.1:18082
    ├── Container: ~/Library/Application Support/fuego-wallet/wallets/wallet.bin
    └── Connects to: fuegod @ 18081
```

### IPC Communication

**Main Process** (`main.js`)
- Spawns & manages `fuegod` and `walletd` processes
- Handles JSON-RPC calls to both daemons
- Manages wallet file operations
- Exposes IPC handlers for renderer

**Preload** (`preload.js`)
- Secure bridge between main and renderer
- Context isolation enabled
- Exposes `electronAPI` to renderer

**Renderer** (`renderer.html`)
- UI logic and state management
- Calls `electronAPI` methods
- Listens to real-time events

### Data Flow

```
User Action
    ↓
Renderer (renderer.html)
    ↓
IPC Bridge (preload.js)
    ↓
Main Process (main.js)
    ↓
JSON-RPC → fuegod/walletd
    ↓
Response → Renderer
    ↓
UI Update
```

## API Methods

### Node Control
```javascript
await electronAPI.startNode()
await electronAPI.stopNode()
await electronAPI.getNodeStatus()
```

### Wallet Management
```javascript
await electronAPI.createWallet(password)
await electronAPI.deleteWallet()
await electronAPI.restoreWallet(seed)
```

### Transactions
```javascript
await electronAPI.getBalance()
await electronAPI.getAddress()
await electronAPI.sendTransaction({ address, amount, fee })
await electronAPI.getTransactions(limit)
```

### Security
```javascript
await electronAPI.getMnemonic()
await electronAPI.exportKeys()
await electronAPI.saveFile({ content, filename })
```

### Events
```javascript
electronAPI.onNodeReady(() => { /* ... */ })
electronAPI.onWalletReady(() => { /* ... */ })
electronAPI.onWalletInfo((data) => { /* ... */ })
```

## JSON-RPC Methods Used

### fuegod (`getinfo`)
```json
{
  "height": 123456,
  "difficulty": 1000000,
  "hashrate": 500000,
  "incoming_connections_count": 8,
  "outgoing_connections_count": 10,
  "tx_pool_size": 5
}
```

### walletd
- `createAddress` - Create new address
- `getAddresses` - List all addresses
- `getBalance` - Get available & locked balance
- `sendTransaction` - Send XFG with fee
- `getTransactions` - Transaction history
- `getMnemonicSeed` - Export seed phrase
- `getViewKey` - Export view key
- `getSpendKeys` - Export spend key

## File Structure

```
electron-wallet/
├── main.js           # Main process (node/wallet management)
├── preload.js        # IPC bridge (security)
├── renderer.html     # UI & frontend logic
├── package.json      # Dependencies & build config
└── README.md         # This file
```

## Configuration

### Ports
- Node RPC: `18081`
- Wallet RPC: `18082`

### Data Locations
- **macOS**: `~/Library/Application Support/fuego-wallet/`
- **Windows**: `%APPDATA%\fuego-wallet\`
- **Linux**: `~/.config/fuego-wallet/`

```
fuego-wallet/
├── fuego-data/      # Blockchain data
│   ├── blocks/
│   └── ...
└── wallets/         # Wallet files
    ├── wallet.bin
    ├── wallet.bin.keys
    └── wallet.bin.address.txt
```

## Packaging Binaries

For production builds, place compiled binaries in:

```
electron-wallet/
└── binaries/
    ├── fuegod      # macOS/Linux
    ├── fuegod.exe  # Windows
    ├── walletd
    └── walletd.exe
```

electron-builder will copy these to app resources automatically.

## Troubleshooting

### Binary not found
**Development**: Make sure binaries are in `../build/src/`
```bash
cd ..
mkdir -p build/src
# Copy or symlink fuegod and walletd here
```

**Production**: Place binaries in `electron-wallet/binaries/`

### Wallet won't start
- Check if `fuegod` is running and synced
- Verify RPC ports are not in use
- Check logs in Settings tab

### Transaction fails
- Ensure sufficient balance (amount + fee)
- Wait for node to sync
- Check if wallet is unlocked

## Security Considerations

### Seed Phrase
- **Never share** your 25-word seed phrase
- **Store offline** in a secure location
- Seed can restore wallet on any device

### Private Keys
- View key: Read-only access to transactions
- Spend key: Full control of wallet
- **Both required** to fully restore wallet

### Best Practices
1. Backup seed phrase immediately after creation
2. Export and secure private keys
3. Use strong passwords (planned feature)
4. Keep app and binaries updated

## Roadmap

- [ ] Password protection for wallet
- [ ] Multiple wallets support
- [ ] Address book
- [ ] QR code generation/scanning
- [ ] Integrated Elderfier dashboard
- [ ] Burn2Mint UI integration
- [ ] Hardware wallet support
- [ ] Multi-language support

## License

MIT License - See main repository LICENSE file
