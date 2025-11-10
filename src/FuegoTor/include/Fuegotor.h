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
class FuegoTorManager;
class FuegoTorConnection;
class FuegoTorConfig;

/**
 * @brief FuegoTor connection status enumeration
 */
enum class FuegoTorStatus {
    DISCONNECTED,   // Not connected to Tor
    CONNECTING,     // Attempting to connect
    CONNECTED,      // Successfully connected
    ERROR,          // Connection error
    UNKNOWN         // Status unknown
};

/**
 * @brief FuegoTor configuration structure
 */
struct FuegoTorConfig {
    bool enabled = false;                    // Enable FuegoTor integration
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
 * @brief FuegoTor connection information
 */
struct FuegoTorConnectionInfo {
    std::string address;                     // Connection address
    uint16_t port = 0;                      // Connection port
    std::string onionAddress;               // Onion address (if applicable)
    FuegoTorStatus status = FuegoTorStatus::UNKNOWN;  // Connection status
    uint32_t latency = 0;                  // Connection latency (ms)
    std::string errorMessage;              // Error message (if any)
};

/**
 * @brief FuegoTor statistics
 */
struct FuegoTorStats {
    uint32_t totalConnections = 0;          // Total connections made
    uint32_t successfulConnections = 0;     // Successful connections
    uint32_t failedConnections = 0;        // Failed connections
    uint32_t bytesTransferred = 0;          // Total bytes transferred
    uint32_t averageLatency = 0;            // Average connection latency
    uint32_t circuitCount = 0;              // Active circuit count
    std::string torVersion;                 // Tor version string
};

/**
 * @brief FuegoTor event callback types
 */
using FuegoTorStatusCallback = std::function<void(FuegoTorStatus status, const std::string& message)>;
using FuegoTorConnectionCallback = std::function<void(const FuegoTorConnectionInfo& info)>;
using FuegoTorErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Main FuegoTor integration manager
 */
class FuegoTorManager {
public:
    /**
     * @brief Constructor
     * @param config FuegoTor configuration
     */
    explicit FuegoTorManager(const FuegoTorConfig& config);
    
    /**
     * @brief Destructor
     */
    ~FuegoTorManager();
    
    /**
     * @brief Initialize FuegoTor integration
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown FuegoTor integration
     */
    void shutdown();
    
    /**
     * @brief Check if Tor is available and running
     * @return true if Tor is available
     */
    bool isTorAvailable() const;
    
    /**
     * @brief Get current FuegoTor status
     * @return Current status
     */
    FuegoTorStatus getStatus() const;
    
    /**
     * @brief Get FuegoTor statistics
     * @return Current statistics
     */
    FuegoTorStats getStats() const;
    
    /**
     * @brief Create a Tor connection
     * @param address Target address
     * @param port Target port
     * @return Connection info
     */
    FuegoTorConnectionInfo createConnection(const std::string& address, uint16_t port);
    
    /**
     * @brief Get hidden service address
     * @return Hidden service address (empty if not enabled)
     */
    std::string getHiddenServiceAddress() const;
    
    /**
     * @brief Set status callback
     * @param callback Status change callback
     */
    void setStatusCallback(FuegoTorStatusCallback callback);
    
    /**
     * @brief Set connection callback
     * @param callback Connection event callback
     */
    void setConnectionCallback(FuegoTorConnectionCallback callback);
    
    /**
     * @brief Set error callback
     * @param callback Error event callback
     */
    void setErrorCallback(FuegoTorErrorCallback callback);
    
    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if update successful
     */
    bool updateConfig(const FuegoTorConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    FuegoTorConfig getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Individual Tor connection
 */
class FuegoTorConnection {
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

};

} // namespace CryptoNote
