const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');
const fetch = require('node-fetch');

let mainWindow;
let nodeProcess = null;
let walletProcess = null;
let nodeReady = false;

const NODE_RPC_PORT = 18081;
const WALLET_RPC_PORT = 18082;

// Get binary paths
function getBinaryPath(name) {
  const isDev = process.argv.includes('--dev');
  if (isDev) {
    // Development mode - use build directory
    return path.join(__dirname, '..', 'build', 'src', name);
  } else {
    // Production mode - use packaged binaries
    return path.join(process.resourcesPath, 'binaries', name);
  }
}

// Get data directory
function getDataDir() {
  return path.join(app.getPath('userData'), 'fuego-data');
}

// Ensure data directory exists
function ensureDataDir() {
  const dataDir = getDataDir();
  if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
  }
  return dataDir;
}

// Create main window
function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    minWidth: 600,
    minHeight: 400,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    },
    titleBarStyle: 'hiddenInset',
    backgroundColor: '#1a1a1a'
  });

  mainWindow.loadFile('renderer.html');
  
  if (process.argv.includes('--dev')) {
    mainWindow.webContents.openDevTools();
  }
}

app.whenReady().then(() => {
  createWindow();
  ensureDataDir();
});

app.on('window-all-closed', () => {
  if (nodeProcess) nodeProcess.kill();
  if (walletProcess) walletProcess.kill();
  app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});

// JSON-RPC helper
async function jsonRpc(port, method, params = {}) {
  try {
    const response = await fetch(`http://127.0.0.1:${port}/json_rpc`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        jsonrpc: '2.0',
        id: '0',
        method: method,
        params: params
      })
    });
    const data = await response.json();
    return data.result || data;
  } catch (error) {
    console.error('RPC Error:', error);
    throw error;
  }
}

// IPC Handlers
ipcMain.handle('start-node', async () => {
  if (nodeProcess) {
    return { status: 'already-running', message: 'Node is already running' };
  }

  const dataDir = getDataDir();
  const fuegodPath = getBinaryPath('fuegod');
  
  return new Promise((resolve) => {
    nodeProcess = spawn(fuegodPath, [
      `--data-dir=${dataDir}`,
      `--rpc-bind-port=${NODE_RPC_PORT}`,
      '--restricted-rpc',
      '--enable-cors=*',
      '--log-level=1'
    ]);

    nodeProcess.stdout.on('data', (data) => {
      const msg = data.toString();
      console.log('[NODE]', msg);
      mainWindow.webContents.send('node-log', msg);
      
      if (msg.includes('Core initialized OK') || msg.includes('Node started')) {
        nodeReady = true;
      }
    });

    nodeProcess.stderr.on('data', (data) => {
      console.error('[NODE ERROR]', data.toString());
      mainWindow.webContents.send('node-error', data.toString());
    });

    nodeProcess.on('close', (code) => {
      console.log(`Node process exited with code ${code}`);
      nodeProcess = null;
      nodeReady = false;
      mainWindow.webContents.send('node-stopped');
    });

    setTimeout(() => {
      resolve({ 
        status: 'starting', 
        message: 'Node is starting...',
        port: NODE_RPC_PORT 
      });
    }, 1000);
  });
});

ipcMain.handle('stop-node', async () => {
  if (!nodeProcess) {
    return { status: 'not-running', message: 'Node is not running' };
  }
  
  nodeProcess.kill('SIGTERM');
  nodeProcess = null;
  nodeReady = false;
  
  return { status: 'stopped', message: 'Node stopped successfully' };
});

ipcMain.handle('get-node-status', async () => {
  if (!nodeProcess || !nodeReady) {
    return { running: false, height: 0, peers: 0 };
  }

  try {
    const info = await jsonRpc(NODE_RPC_PORT, 'get_info');
    return {
      running: true,
      height: info.height || 0,
      peers: info.incoming_connections_count + info.outgoing_connections_count || 0,
      difficulty: info.difficulty || 0,
      hashrate: info.hashrate || 0
    };
  } catch (error) {
    return { running: false, height: 0, peers: 0 };
  }
});

ipcMain.handle('start-wallet-rpc', async () => {
  if (walletProcess) {
    return { status: 'already-running' };
  }

  const dataDir = getDataDir();
  const walletdPath = getBinaryPath('walletd');
  const walletFile = path.join(dataDir, 'wallet.bin');

  walletProcess = spawn(walletdPath, [
    `--container-file=${walletFile}`,
    `--container-password=`,
    `--rpc-bind-port=${WALLET_RPC_PORT}`,
    '--daemon-port=' + NODE_RPC_PORT,
    '--log-level=1'
  ]);

  walletProcess.stdout.on('data', (data) => {
    console.log('[WALLET]', data.toString());
  });

  walletProcess.stderr.on('data', (data) => {
    console.error('[WALLET ERROR]', data.toString());
  });

  return { status: 'started', port: WALLET_RPC_PORT };
});

ipcMain.handle('create-wallet', async (event, password) => {
  const dataDir = getDataDir();
  const walletFile = path.join(dataDir, 'wallet.bin');
  
  // Check if wallet already exists
  if (fs.existsSync(walletFile)) {
    return { 
      status: 'exists', 
      message: 'Wallet already exists. Use restore or delete existing wallet.' 
    };
  }

  try {
    // Create wallet via RPC
    await jsonRpc(WALLET_RPC_PORT, 'create_address');
    const addresses = await jsonRpc(WALLET_RPC_PORT, 'get_addresses');
    
    return {
      status: 'created',
      address: addresses.addresses[0],
      message: 'Wallet created successfully'
    };
  } catch (error) {
    return { status: 'error', message: error.message };
  }
});

ipcMain.handle('get-balance', async () => {
  try {
    const balance = await jsonRpc(WALLET_RPC_PORT, 'get_balance');
    return {
      available: (balance.availableBalance || 0) / 100000000,
      locked: (balance.lockedAmount || 0) / 100000000
    };
  } catch (error) {
    return { available: 0, locked: 0 };
  }
});

ipcMain.handle('get-address', async () => {
  try {
    const addresses = await jsonRpc(WALLET_RPC_PORT, 'get_addresses');
    return { address: addresses.addresses[0] };
  } catch (error) {
    return { address: '' };
  }
});

ipcMain.handle('send-transaction', async (event, { address, amount, fee = 0.1 }) => {
  try {
    const amountAtomic = Math.floor(amount * 100000000);
    const feeAtomic = Math.floor(fee * 100000000);
    
    const result = await jsonRpc(WALLET_RPC_PORT, 'send_transaction', {
      transfers: [{ address, amount: amountAtomic }],
      fee: feeAtomic,
      anonymity: 4
    });
    
    return {
      status: 'success',
      transactionHash: result.transactionHash,
      message: 'Transaction sent successfully'
    };
  } catch (error) {
    return {
      status: 'error',
      message: error.message
    };
  }
});

ipcMain.handle('open-dialog', async (event, options) => {
  const result = await dialog.showMessageBox(mainWindow, options);
  return result;
});
