#include "TorIntegration.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace CryptoNote;

int main() {
    std::cout << "FuegoTor Example" << std::endl;
    std::cout << "================" << std::endl;
    
    // Check if Tor is installed
    if (!TorUtils::isTorInstalled()) {
        std::cout << "Error: Tor is not installed on this system" << std::endl;
        std::cout << "Please install Tor and try again" << std::endl;
        return 1;
    }
    
    std::cout << "Tor version: " << TorUtils::getTorVersion() << std::endl;
    
    // Load configuration
    TorConfig config = TorUtils::getDefaultConfig();
    config.enabled = true;
    config.socksHost = "127.0.0.1";
    config.socksPort = 9050;
    config.controlHost = "127.0.0.1";
    config.controlPort = 9051;
    config.autoStart = false;
    config.enableHiddenService = false;
    
    // Create Tor manager
    TorManager torManager(config);
    
    // Set up callbacks
    torManager.setStatusCallback([](TorStatus status, const std::string& message) {
        std::cout << "Tor Status: " << static_cast<int>(status) << " - " << message << std::endl;
    });
    
    torManager.setConnectionCallback([](const TorConnectionInfo& info) {
        std::cout << "Connection: " << info.address << ":" << info.port 
                  << " Status: " << static_cast<int>(info.status) << std::endl;
    });
    
    torManager.setErrorCallback([](const std::string& error) {
        std::cout << "Tor Error: " << error << std::endl;
    });
    
    // Initialize Tor
    std::cout << "Initializing Tor..." << std::endl;
    if (!torManager.initialize()) {
        std::cout << "Failed to initialize Tor" << std::endl;
        return 1;
    }
    
    // Wait for connection
    std::cout << "Waiting for Tor connection..." << std::endl;
    int attempts = 0;
    while (torManager.getStatus() != TorStatus::CONNECTED && attempts < 30) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        attempts++;
        std::cout << "Attempt " << attempts << "/30..." << std::endl;
    }
    
    if (torManager.getStatus() != TorStatus::CONNECTED) {
        std::cout << "Failed to connect to Tor after 30 seconds" << std::endl;
        return 1;
    }
    
    std::cout << "Successfully connected to Tor!" << std::endl;
    
    // Test connection
    std::cout << "Testing Tor connection..." << std::endl;
    TorConnectionInfo info = torManager.createConnection("example.com", 80);
    
    if (info.status == TorStatus::CONNECTED) {
        std::cout << "Successfully created Tor connection to " << info.address << ":" << info.port << std::endl;
    } else {
        std::cout << "Failed to create Tor connection: " << info.errorMessage << std::endl;
    }
    
    // Get statistics
    TorStats stats = torManager.getStats();
    std::cout << "Tor Statistics:" << std::endl;
    std::cout << "  Total Connections: " << stats.totalConnections << std::endl;
    std::cout << "  Successful Connections: " << stats.successfulConnections << std::endl;
    std::cout << "  Failed Connections: " << stats.failedConnections << std::endl;
    std::cout << "  Bytes Transferred: " << stats.bytesTransferred << std::endl;
    std::cout << "  Average Latency: " << stats.averageLatency << " ms" << std::endl;
    std::cout << "  Circuit Count: " << stats.circuitCount << std::endl;
    std::cout << "  Tor Version: " << stats.torVersion << std::endl;
    
    // Test onion address generation
    std::cout << "Testing onion address generation..." << std::endl;
    std::string onionAddress = TorUtils::generateOnionAddress();
    std::cout << "Generated onion address: " << onionAddress << std::endl;
    
    if (TorUtils::isValidOnionAddress(onionAddress)) {
        std::cout << "Onion address is valid" << std::endl;
    } else {
        std::cout << "Onion address is invalid" << std::endl;
    }
    
    // Test configuration management
    std::cout << "Testing configuration management..." << std::endl;
    TorConfig currentConfig = torManager.getConfig();
    std::cout << "Current configuration:" << std::endl;
    std::cout << "  Enabled: " << (currentConfig.enabled ? "true" : "false") << std::endl;
    std::cout << "  SOCKS Host: " << currentConfig.socksHost << std::endl;
    std::cout << "  SOCKS Port: " << currentConfig.socksPort << std::endl;
    std::cout << "  Control Host: " << currentConfig.controlHost << std::endl;
    std::cout << "  Control Port: " << currentConfig.controlPort << std::endl;
    std::cout << "  Auto Start: " << (currentConfig.autoStart ? "true" : "false") << std::endl;
    std::cout << "  Enable Hidden Service: " << (currentConfig.enableHiddenService ? "true" : "false") << std::endl;
    
    // Test configuration update
    currentConfig.connectionTimeout = 60000;
    if (torManager.updateConfig(currentConfig)) {
        std::cout << "Successfully updated configuration" << std::endl;
    } else {
        std::cout << "Failed to update configuration" << std::endl;
    }
    
    // Test hidden service
    if (currentConfig.enableHiddenService) {
        std::string hiddenServiceAddress = torManager.getHiddenServiceAddress();
        if (!hiddenServiceAddress.empty()) {
            std::cout << "Hidden service address: " << hiddenServiceAddress << std::endl;
        } else {
            std::cout << "No hidden service address available" << std::endl;
        }
    }
    
    // Keep running for a bit to test stability
    std::cout << "Running for 10 seconds to test stability..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // Shutdown
    std::cout << "Shutting down Tor..." << std::endl;
    torManager.shutdown();
    
    std::cout << "FuegoTor example completed successfully!" << std::endl;
    return 0;
}
