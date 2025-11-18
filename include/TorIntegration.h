#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>

namespace CryptoNote {

// Forward declarations
class TorManager;
class TorConnection;
class TorConfig;

/**
 * @brief Tor connection status enumeration
 */
enum class TorStatus {
    DISCONNECTED,   // Not connected to Tor
    CONNECTING,     // Attempting to connect
    CONNECTED,      // Successfully connected
    ERROR,          // Connection error
    UNKNOWN         // Status unknown
};

/**
 * @brief Tor configuration structure
 */
struct TorConfig {
    bool enabled = false;                    // Enable Tor integration
    std::string socksHost = "127.0.0.1";     // SOCKS5 proxy host
    uint16_t socksPort = 9050;              // SOCKS5 proxy port
    std::string controlHost = "127.0.0.1";  // Tor control host
    uint16_t controlPort = 9051;            // Tor control port
    std::string dataDirectory = "";         // Tor data directory
    std::string hiddenServiceDir = "";      // Hidden service directory
    uint16_t hiddenServicePort = 8081;      // Hidden service port
    bool autoStart = false;                 // Auto-start Tor if not running
    uint32_t connectionTimeout = 30000;    // Connection timeout (ms)
    uint32_t circuitTimeout = 60000;       // Circuit timeout (ms)
    bool enableHiddenService = false;       // Enable hidden service
    std::string hiddenServiceAddress = "";  // Hidden service address
};

/**
 * @brief Tor connection information
 */
struct TorConnectionInfo {
    std::string address;                     // Connection address
    uint16_t port = 0;                      // Connection port
    std::string onionAddress;               // Onion address (if applicable)
    TorStatus status = TorStatus::UNKNOWN;  // Connection status
    uint32_t latency = 0;                  // Connection latency (ms)
    std::string errorMessage;              // Error message (if any)
};

/**
 * @brief Tor statistics
 */
struct TorStats {
    uint32_t totalConnections = 0;          // Total connections made
    uint32_t successfulConnections = 0;     // Successful connections
    uint32_t failedConnections = 0;        // Failed connections
    uint32_t bytesTransferred = 0;          // Total bytes transferred
    uint32_t averageLatency = 0;            // Average connection latency
    uint32_t circuitCount = 0;              // Active circuit count
    std::string torVersion;                 // Tor version string
};

/**
 * @brief Tor event callback types
 */
using TorStatusCallback = std::function<void(TorStatus status, const std::string& message)>;
using TorConnectionCallback = std::function<void(const TorConnectionInfo& info)>;
using TorErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Main Tor integration manager
 */
class TorManager {
public:
    /**
     * @brief Constructor
     * @param config Tor configuration
     */
    explicit TorManager(const TorConfig& config);
    
    /**
     * @brief Destructor
     */
    ~TorManager();
    
    /**
     * @brief Initialize Tor integration
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown Tor integration
     */
    void shutdown();
    
    /**
     * @brief Check if Tor is available and running
     * @return true if Tor is available
     */
    bool isTorAvailable() const;
    
    /**
     * @brief Get current Tor status
     * @return Current status
     */
    TorStatus getStatus() const;
    
    /**
     * @brief Get Tor statistics
     * @return Current statistics
     */
    TorStats getStats() const;
    
    /**
     * @brief Create a Tor connection
     * @param address Target address
     * @param port Target port
     * @return Connection info
     */
    TorConnectionInfo createConnection(const std::string& address, uint16_t port);
    
    /**
     * @brief Get hidden service address
     * @return Hidden service address (empty if not enabled)
     */
    std::string getHiddenServiceAddress() const;
    
    /**
     * @brief Set status callback
     * @param callback Status change callback
     */
    void setStatusCallback(TorStatusCallback callback);
    
    /**
     * @brief Set connection callback
     * @param callback Connection event callback
     */
    void setConnectionCallback(TorConnectionCallback callback);
    
    /**
     * @brief Set error callback
     * @param callback Error event callback
     */
    void setErrorCallback(TorErrorCallback callback);
    
    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if update successful
     */
    bool updateConfig(const TorConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    TorConfig getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Individual Tor connection
 */
class TorConnection {
public:
    /**
     * @brief Constructor
     * @param manager Tor manager reference
     * @param address Target address
     * @param port Target port
     */
    TorConnection(TorManager& manager, const std::string& address, uint16_t port);
    
    /**
     * @brief Destructor
     */
    ~TorConnection();
    
    /**
     * @brief Connect to target
     * @return true if connection successful
     */
    bool connect();
    
    /**
     * @brief Disconnect from target
     */
    void disconnect();
    
    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Send data
     * @param data Data to send
     * @param size Data size
     * @return Number of bytes sent
     */
    size_t send(const void* data, size_t size);
    
    /**
     * @brief Receive data
     * @param buffer Buffer to receive data
     * @param size Buffer size
     * @return Number of bytes received
     */
    size_t receive(void* buffer, size_t size);
    
    /**
     * @brief Get connection info
     * @return Connection information
     */
    TorConnectionInfo getInfo() const;
    
    /**
     * @brief Get connection latency
     * @return Latency in milliseconds
     */
    uint32_t getLatency() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Tor utility functions
 */
namespace TorUtils {
    /**
     * @brief Check if Tor is installed on system
     * @return true if Tor is installed
     */
    bool isTorInstalled();
    
    /**
     * @brief Get Tor version string
     * @return Version string (empty if not available)
     */
    std::string getTorVersion();
    
    /**
     * @brief Generate random onion address
     * @return Random onion address
     */
    std::string generateOnionAddress();
    
    /**
     * @brief Validate onion address format
     * @param address Address to validate
     * @return true if valid onion address
     */
    bool isValidOnionAddress(const std::string& address);
    
    /**
     * @brief Convert regular address to onion address
     * @param address Regular address
     * @return Onion address (if available)
     */
    std::string resolveToOnion(const std::string& address);
    
    /**
     * @brief Get default Tor configuration
     * @return Default configuration
     */
    TorConfig getDefaultConfig();
    
    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return Loaded configuration
     */
    TorConfig loadConfigFromFile(const std::string& filename);
    
    /**
     * @brief Save configuration to file
     * @param config Configuration to save
     * @param filename Configuration file path
     * @return true if save successful
     */
    bool saveConfigToFile(const TorConfig& config, const std::string& filename);
}

} // namespace CryptoNote
