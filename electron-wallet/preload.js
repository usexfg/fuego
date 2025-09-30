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
  getBalance: () => ipcRenderer.invoke('get-balance'),
  getAddress: () => ipcRenderer.invoke('get-address'),
  sendTransaction: (data) => ipcRenderer.invoke('send-transaction', data),
  
  // Dialogs
  openDialog: (options) => ipcRenderer.invoke('open-dialog', options),
  
  // Event listeners
  onNodeLog: (callback) => ipcRenderer.on('node-log', (event, data) => callback(data)),
  onNodeError: (callback) => ipcRenderer.on('node-error', (event, data) => callback(data)),
  onNodeStopped: (callback) => ipcRenderer.on('node-stopped', () => callback())
});
