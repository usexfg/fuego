# 🚨 Eldernode Troubleshooting Guide

## 🎯 **Quick Problem Resolution**

This guide helps you solve common issues when setting up and running your Eldernode. Most problems can be resolved by following these step-by-step solutions.

## 🔍 **Problem Categories**

- [🔨 Build Issues](#-build-issues)
- [🔑 Key Generation Problems](#-key-generation-problems)
- [🌐 Network Connection Issues](#-network-connection-issues)
- [🤝 Consensus Failures](#-consensus-failures)
- [📊 Performance Problems](#-performance-problems)
- [🔒 Security Issues](#-security-issues)

---

## 🔨 **Build Issues**

### **Problem: CMake Configuration Fails**

**Symptoms:**
```bash
CMake Error: Could not find package 'Boost'
CMake Error: Could not find package 'OpenSSL'
CMake Error: Could not find package 'cURL'
```

**Solutions:**

#### **Ubuntu/Debian:**
```bash
# Install missing dependencies
sudo apt update
sudo apt install -y libssl-dev libboost-all-dev libcurl4-openssl-dev libjsoncpp-dev

# Clear CMake cache and retry
cd build
rm -rf CMakeCache.txt CMakeFiles/
cmake ..
```

#### **macOS:**
```bash
# Install missing packages
brew install boost openssl curl jsoncpp

# Specify paths if needed
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/opt/boost:/usr/local/opt/openssl
```

#### **Windows:**
- Install [vcpkg](https://github.com/microsoft/vcpkg) for package management
- Install required packages: `vcpkg install boost openssl curl jsoncpp`
- Configure with: `cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake`

### **Problem: Compilation Fails**

**Symptoms:**
```bash
error: 'std::string' does not name a type
error: 'std::vector' does not name a type
error: expected ';' before 'namespace'
```

**Solutions:**

#### **Check C++ Standard:**
```bash
# Verify compiler version
g++ --version
clang++ --version

# Should be GCC 9+ or Clang 10+
# If older, update your compiler
```

#### **Set C++ Standard:**
```bash
# In CMakeLists.txt, ensure:
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

#### **Clear Build and Retry:**
```bash
cd build
rm -rf *
cmake ..
make -j$(nproc)
```

### **Problem: Missing Header Files**

**Symptoms:**
```bash
fatal error: 'crypto/crypto.h' file not found
fatal error: 'Common/JsonValue.h' file not found
```

**Solutions:**

#### **Check Include Paths:**
```bash
# Verify the source structure
ls -la src/EldernodeRelayer/
ls -la include/

# Ensure all required headers exist
```

#### **Fix Include Paths:**
```bash
# In CMakeLists.txt, add:
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
```

---

## 🔑 **Key Generation Problems**

### **Problem: Cannot Generate Cryptographic Keys**

**Symptoms:**
```bash
❌ Failed to generate new key pair
❌ Error generating keys: [error message]
```

**Solutions:**

#### **Check File Permissions:**
```bash
# Verify write permissions
ls -la eldernode_keys.dat
chmod 600 eldernode_keys.dat

# Check directory permissions
ls -la .
chmod 755 .
```

#### **Check Disk Space:**
```bash
# Verify available space
df -h
du -sh .

# Free up space if needed
rm -rf build/CMakeFiles/
```

#### **Regenerate Keys:**
```bash
# Remove corrupted key file
rm eldernode_keys.dat

# Run Eldernode again
./src/EldernodeRelayer/EldernodeRelayer
```

### **Problem: Keys File Corrupted**

**Symptoms:**
```bash
❌ Invalid public key or signature format
❌ Error loading keys: [corruption message]
```

**Solutions:**

#### **Restore from Backup:**
```bash
# Copy from backup
cp ~/.fuego-eldernode/backup/eldernode_keys.dat .

# Or from Desktop
cp ~/Desktop/eldernode_keys.dat .
```

#### **Regenerate if No Backup:**
```bash
# Delete corrupted file
rm eldernode_keys.dat

# Generate new keys
./src/EldernodeRelayer/EldernodeRelayer
```

---

## 🌐 **Network Connection Issues**

### **Problem: Cannot Connect to Other Eldernodes**

**Symptoms:**
```bash
❌ Cannot connect to eldernode1.fuego.network:8070
❌ Network connection failed
```

**Solutions:**

#### **Check Network Connectivity:**
```bash
# Test basic connectivity
ping eldernode1.fuego.network

# Test port accessibility
telnet eldernode1.fuego.network 8070
nc -zv eldernode1.fuego.network 8070
```

#### **Check Firewall Settings:**
```bash
# Ubuntu/Debian
sudo ufw status
sudo ufw allow 8070

# macOS
sudo pfctl -s rules
# Add rule to /etc/pf.conf

# Windows
# Check Windows Firewall settings
```

#### **Verify Eldernode Registry:**
```bash
# Check registry file
cat ~/.fuego-eldernode/eldernode_registry.txt

# Test each Eldernode individually
curl -X POST http://eldernode1.fuego.network:8070/json_rpc \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"getEldernodeStatistics"}'
```

### **Problem: Port 8070 Not Accessible**

**Symptoms:**
```bash
❌ Port 8070 is not accessible
❌ Connection refused on port 8070
```

**Solutions:**

#### **Check Port Usage:**
```bash
# Check if port is in use
sudo netstat -tlnp | grep :8070
sudo lsof -i :8070

# Kill process if needed
sudo kill -9 [PID]
```

#### **Configure Firewall:**
```bash
# Ubuntu/Debian
sudo ufw allow 8070/tcp
sudo ufw reload

# macOS
sudo pfctl -f /etc/pf.conf
sudo pfctl -e

# Windows
# Add rule in Windows Firewall
```

#### **Check Service Status:**
```bash
# Verify Eldernode is running
sudo systemctl status fuego-eldernode

# Check if service is listening
sudo ss -tlnp | grep :8070
```

---

## 🤝 **Consensus Failures**

### **Problem: 2/2 Consensus Always Fails**

**Symptoms:**
```bash
❌ Fast 2/2 consensus failed
❌ Not enough valid proofs for 2/2 consensus
```

**Solutions:**

#### **Check Eldernode Registry:**
```bash
# Verify registry has enough Eldernodes
cat ~/.fuego-eldernode/eldernode_registry.txt | grep -v '^#' | wc -l

# Should have at least 2 active Eldernodes
```

#### **Test Individual Eldernodes:**
```bash
# Test each Eldernode individually
for eldernode in $(cat ~/.fuego-eldernode/eldernode_registry.txt | grep -v '^#'); do
    echo "Testing $eldernode..."
    curl -X POST $eldernode/json_rpc \
      -H "Content-Type: application/json" \
      -d '{"jsonrpc":"2.0","id":1,"method":"getEldernodeStatistics"}'
done
```

#### **Check Network Conditions:**
```bash
# Monitor network latency
ping -c 10 eldernode1.fuego.network

# Check for packet loss
mtr eldernode1.fuego.network
```

### **Problem: 3/5 Consensus Fails**

**Symptoms:**
```bash
❌ Not enough valid proofs for 3/5 consensus
❌ 3/5 consensus validation failed
```

**Solutions:**

#### **Ensure Minimum Eldernodes:**
```bash
# Check available Eldernodes
curl -X POST http://localhost:8070/json_rpc \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"discoverEldernodes","params":{"minCount":5}}'
```

#### **Verify Consensus Configuration:**
```bash
# Check configuration file
cat ~/.fuego-eldernode/eldernode.conf | grep consensus

# Should show:
# consensus_threshold = 3
# min_eldernodes = 5
```

#### **Test Consensus Manually:**
```bash
# Run consensus test
./scripts/test_progressive_consensus.sh

# Check detailed output for errors
```

---

## 📊 **Performance Problems**

### **Problem: Slow Consensus Response**

**Symptoms:**
```bash
# Taking longer than expected
# 2/2 consensus: >500ms (should be ~200ms)
# 3/5 consensus: >1000ms (should be ~500ms)
```

**Solutions:**

#### **Optimize System Settings:**
```bash
# Increase file descriptor limits
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf

# Optimize network settings
echo "net.core.rmem_max = 16777216" | sudo tee -a /etc/sysctl.conf
echo "net.core.wmem_max = 16777216" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

#### **Check Resource Usage:**
```bash
# Monitor system resources
htop
iotop
nethogs

# Check for bottlenecks
iostat -x 1
vmstat 1
```

#### **Optimize Network:**
```bash
# Use closer Eldernodes
# Update registry with geographically closer nodes
nano ~/.fuego-eldernode/eldernode_registry.txt
```

### **Problem: High Memory Usage**

**Symptoms:**
```bash
# Memory usage >80%
# System becomes unresponsive
```

**Solutions:**

#### **Check Memory Usage:**
```bash
# Monitor memory
free -h
top -p $(pgrep EldernodeRelayer)

# Check for memory leaks
valgrind --tool=memcheck ./src/EldernodeRelayer/EldernodeRelayer
```

#### **Optimize Configuration:**
```bash
# Reduce block scan interval
# In eldernode.conf:
block_scan_interval_seconds = 5  # Increase from 2 to 5

# Reduce max confirmations
max_confirmations = 50  # Reduce from 100 to 50
```

---

## 🔒 **Security Issues**

### **Problem: Private Key Exposure**

**Symptoms:**
```bash
# Keys file has wrong permissions
# Keys accessible to other users
```

**Solutions:**

#### **Fix File Permissions:**
```bash
# Set correct permissions
chmod 600 eldernode_keys.dat
chown $USER:$USER eldernode_keys.dat

# Verify permissions
ls -la eldernode_keys.dat
# Should show: -rw------- (600)
```

#### **Secure Key Storage:**
```bash
# Move keys to secure location
sudo mkdir -p /opt/fuego-eldernode/keys
sudo mv eldernode_keys.dat /opt/fuego-eldernode/keys/
sudo chown $USER:$USER /opt/fuego-eldernode/keys/eldernode_keys.dat

# Update configuration
echo "key_file_path = \"/opt/fuego-eldernode/keys/eldernode_keys.dat\"" >> ~/.fuego-eldernode/eldernode.conf
```

### **Problem: Unauthorized Access**

**Symptoms:**
```bash
# Unknown connections to port 8070
# Unauthorized RPC calls
```

**Solutions:**

#### **Restrict Network Access:**
```bash
# Configure firewall to allow only specific IPs
sudo ufw deny 8070
sudo ufw allow from [trusted-ip] to any port 8070

# Or use iptables for more granular control
sudo iptables -A INPUT -p tcp --dport 8070 -s [trusted-ip] -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8070 -j DROP
```

#### **Enable Authentication:**
```bash
# Add basic authentication to configuration
echo "rpc_username = \"your_username\"" >> ~/.fuego-eldernode/eldernode.conf
echo "rpc_password = \"your_secure_password\"" >> ~/.fuego-eldernode/eldernode.conf
```

---

## 🆘 **Getting More Help**

### **Collect Debug Information:**
```bash
# Create debug report
cat > debug_report.txt << 'EOF'
=== Eldernode Debug Report ===
Date: $(date)
User: $(whoami)
OS: $(uname -a)
Eldernode Version: $(./src/EldernodeRelayer/EldernodeRelayer --version 2>/dev/null || echo "Unknown")

=== System Resources ===
Memory: $(free -h)
Disk: $(df -h)
CPU: $(nproc) cores

=== Configuration ===
Config File: $(cat ~/.fuego-eldernode/eldernode.conf)
Registry: $(cat ~/.fuego-eldernode/eldernode_registry.txt)

=== Logs ===
Recent Logs:
$(tail -50 ~/.fuego-eldernode/eldernode.log 2>/dev/null || echo "No logs found")

=== Network ===
Port 8070: $(sudo netstat -tlnp | grep :8070 || echo "Port not listening")
EOF

echo "Debug report created: debug_report.txt"
```

### **Support Channels:**
- **GitHub Issues**: Report bugs with debug information
- **Community Forums**: Ask for help from other operators
- **Documentation**: Check comprehensive guides
- **Logs**: Review detailed error messages

---

## 🎯 **Prevention Tips**

### **Regular Maintenance:**
- 🔄 **Update software** regularly
- 📊 **Monitor performance** metrics
- 🔐 **Rotate keys** every 30-90 days
- 💾 **Backup configuration** and keys
- 📝 **Review logs** for anomalies

### **Best Practices:**
- 🛡️ **Use firewalls** to restrict access
- 🔒 **Secure key storage** with proper permissions
- 🌐 **Monitor network** connectivity
- 📈 **Track consensus** performance
- 🚨 **Set up alerts** for failures

---

**🔧 Most issues can be resolved by following these solutions. If you continue to have problems, collect the debug information and reach out to the community for help!**
