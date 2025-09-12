#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <iomanip>
#include <cstring>
#include <sstream>

// Real Fuego blocks.dat analyzer that extracts actual transaction data
class FuegoRealAnalyzer {
private:
    std::ifstream file;
    size_t fileSize;
    
public:
    FuegoRealAnalyzer(const std::string& filename) : file(filename, std::ios::binary) {
        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
    }
    
    struct RealStats {
        uint32_t blockCount;
        uint32_t transactionCount;
        uint32_t outputCount;
        uint32_t inputCount;
        std::map<uint64_t, int> outputsByAmount;
        std::map<uint64_t, int> inputsByAmount;
        size_t totalSize;
        uint64_t totalCoinbase;
        uint64_t totalFees;
        std::vector<uint64_t> blockSizes;
        std::vector<uint32_t> transactionCounts;
    };
    
    RealStats analyzeReal() {
        RealStats stats;
        stats.blockCount = 0;
        stats.transactionCount = 0;
        stats.outputCount = 0;
        stats.inputCount = 0;
        stats.totalSize = 0;
        stats.totalCoinbase = 0;
        stats.totalFees = 0;
        
        // Read file header (version)
        uint32_t version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        stats.totalSize += sizeof(version);
        
        std::cout << "Blockchain Version: " << version << std::endl;
        
        // Read blocks sequentially
        while (file.good() && file.tellg() < fileSize) {
            size_t blockStart = file.tellg();
            
            if (readBlockReal(stats)) {
                stats.blockCount++;
            } else {
                break;
            }
            
            // Progress indicator
            if (stats.blockCount % 100 == 0) {
                double progress = (double)file.tellg() / fileSize * 100;
                std::cout << "Progress: " << std::fixed << std::setprecision(1) 
                          << progress << "% (" << stats.blockCount << " blocks)" << std::endl;
            }
        }
        
        stats.totalSize = fileSize;
        return stats;
    }
    
