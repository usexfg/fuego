# 🆓 Free Hosting Guide for Fuego Nodes

Run your own Fuego blockchain node completely **FREE** using cloud providers' free tiers!

## Oracle Cloud Always Free

<sup>⚠️ important - using the method described below only works if the 'region' you select as home has 'available' free resources. Regions US east/west/mid are likely unavailable. Supposedly, Germany Central (Frankfurt), UK South (London), and Canada Southeast (Montreal) have greater possibilities for available free resources, but also likely 'exhausted' by now.  YOU CANNOT CHANGE HOME REGIONS ONCE SIGNED UP. If anyone has any new info please add a PR or comment below.</sup>

### Why Oracle Cloud?
- ✅ **Permanent free** (not a trial)
- ✅ **4 ARM CPUs + 24GB RAM** available
- ✅ **200GB storage** included
- ✅ **ARM64 support** (we have ARM builds!)
- ✅ **No credit card expiration issues**

### Quick Setup (5 minutes)

1. **Create Oracle Cloud Account**: [cloud.oracle.com](https://cloud.oracle.com)
2. **Launch ARM Instance**: 
   - Shape: `VM.Standard.A1.Flex` (4 CPUs, 24GB RAM)
   - Image: `Ubuntu 22.04 LTS`
   - Storage: 200GB Block Volume
3. **Run Setup Script**:
```bash
# Download and run setup script
wget https://raw.githubusercontent.com/usexfg/fuego/master/docs/scripts/oracle-cloud-setup.sh
chmod +x oracle-cloud-setup.sh

# For mainnet:
./oracle-cloud-setup.sh mainnet 8080

# For testnet:
./oracle-cloud-setup.sh testnet 8080
```

4. **Access Your Node**:
   - Web Dashboard: `http://YOUR_IP:8080`
   - RPC Endpoint: `http://YOUR_IP:18180/json_rpc`

---

## 🔥 Alternative Free Options

### Google Cloud Platform (GCP)
```bash
# $300 credit + always-free f1-micro
# Good for testnet or light usage
Instance: f1-micro (1 vCPU, 0.6GB RAM)
Setup: Use our Docker image
```

### AWS Free Tier
```bash
# 12 months free t2.micro
# Limited but works for development
Instance: t2.micro (1 vCPU, 1GB RAM)
Setup: Use our Docker image
```

### DigitalOcean
```bash
# $200 credit for 60 days
# Then $4/month for basic droplet
Instance: Basic Droplet (1 vCPU, 1GB RAM)
Setup: Use our Docker image
```

---

## 🐳 Docker Deployment (Any Platform)

### Quick Start
```bash
# Create data directory
mkdir fuego-data

# Run mainnet node
docker run -d --name fuego-node \
  -p 10808:10808 -p 18180:18180 \
  -v $(pwd)/fuego-data:/home/fuego/.fuego \
  ghcr.io/usexfg/fuego:latest

# Run testnet node
docker run -d --name fuego-testnet \
  -p 10809:20808 -p 28282:28282 \
  -v $(pwd)/fuego-testnet-data:/home/fuego/.fuego \
  ghcr.io/usexfg/fuego:latest \
  fuegod --testnet --data-dir=/home/fuego/.fuego
```

### With Web Interface
```bash
# Clone repository for web files
git clone https://github.com/usexfg/fuego.git
cd fuego

# Use our complete setup
docker-compose up -d
```

---

## 📊 Resource Requirements

### Minimum Requirements
| Component | Mainnet | Testnet |
|-----------|---------|---------|
| **CPU** | 1 core | 1 core |
| **RAM** | 2GB | 1GB |
| **Storage** | 50GB+ | 10GB+ |
| **Bandwidth** | 100GB/month | 50GB/month |

### Recommended (Oracle Free Tier)
| Component | Specs |
|-----------|-------|
| **CPU** | 4 ARM cores |
| **RAM** | 24GB |
| **Storage** | 200GB |
| **Bandwidth** | 10TB/month |

---

## 🔧 Advanced Configuration

### Environment Variables
```bash
# Mainnet with custom ports
docker run -d \
  -e FUEGO_P2P_PORT=10808 \
  -e FUEGO_RPC_PORT=18180 \
  -e FUEGO_DATA_DIR=/data \
  ghcr.io/usexfg/fuego:latest

# Testnet with logging
docker run -d \
  -e FUEGO_TESTNET=true \
  -e FUEGO_LOG_LEVEL=2 \
  ghcr.io/usexfg/fuego:latest
```

### Custom Configuration
```bash
# Mount custom config
docker run -d \
  -v $(pwd)/fuego.conf:/home/fuego/.fuego/fuego.conf \
  ghcr.io/usexfg/fuego:latest
```

---

## 🌐 Web Interface Features

### Dashboard Includes:
- ✅ **Real-time node status**
- ✅ **Block height tracking**
- ✅ **Network hash rate**
- ✅ **Connected peers count**
- ✅ **Difficulty monitoring**
- ✅ **Mainnet/Testnet detection**

### API Endpoints:
```javascript
// Get node info
fetch('/json_rpc', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    jsonrpc: '2.0',
    id: '1',
    method: 'getinfo'
  })
})

// Get block by height
fetch('/json_rpc', {
  method: 'POST',
  body: JSON.stringify({
    jsonrpc: '2.0',
    id: '1',
    method: 'getblockbyheight',
    params: { height: 12345 }
  })
})
```

---

## 🔒 Security Best Practices

### Firewall Configuration
```bash
# Allow only necessary ports
sudo ufw allow ssh
sudo ufw allow 8080   # Web interface
sudo ufw allow 10808  # P2P port
sudo ufw enable
```

### SSL/HTTPS Setup (Optional)
```bash
# Using Let's Encrypt with Caddy
docker run -d --name caddy \
  -p 80:80 -p 443:443 \
  -v caddy_data:/data \
  -v $(pwd)/Caddyfile:/etc/caddy/Caddyfile \
  caddy:alpine
```

### Monitoring
```bash
# View logs
docker logs -f fuego-node

# Check resource usage
docker stats fuego-node

# Health check
curl http://localhost:18180/json_rpc \
  -d '{"jsonrpc":"2.0","id":"1","method":"getinfo"}'
```

---

## 🎯 Use Cases

### 1. **Development & Testing**
- Test applications against live blockchain
- Develop dApps with reliable RPC endpoint
- Experiment with testnet safely

### 2. **Network Participation**
- Help secure the Fuego network
- Provide RPC services to community
- Run mining pool backend

### 3. **Business Applications**
- Payment processing integration
- Blockchain analytics
- Wallet backend services

---

## 🆘 Troubleshooting

### Common Issues

**Node won't sync:**
```bash
# Check peers
docker exec fuego-node fuegod --help
# Ensure ports are open
sudo ufw status
```

**High memory usage:**
```bash
# Limit container memory
docker run --memory=2g ghcr.io/usexfg/fuego:latest
```

**Storage full:**
```bash
# Clean old data (CAUTION: This removes blockchain data)
docker volume prune
```

### Getting Help
- 💬 **Discord**: [Fuego Community](https://discord.usexfg.org)
- 📖 **Documentation**: [docs.fuego.network](https://docs.usexfg.org)
- 🐛 **Issues**: [GitHub Issues](https://github.com/usexfg/fuego/issues)

---

## 💡 Pro Tips

1. **Use Oracle Cloud ARM instances** - Best free specs available
2. **Monitor resource usage** - Set up alerts for storage/bandwidth
3. **Enable blockchain indexes** - Better performance for RPC calls
4. **Regular backups** - Export wallet files and important data
5. **Update regularly** - Keep your node software current

---

**🔥 Start hosting your free Fuego node today!**

*Total setup time: ~5 minutes | Total cost: $0 | Network contribution: Priceless🏴‍☠️* 
