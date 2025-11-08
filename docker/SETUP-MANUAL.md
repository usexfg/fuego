# 🔥 Fuego Testnet Manual Setup (Alternative to Docker)

Since Docker Desktop seems to have issues, here's how to set up the Fuego testnet manually using the source code.

## 🚀 Quick Start

### **Prerequisites**
- macOS with Xcode Command Line Tools
- CMake 3.10+
- Boost libraries
- Git

### **Install Dependencies**
```bash
# Install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install cmake boost git

# Install Xcode Command Line Tools
xcode-select --install
```

### **Build Fuego**
```bash
# Navigate to the fuego-fresh directory
cd /Users/aejt/fuegowalletproof/fuego-fresh

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..

# Build (this may take 10-20 minutes)
make -j$(nproc)
```

### **Start Testnet Node**
```bash
# Create data directory
mkdir -p ~/.fuego-testnet

# Start the daemon in testnet mode
./src/fuegod \
  --data-dir=~/.fuego-testnet \
  --rpc-bind-ip=0.0.0.0 \
  --rpc-bind-port=28180 \
  --p2p-bind-ip=0.0.0.0 \
  --p2p-bind-port=20808 \
  --testnet \
  --log-level=2
```

### **Start Wallet Service**
```bash
# In a new terminal, start the wallet service
./src/walletd \
  --daemon-host=localhost \
  --daemon-port=28180 \
  --bind-port=8070 \
  --bind-ip=0.0.0.0 \
  --wallet-dir=~/.fuego-testnet-wallet \
  --testnet \
  --log-level=2
```

## 🧪 **Testing Dynamic Supply**

### **Test RPC Endpoints**
```bash
# Get node info
curl -X POST http://localhost:28180/getinfo

# Get dynamic supply overview
curl -X POST http://localhost:28180/getDynamicSupplyOverview

# Get wallet info
curl -X POST http://localhost:8070/getinfo
```

### **Create Test Wallet**
```bash
# Create a new wallet
curl -X POST http://localhost:8070/createAddress \
  -H "Content-Type: application/json" \
  -d '{}'
```

## 🔧 **Docker Troubleshooting**

If you want to fix Docker Desktop:

### **Option 1: Reinstall Docker Desktop**
```bash
# Remove existing Docker Desktop
sudo rm -rf /Applications/Docker.app
sudo rm -rf ~/.docker

# Download and install from https://www.docker.com/products/docker-desktop/
```

### **Option 2: Use Docker CLI Only**
```bash
# Install Docker CLI via Homebrew
brew install docker

# Start Docker daemon manually (requires sudo)
sudo dockerd
```

### **Option 3: Use Colima (Docker Alternative)**
```bash
# Install Colima
brew install colima

# Start Colima
colima start

# Use Docker commands normally
docker ps
```

## 📋 **Manual Setup vs Docker**

| Feature | Manual Setup | Docker Setup |
|---------|-------------|--------------|
| **Setup Time** | 15-30 minutes | 5-10 minutes |
| **Dependencies** | System-wide | Isolated |
| **Resource Usage** | Lower | Higher |
| **Portability** | System-specific | Cross-platform |
| **Maintenance** | Manual updates | Automated |

## 🛠️ **Useful Commands**

### **Manual Setup**
```bash
# Check if daemon is running
ps aux | grep fuegod

# View logs
tail -f ~/.fuego-testnet/fuego.log

# Stop daemon
pkill fuegod

# Check ports
netstat -tulpn | grep :28180
```

### **Docker Setup (when working)**
```bash
# Start testnet
cd docker && ./setup-testnet.sh

# View logs
docker logs fuego-testnet-node

# Stop testnet
docker-compose -f docker-compose.testnet.yml down
```

## 🔍 **Monitoring**

### **Check Node Status**
```bash
# Get node info
curl -X POST http://localhost:28180/getinfo | jq

# Get block count
curl -X POST http://localhost:28180/getblockcount | jq

# Get dynamic supply
curl -X POST http://localhost:28180/getDynamicSupplyOverview | jq
```

### **Check Wallet Status**
```bash
# Get wallet info
curl -X POST http://localhost:8070/getinfo | jq

# List addresses
curl -X POST http://localhost:8070/getAddresses | jq
```

## 🐛 **Troubleshooting**

### **Build Issues**
```bash
# Clean build
cd build && make clean && make -j$(nproc)

# Check CMake version
cmake --version

# Check Boost installation
brew list | grep boost
```

### **Runtime Issues**
```bash
# Check if ports are available
lsof -i :28180
lsof -i :8070

# Check disk space
df -h

# Check memory usage
top -l 1 | head -10
```

## 📚 **Next Steps**

Once your testnet is running:

1. **Test Dynamic Supply** - Create FOREVER deposits and monitor supply changes
2. **Test RPC APIs** - Use the provided endpoints to interact with the node
3. **Monitor Logs** - Watch for any errors or performance issues
4. **Experiment** - Try different deposit amounts and terms

---

**🔥 Happy Testing! 🚀**