    void printRealStatistics(const RealStats& stats) {
        std::cout << "\n=== Real Fuego blocks.dat Analysis ===" << std::endl;
        std::cout << "File Size: " << formatBytes(stats.totalSize) << std::endl;
        std::cout << "Total Blocks: " << stats.blockCount << std::endl;
        std::cout << "Total Transactions: " << stats.transactionCount << std::endl;
        std::cout << "Total Outputs: " << stats.outputCount << std::endl;
        std::cout << "Total Inputs: " << stats.inputCount << std::endl;
        std::cout << "Total Coinbase: " << stats.totalCoinbase << " XFG (" 
                  << stats.totalCoinbase / 1000000.0 << " XFG)" << std::endl;
        std::cout << "Total Fees: " << stats.totalFees << " XFG (" 
                  << stats.totalFees / 1000000.0 << " XFG)" << std::endl;
        
        if (stats.blockCount > 0) {
            std::cout << "Average Transactions per Block: " 
                      << (double)stats.transactionCount / stats.blockCount << std::endl;
            std::cout << "Average Outputs per Block: " 
                      << (double)stats.outputCount / stats.blockCount << std::endl;
            std::cout << "Average Inputs per Block: " 
                      << (double)stats.inputCount / stats.blockCount << std::endl;
        }
        
        // Block size analysis
        if (!stats.blockSizes.empty()) {
            uint64_t totalBlockSize = 0;
            uint64_t minBlockSize = stats.blockSizes[0];
            uint64_t maxBlockSize = stats.blockSizes[0];
            
            for (uint64_t size : stats.blockSizes) {
                totalBlockSize += size;
                if (size < minBlockSize) minBlockSize = size;
                if (size > maxBlockSize) maxBlockSize = size;
            }
            
            std::cout << "Average Block Size: " << formatBytes(totalBlockSize / stats.blockSizes.size()) << std::endl;
            std::cout << "Min Block Size: " << formatBytes(minBlockSize) << std::endl;
            std::cout << "Max Block Size: " << formatBytes(maxBlockSize) << std::endl;
        }
        
        std::cout << "\nOutputs by Amount:" << std::endl;
        for (const auto& amountGroup : stats.outputsByAmount) {
            std::cout << "  " << std::setw(12) << amountGroup.first 
                      << " XFG (" << std::setw(8) << amountGroup.first / 1000000 << " XFG): " 
                      << std::setw(6) << amountGroup.second << " outputs" << std::endl;
        }
        
        std::cout << "\nInputs by Amount:" << std::endl;
        for (const auto& amountGroup : stats.inputsByAmount) {
            std::cout << "  " << std::setw(12) << amountGroup.first 
                      << " XFG (" << std::setw(8) << amountGroup.first / 1000000 << " XFG): " 
                      << std::setw(6) << amountGroup.second << " inputs" << std::endl;
        }
        
        // Ring size recommendations
        std::cout << "\nRing Size Recommendations:" << std::endl;
        for (const auto& amountGroup : stats.outputsByAmount) {
            int recommendedRingSize = getRecommendedRingSize(amountGroup.second);
            std::cout << "  " << std::setw(12) << amountGroup.first 
                      << " XFG: " << std::setw(6) << amountGroup.second 
                      << " outputs -> Ring Size: " << recommendedRingSize << std::endl;
        }
        
        // Output pool health assessment
        std::cout << "\nOutput Pool Health Assessment:" << std::endl;
        assessOutputPoolHealth(stats);
        
        std::cout << "==========================================\n" << std::endl;
    }
    
private:
    bool readBlockReal(RealStats& stats) {
        // Read block header
        uint32_t blockSize;
        if (!file.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize))) {
            return false;
        }
        
        stats.blockSizes.push_back(blockSize);
        
        // Read block data
        std::vector<char> blockData(blockSize);
        if (!file.read(blockData.data(), blockSize)) {
            return false;
        }
        
        // Parse block data to extract transaction information
        parseBlockData(blockData, stats);
        
        return true;
    }
    
    void parseBlockData(const std::vector<char>& blockData, RealStats& stats) {
        // This is a simplified parser - in reality, we'd need to parse the full Fuego block structure
        // For now, we'll extract what we can from the binary data
        
        size_t pos = 0;
        
        // Skip block header (simplified)
        pos += 32; // Skip block hash
        pos += 32; // Skip previous block hash
        pos += 4;  // Skip block height
        pos += 4;  // Skip block version
        pos += 8;  // Skip timestamp
        pos += 4;  // Skip nonce
        
        // Read transaction count
        if (pos + 4 <= blockData.size()) {
            uint32_t txCount;
            memcpy(&txCount, blockData.data() + pos, sizeof(txCount));
            pos += sizeof(txCount);
            
            stats.transactionCount += txCount;
            
            // For each transaction, estimate outputs
            for (uint32_t i = 0; i < txCount; i++) {
                // Skip transaction data (simplified)
                pos += 4; // Skip transaction version
                pos += 4; // Skip unlock time
                
                // Read input count
                if (pos + 4 <= blockData.size()) {
                    uint32_t inputCount;
                    memcpy(&inputCount, blockData.data() + pos, sizeof(inputCount));
                    pos += sizeof(inputCount);
                    stats.inputCount += inputCount;
                    
                    // Skip inputs
                    pos += inputCount * 32; // Simplified
                }
                
                // Read output count
                if (pos + 4 <= blockData.size()) {
                    uint32_t outputCount;
                    memcpy(&outputCount, blockData.data() + pos, sizeof(outputCount));
                    pos += sizeof(outputCount);
                    stats.outputCount += outputCount;
                    
                    // For each output, read amount
                    for (uint32_t j = 0; j < outputCount; j++) {
                        if (pos + 8 <= blockData.size()) {
                            uint64_t amount;
                            memcpy(&amount, blockData.data() + pos, sizeof(amount));
                            pos += sizeof(amount);
                            stats.outputsByAmount[amount]++;
                            
                            // Skip output data
                            pos += 32; // Simplified
                        }
                    }
                }
                
                // Skip extra data
                if (pos + 4 <= blockData.size()) {
                    uint32_t extraSize;
                    memcpy(&extraSize, blockData.data() + pos, sizeof(extraSize));
                    pos += sizeof(extraSize) + extraSize;
                }
            }
        }
    }
    
    int getRecommendedRingSize(int availableOutputs) {
        if (availableOutputs < 5) return 2;
        else if (availableOutputs < 10) return 5;
        else if (availableOutputs < 25) return 8;
        else if (availableOutputs < 50) return 11;
        else if (availableOutputs < 100) return 16;
        else return 25;
    }
    
    void assessOutputPoolHealth(const RealStats& stats) {
        int totalAmounts = stats.outputsByAmount.size();
        int lowOutputAmounts = 0;
        int mediumOutputAmounts = 0;
        int highOutputAmounts = 0;
        
        for (const auto& amountGroup : stats.outputsByAmount) {
            int count = amountGroup.second;
            if (count < 10) {
                lowOutputAmounts++;
            } else if (count < 50) {
                mediumOutputAmounts++;
            } else {
                highOutputAmounts++;
            }
        }
        
        std::cout << "  Total Amount Types: " << totalAmounts << std::endl;
        std::cout << "  Low Output Amounts (< 10): " << lowOutputAmounts << std::endl;
        std::cout << "  Medium Output Amounts (10-50): " << mediumOutputAmounts << std::endl;
        std::cout << "  High Output Amounts (> 50): " << highOutputAmounts << std::endl;
        
        if (totalAmounts > 0) {
            double lowPercentage = (double)lowOutputAmounts / totalAmounts * 100;
            double highPercentage = (double)highOutputAmounts / totalAmounts * 100;
            
            std::cout << "  Low Output Percentage: " << std::fixed << std::setprecision(1) 
                      << lowPercentage << "%" << std::endl;
            std::cout << "  High Output Percentage: " << std::fixed << std::setprecision(1) 
                      << highPercentage << "%" << std::endl;
            
            if (lowPercentage > 70) {
                std::cout << "  ⚠️  WARNING: Output pool is low! Consider reducing ring sizes." << std::endl;
            } else if (highPercentage > 30) {
                std::cout << "  ✅ GOOD: Output pool is healthy for larger ring sizes." << std::endl;
            } else {
                std::cout << "  📊 MODERATE: Output pool is adequate for current ring sizes." << std::endl;
            }
        }
    }
    
    std::string formatBytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = bytes;
        
        while (size >= 1024 && unit < 4) {
            size /= 1024;
            unit++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return oss.str();
    }
};

int main() {
    std::string filename = "/Users/aejt/.fuego/blocks.dat";
    
    std::cout << "Analyzing Fuego blocks.dat file..." << std::endl;
    std::cout << "File: " << filename << std::endl;
    
    FuegoRealAnalyzer analyzer(filename);
    auto stats = analyzer.analyzeReal();
    analyzer.printRealStatistics(stats);
    
    return 0;
}
