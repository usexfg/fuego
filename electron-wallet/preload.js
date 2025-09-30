const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods to renderer process
contextBridge.exposeInMainWorld('electronAPI', {
  // Node controls
  startNode: () => ipcRenderer.invoke('start-node'),
  stopNode: () => ipcRenderer.invoke('stop-node'),
  getNodeStatus: () => ipcRenderer.invoke('get-node-status'),
  
  // Wallet RPC
  startWalletRpc: () => ipcRenderer.invoke('start-wallet-rpc'),
  createWallet: (password) => ipcRenderer.invoke('create-wallet', password),
  deleteWallet: () => ipcRenderer.invoke('delete-wallet'),
  restoreWallet: (seed) => ipcRenderer.invoke('restore-wallet', { seed }),
  
  // Balance & Address
  getBalance: () => ipcRenderer.invoke('get-balance'),
  getAddress: () => ipcRenderer.invoke('get-address'),
  
  // Transactions
  sendTransaction: (data) => ipcRenderer.invoke('send-transaction', data),
  getTransactions: (limit) => ipcRenderer.invoke('get-transactions', { limit }),
  
  // Security & Backup
  getMnemonic: () => ipcRenderer.invoke('get-mnemonic'),
  exportKeys: () => ipcRenderer.invoke('export-keys'),
  
  // Dialogs
  openDialog: (options) => ipcRenderer.invoke('open-dialog', options),
  saveFile: (data) => ipcRenderer.invoke('save-file', data),
  
  // Event listeners
  onNodeLog: (callback) => ipcRenderer.on('node-log', (event, data) => callback(data)),
  onNodeError: (callback) => ipcRenderer.on('node-error', (event, data) => callback(data)),
  onNodeStopped: (callback) => ipcRenderer.on('node-stopped', () => callback()),
  onNodeReady: (callback) => ipcRenderer.on('node-ready', () => callback()),
  
  onWalletLog: (callback) => ipcRenderer.on('wallet-log', (event, data) => callback(data)),
  onWalletError: (callback) => ipcRenderer.on('wallet-error', (event, data) => callback(data)),
  onWalletStopped: (callback) => ipcRenderer.on('wallet-stopped', () => callback()),
  onWalletReady: (callback) => ipcRenderer.on('wallet-ready', () => callback()),
  onWalletInfo: (callback) => ipcRenderer.on('wallet-info', (event, data) => callback(data))
});
