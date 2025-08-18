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

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <system_error>
#include "CryptoNote.h"

namespace CryptoNote {

// Burn Proof Data File Generator
// Generates JSON files compatible with xfgwinter proof generation
class BurnProofDataFileGenerator {
public:
    // Generate burn proof data file (BPDF)
    static std::error_code generateBPDF(
        const std::string& txHash,
        const Crypto::SecretKey& secret,
        const std::string& recipientAddress,
        uint64_t amount,
        const std::string& outputPath);
    
    // Extract secret from transaction
    static std::error_code extractSecretFromTransaction(
        const std::string& txHash,
        Crypto::SecretKey& secret,
        uint64_t& amount);
    
    // Validate burn proof data file
    static bool validateBPDF(const std::string& filePath);
    
    // Calculate nullifier from secret (same as xfgwinter)
    static Crypto::Hash calculateNullifier(const Crypto::SecretKey& secret);
    
    // Calculate commitment from secret and amount (pure, no recipient)
    static Crypto::Hash calculateCommitment(const Crypto::SecretKey& secret, uint64_t amount);
    
    // Calculate recipient hash from address
    static Crypto::Hash calculateRecipientHash(const std::string& recipientAddress);
    
    // Calculate transaction extra hash (just the secret)
    static Crypto::Hash calculateTxExtraHash(const Crypto::SecretKey& secret);
    
    // Calculate network validation hash
    static Crypto::Hash calculateNetworkValidationHash(uint64_t networkId, const std::string& genesisTx);
    
    // Validate Arbitrum address format
    static bool isValidArbitrumAddress(const std::string& address);
    
    // Validate XFG amount (supports both 0.8 XFG and 8000 XFG)
    static bool isValidXfgAmount(uint64_t amount);

private:
    // Generate filename from transaction hash
    static std::string generateFilename(const std::string& txHash);
    
    // Save JSON to file
    static std::error_code saveToFile(const std::string& jsonData, const std::string& outputPath);
};

} // namespace CryptoNote
