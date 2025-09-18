// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2014-2017 The XDN developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope
// that it will be useful, but WITHOUT ANY WARRANTY; without even
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "BurnProofDataFileGenerator.h"
#include "crypto/keccak.h"
#include "Common/StringTools.h"
#include "Common/JsonValue.h"
#include <fstream>
#include <sstream>
#include <chrono>

namespace CryptoNote {

std::error_code BurnProofDataFileGenerator::generateBPDF(
    const std::string& txHash,
    const Crypto::SecretKey& secret,
    const std::string& recipientAddress,
    uint64_t amount,
    const std::string& outputPath) {
    
    // Validate Arbitrum address
    if (!isValidArbitrumAddress(recipientAddress)) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    // Validate XFG amount (supports both 0.8 XFG and 800 XFG)
    if (!isValidXfgAmount(amount)) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    
    // Calculate cryptographic hashes (same as xfgwinter)
    Crypto::Hash nullifier = calculateNullifier(secret);
    Crypto::Hash commitment = calculateCommitment(secret, amount);
    Crypto::Hash recipientHash = calculateRecipientHash(recipientAddress);
    Crypto::Hash txExtraHash = calculateTxExtraHash(secret);
    
    // Calculate network validation hash
    std::string genesisTx = "013c01ff0001b4bcc29101029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd0880712101bd4e0bf284c04d004fd016a21405046e8267ef81328cabf3017c4c24b273b25a";
    // Fuego Network ID: 93385046440755750514194170694064996624
    Crypto::Hash networkValidationHash = calculateNetworkValidationHash(0, genesisTx); // TODO: Fix large integer
    
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    // Create JSON structure compatible with xfgwinter
    std::ostringstream json;
    json << "{\n";
    json << "  \"metadata\": {\n";
    json << "    \"version\": \"1.0\",\n";
    json << "    \"proof_type\": \"XFG_BURN\",\n";
    json << "    \"transaction_hash\": \"" << txHash << "\",\n";
    json << "    \"created_at\": " << timestamp << ",\n";
    json << "    \"format_version\": \"1.0\"\n";
    json << "  },\n";
    json << "  \"cryptographic_data\": {\n";
    json << "    \"secret\": \"0x" << Common::podToHex(secret) << "\",\n";
    json << "    \"nullifier\": \"0x" << Common::podToHex(nullifier) << "\",\n";
    json << "    \"commitment\": \"0x" << Common::podToHex(commitment) << "\",\n";
    json << "    \"block_height\": 0,\n";
    json << "    \"xfg_amount\": " << amount << ",\n";
    json << "    \"tx_extra_hash\": \"0x0000000000000000000000000000000000000000000000000000000000000000\"\n";
    json << "  },\n";
    json << "  \"user_data\": {\n";
    json << "    \"recipient_address\": \"" << recipientAddress << "\",\n";
    json << "    \"recipient_hash\": \"0x" << Common::podToHex(recipientHash) << "\",\n";
    json << "    \"heat_amount\": " << (amount * 10) << ",\n";
    json << "    \"xfg_amount_formatted\": \"" << (amount / 10000000.0) << " XFG\",\n";
    json << "    \"heat_amount_formatted\": \"" << (amount * 10) << " HEAT\",\n";
    json << "    \"transaction_timestamp\": " << timestamp << "\n";
    json << "  },\n";
    json << "  \"security\": {\n";
    json << "    \"signature\": \"\",\n";
    json << "    \"checksum\": \"\",\n";
    json << "    \"signature_pubkey\": \"\",\n";
    json << "    \"integrity_hash\": \"\",\n";
    json << "    \"genesis_validation\": {\n";
    json << "      \"genesis_transaction_hash\": \"0x013c01ff0001b4bcc29101029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd0880712101bd4e0bf284c04d004fd016a21405046e8267ef81328cabf3017c4c24b273b25a\",\n";
    json << "      \"genesis_block_hash\": \"0x0000000000000000000000000000000000000000000000000000000000000000\",\n";
    json << "      \"genesis_timestamp\": 0,\n";
    json << "      \"genesis_validation_hash\": \"0x0000000000000000000000000000000000000000000000000000000000000000\",\n";
    json << "      \"fuego_network_id\": 93385046440755750514194170694064996624,\n";
    json << "      \"network_validation_hash\": \"0x" << Common::podToHex(networkValidationHash) << "\"\n";
    json << "    }\n";
    json << "  }\n";
    json << "}\n";
    
    // Save to file
    return saveToFile(json.str(), outputPath);
}

std::error_code BurnProofDataFileGenerator::extractSecretFromTransaction(
    const std::string& txHash,
    Crypto::SecretKey& secret,
    uint64_t& amount) {
    
    // TODO: Implement transaction parsing to extract secret and amount
    // This would parse the transaction extra field to get the secret
    // For now, return error (placeholder implementation)
    return std::make_error_code(std::errc::not_supported);
}

bool BurnProofDataFileGenerator::validateBPDF(const std::string& filePath) {
    // TODO: Implement BPDF validation
    // This would validate the JSON structure and cryptographic hashes
    return false;
}

Crypto::Hash BurnProofDataFileGenerator::calculateNullifier(const Crypto::SecretKey& secret) {
    // Same algorithm as xfgwinter: keccak256(secret + "nullifier")
    std::vector<uint8_t> data;
    data.insert(data.end(), secret.data, secret.data + sizeof(secret.data));
    data.insert(data.end(), (uint8_t*)"nullifier", (uint8_t*)"nullifier" + 9);
    
    Crypto::Hash nullifier;
    keccak(data.data(), data.size(), nullifier.data, sizeof(nullifier.data));
    return nullifier;
}

Crypto::Hash BurnProofDataFileGenerator::calculateCommitment(const Crypto::SecretKey& secret, uint64_t amount) {
    // Match xfgwinter: keccak256(secret + "commitment")
    std::vector<uint8_t> data;
    data.insert(data.end(), secret.data, secret.data + sizeof(secret.data));
    data.insert(data.end(), (uint8_t*)"commitment", (uint8_t*)"commitment" + 10);
    
    Crypto::Hash commitment;
    keccak(data.data(), data.size(), commitment.data, sizeof(commitment.data));
    return commitment;
}

Crypto::Hash BurnProofDataFileGenerator::calculateRecipientHash(const std::string& recipientAddress) {
    // Same algorithm as xfgwinter: keccak256(recipient_address)
    std::vector<uint8_t> addressData(recipientAddress.begin(), recipientAddress.end());
    Crypto::Hash recipientHash;
    keccak(addressData.data(), addressData.size(), recipientHash.data, sizeof(recipientHash.data));
    return recipientHash;
}

Crypto::Hash BurnProofDataFileGenerator::calculateTxExtraHash(const Crypto::SecretKey& secret) {
    // Match xfgwinter: keccak256(secret) - just the secret
    Crypto::Hash txExtraHash;
    keccak(secret.data, sizeof(secret.data), txExtraHash.data, sizeof(txExtraHash.data));
    return txExtraHash;
}

Crypto::Hash BurnProofDataFileGenerator::calculateNetworkValidationHash(uint64_t networkId, const std::string& genesisTx) {
    // Match xfgwinter: keccak256(network_id + genesis_tx)
    std::vector<uint8_t> data;
    
    // Add network ID as string
    std::string networkIdStr = std::to_string(networkId);
    data.insert(data.end(), networkIdStr.begin(), networkIdStr.end());
    
    // Add genesis transaction hash
    data.insert(data.end(), genesisTx.begin(), genesisTx.end());
    
    Crypto::Hash networkHash;
    keccak(data.data(), data.size(), networkHash.data, sizeof(networkHash.data));
    return networkHash;
}

bool BurnProofDataFileGenerator::isValidArbitrumAddress(const std::string& address) {
    // Basic Arbitrum address validation
    if (address.length() != 42) return false;  // 0x + 40 hex chars
    if (address.substr(0, 2) != "0x") return false;
    
    // Check if all characters after 0x are hex
    for (size_t i = 2; i < address.length(); i++) {
        if (!isxdigit(address[i])) return false;
    }
    
    return true;
}

bool BurnProofDataFileGenerator::isValidXfgAmount(uint64_t amount) {
    // Validate XFG amount (supports both 0.8 XFG and 8000 XFG)
    switch (amount) {
        case 8000000:        // 0.8 XFG
        case 80000000000ULL: // 8000 XFG
            return true;
        default:
            return false;
    }
}

std::string BurnProofDataFileGenerator::generateFilename(const std::string& txHash) {
    // Generate filename from transaction hash
    std::string shortHash = txHash.substr(0, 8);
    return "burn_proof_" + shortHash + ".json";
}

std::error_code BurnProofDataFileGenerator::saveToFile(const std::string& jsonData, const std::string& outputPath) {
    try {
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            return std::make_error_code(std::errc::io_error);
        }
        
        file << jsonData;
        file.close();
        
        return std::error_code();
    } catch (const std::exception&) {
        return std::make_error_code(std::errc::io_error);
    }
}

} // namespace CryptoNote
