// Copyright (c) 2017-2026 Fuego Developers
// Copyright (c) 2025 Elderfire Privacy Council
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
// You should receive a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "BurnProofDataFileGenerator.h"
#include "BurnProofDataFileGenerator.h"
#include "Common/JsonValue.h"
#include "crypto/keccak.h"
#include "Common/StringTools.h"
#include <iomanip>

#include <fstream>
#include <iterator>
#include <iomanip>
#include <sstream>

using namespace CryptoNote;
using namespace Common;

std::error_code BurnProofDataFileGenerator::generateBPDF(
    const std::string& txHash,
    const Crypto::SecretKey& secret,
    const std::string& recipientAddress,
    uint64_t amount,
    const std::string& outputPath) {
    
    try {
        // Validate inputs
        if (!isValidArbitrumAddress(recipientAddress)) {
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (!isValidXfgAmount(amount)) {
            return std::make_error_code(std::errc::invalid_argument);
        }
        
        // Generate cryptographic data
        Crypto::Hash nullifier = calculateNullifier(secret);
        Crypto::Hash commitment = calculateCommitment(secret, amount);
        Crypto::Hash recipientHash = calculateRecipientHash(recipientAddress);
        Crypto::Hash txExtraHash = calculateTxExtraHash(secret);
        
        // Build JSON structure
        JsonValue root(JsonValue::OBJECT);
        
        // Metadata
        JsonValue metadata(JsonValue::OBJECT);
        metadata.set("version", "1.0");
        metadata.set("proof_type", "burn_proof");
        metadata.set("transaction_hash", "0x" + txHash);
        metadata.set("created_at", static_cast<int64_t>(time(nullptr)));
        metadata.set("format_version", "xfgwinter-1.0");
        root.set("metadata", metadata);
        
        // Cryptographic data
        JsonValue cryptoData(JsonValue::OBJECT);
        cryptoData.set("secret", "0x" + Common::podToHex(secret));
        cryptoData.set("nullifier", "0x" + Common::podToHex(nullifier));
        cryptoData.set("commitment", "0x" + Common::podToHex(commitment));
        cryptoData.set("block_height", JsonValue(static_cast<int64_t>(0))); // Placeholder
        cryptoData.set("xfg_amount", static_cast<int64_t>(amount));
        cryptoData.set("tx_extra_hash", "0x" + Common::podToHex(txExtraHash));
        root.set("cryptographic_data", cryptoData);
        
        // User data
        JsonValue userData(JsonValue::OBJECT);
        userData.set("recipient_address", recipientAddress);
        userData.set("recipient_hash", "0x" + Common::podToHex(recipientHash));
        userData.set("heat_amount", static_cast<int64_t>(0)); // Placeholder
        userData.set("xfg_amount_formatted", formatAmount(amount));
        userData.set("heat_amount_formatted", "0");
        userData.set("transaction_timestamp", static_cast<int64_t>(time(nullptr)));
        root.set("user_data", userData);
        
        // Security data (placeholders)
        JsonValue security(JsonValue::OBJECT);
        security.set("signature", "0x0000000000000000000000000000000000000000000000000000000000000000");
        security.set("checksum", "0x0000000000000000000000000000000000000000000000000000000000000000");
        security.set("signature_pubkey", "0x0000000000000000000000000000000000000000000000000000000000000000");
        security.set("integrity_hash", "0x0000000000000000000000000000000000000000000000000000000000000000");
        
        JsonValue genesisValidation(JsonValue::OBJECT);
        genesisValidation.set("genesis_transaction_hash", "0x0000000000000000000000000000000000000000000000000000000000000000");
        genesisValidation.set("genesis_block_hash", "0x0000000000000000000000000000000000000000000000000000000000000000");
        genesisValidation.set("genesis_timestamp", static_cast<int64_t>(0));
        genesisValidation.set("genesis_validation_hash", "0x0000000000000000000000000000000000000000000000000000000000000000");
        genesisValidation.set("fuego_network_id", static_cast<int64_t>(0xBEEF)); // Placeholder network ID
        genesisValidation.set("network_validation_hash", "0x0000000000000000000000000000000000000000000000000000000000000000");
        security.set("genesis_validation", genesisValidation);
        root.set("security", security);
        
        // Save to file
        return saveToFile(root.toString(), outputPath);
        
    } catch (const std::exception&) {
        return std::make_error_code(std::errc::io_error);
    }
}

std::error_code BurnProofDataFileGenerator::extractSecretFromTransaction(
    const std::string& txHash,
    Crypto::SecretKey& secret,
    uint64_t& amount) {
    
    // TODO: Implement transaction parsing to extract secret from tx extra
    // Placeholder implementation
    return std::make_error_code(std::errc::not_supported);
}

bool BurnProofDataFileGenerator::validateBPDF(const std::string& filePath) {
    try {
        // Read the BPDF file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // Parse JSON content
        JsonValue json = JsonValue::fromString(jsonContent);
        if (json.getType() != JsonValue::OBJECT) {
            return false;
        }
        
        // Validate JSON structure
        if (!validateJsonStructure(json)) {
            return false;
        }
        
        // Extract data for validation
        BPDFData data;
        if (!extractBPDFData(json, data)) {
            return false;
        }
        
        // Validate cryptographic hashes
        if (!validateCryptographicHashes(data)) {
            return false;
        }
        
        // Validate data integrity
        if (!validateDataIntegrity(data)) {
            return false;
        }
        
        // Validate format and constraints
        if (!validateFormatConstraints(data)) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
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
    // Validate XFG amount (supports both 0.8 XFG and 800 XFG)
    switch (amount) {
        case 8000000ULL:        // 0.8 XFG
        case 800000000000ULL:   // 800 XFG
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

// BPDF Validation Helper Functions

bool BurnProofDataFileGenerator::validateJsonStructure(const JsonValue& json) {
    // Check if root is an object
    if (json.getType() != JsonValue::OBJECT) {
        return false;
    }
    
    // Validate required top-level sections
    const std::vector<std::string> requiredSections = {
        "metadata", "cryptographic_data", "user_data", "security"
    };
    
    for (const auto& section : requiredSections) {
        if (!json.contains(section) || json(section).getType() != JsonValue::OBJECT) {
            return false;
        }
    }
    
    // Validate metadata section
    const JsonValue& metadata = json("metadata");
    const std::vector<std::string> metadataFields = {
        "version", "proof_type", "transaction_hash", "created_at", "format_version"
    };
    
    for (const auto& field : metadataFields) {
        if (!metadata.contains(field)) {
            return false;
        }
    }
    
    // Validate cryptographic_data section
    const JsonValue& cryptoData = json("cryptographic_data");
    const std::vector<std::string> cryptoFields = {
        "secret", "nullifier", "commitment", "block_height", "xfg_amount", "tx_extra_hash"
    };
    
    for (const auto& field : cryptoFields) {
        if (!cryptoData.contains(field)) {
            return false;
        }
    }
    
    // Validate user_data section
    const JsonValue& userData = json("user_data");
    const std::vector<std::string> userFields = {
        "recipient_address", "recipient_hash", "heat_amount", 
        "xfg_amount_formatted", "heat_amount_formatted", "transaction_timestamp"
    };
    
    for (const auto& field : userFields) {
        if (!userData.contains(field)) {
            return false;
        }
    }
    
    // Validate security section
    const JsonValue& security = json("security");
    if (!security.contains("genesis_validation") || security("genesis_validation").getType() != JsonValue::OBJECT) {
        return false;
    }
    
    const JsonValue& genesisValidation = security("genesis_validation");
    const std::vector<std::string> genesisFields = {
        "genesis_transaction_hash", "genesis_block_hash", "genesis_timestamp",
        "genesis_validation_hash", "fuego_network_id", "network_validation_hash"
    };
    
    for (const auto& field : genesisFields) {
        if (!genesisValidation.contains(field)) {
            return false;
        }
    }
    
    return true;
}

bool BurnProofDataFileGenerator::extractBPDFData(const JsonValue& json, BPDFData& data) {
    try {
        // Extract metadata
        const JsonValue& metadata = json("metadata");
        data.version = metadata("version").getString();
        data.proofType = metadata("proof_type").getString();
        data.transactionHash = metadata("transaction_hash").getString();
        data.createdAt = static_cast<uint64_t>(metadata("created_at").getInteger());
        data.formatVersion = metadata("format_version").getString();
        
        // Extract cryptographic data
        const JsonValue& cryptoData = json("cryptographic_data");
        data.secret = cryptoData("secret").getString();
        data.nullifier = cryptoData("nullifier").getString();
        data.commitment = cryptoData("commitment").getString();
        data.blockHeight = static_cast<uint32_t>(cryptoData("block_height").getInteger());
        data.xfgAmount = static_cast<uint64_t>(cryptoData("xfg_amount").getInteger());
        data.txExtraHash = cryptoData("tx_extra_hash").getString();
        
        // Extract user data
        const JsonValue& userData = json("user_data");
        data.recipientAddress = userData("recipient_address").getString();
        data.recipientHash = userData("recipient_hash").getString();
        data.heatAmount = static_cast<uint64_t>(userData("heat_amount").getInteger());
        data.xfgAmountFormatted = userData("xfg_amount_formatted").getString();
        data.heatAmountFormatted = userData("heat_amount_formatted").getString();
        data.transactionTimestamp = static_cast<uint64_t>(userData("transaction_timestamp").getInteger());
        
        // Extract security data
        const JsonValue& security = json("security");
        data.signature = security("signature").getString();
        data.checksum = security("checksum").getString();
        data.signaturePubkey = security("signature_pubkey").getString();
        data.integrityHash = security("integrity_hash").getString();
        
        // Extract genesis validation data
        const JsonValue& genesisValidation = security("genesis_validation");
        data.genesisTransactionHash = genesisValidation("genesis_transaction_hash").getString();
        data.genesisBlockHash = genesisValidation("genesis_block_hash").getString();
        data.genesisTimestamp = static_cast<uint64_t>(genesisValidation("genesis_timestamp").getInteger());
        data.genesisValidationHash = genesisValidation("genesis_validation_hash").getString();
        data.fuegoNetworkId = static_cast<uint64_t>(genesisValidation("fuego_network_id").getInteger());
        data.networkValidationHash = genesisValidation("network_validation_hash").getString();
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool BurnProofDataFileGenerator::validateCryptographicHashes(const BPDFData& data) {
    // Validate secret format (should be 0x + 64 hex chars)
    if (!isValidHexString(data.secret, 66)) { // 0x + 64 chars
        return false;
    }
    
    // Extract secret key
    std::string secretHex = data.secret.substr(2); // Remove 0x prefix
    Crypto::SecretKey secret;
    if (!Common::fromHex(secretHex, secret.data, sizeof(secret.data))) {
        return false;
    }
    
    // Validate nullifier calculation
    Crypto::Hash expectedNullifier = calculateNullifier(secret);
    std::string expectedNullifierHex = "0x" + Common::podToHex(expectedNullifier);
    if (data.nullifier != expectedNullifierHex) {
        return false;
    }
    
    // Validate commitment calculation
    Crypto::Hash expectedCommitment = calculateCommitment(secret, data.xfgAmount);
    std::string expectedCommitmentHex = "0x" + Common::podToHex(expectedCommitment);
    if (data.commitment != expectedCommitmentHex) {
        return false;
    }
    
    return true;
}

bool BurnProofDataFileGenerator::validateDataIntegrity(const BPDFData& data) {
    // Basic integrity checks
    if (data.version.empty() || data.formatVersion.empty()) {
        return false;
    }
    
    if (!isValidHexString(data.nullifier, 66) || 
        !isValidHexString(data.commitment, 66) ||
        !isValidHexString(data.txExtraHash, 66)) {
        return false;
    }
    
    return true;
}

bool BurnProofDataFileGenerator::validateFormatConstraints(const BPDFData& data) {
    // Validate XFG amount
    if (!isValidXfgAmount(data.xfgAmount)) {
        return false;
    }
    
    // Validate recipient address format
    if (!isValidArbitrumAddress(data.recipientAddress)) {
        return false;
    }
    
    return true;
}

bool BurnProofDataFileGenerator::isValidHexString(const std::string& str, size_t expectedLength) {
    if (str.length() != expectedLength) return false;
    if (str.substr(0, 2) != "0x") return false;
    
    for (size_t i = 2; i < str.length(); i++) {
        if (!isxdigit(str[i])) return false;
    }
    
    return true;
}

std::string BurnProofDataFileGenerator::formatAmount(uint64_t amount) {
    double xfg = amount / 10000000.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << xfg << " XFG";
    return oss.str();
}