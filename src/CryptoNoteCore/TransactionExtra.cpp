// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license

#include "TransactionExtra.h"
#include "CryptoNoteTools.h"
#include "crypto/hash.h"
#include "crypto/chacha8.h"
#include "Common/StringTools.h"
#include "Common/Base58.h"
#include "Serialization/BinaryOutputStreamSerializer.h"
#include "Serialization/BinaryInputStreamSerializer.h"
#include <json/json.h>

namespace CryptoNote {

// Burn Proof Implementation
bool TransactionExtraBurnProof::serialize(ISerializer& serializer) {
    serializer(proof_pubkey, "proof_pubkey");
    serializer(encrypted_data, "encrypted_data");
    serializer(nonce, "nonce");
    serializer(timestamp, "timestamp");
    serializer(proof_type, "proof_type");
    return true;
}

// Deposit Proof Implementation  
bool TransactionExtraDepositProof::serialize(ISerializer& serializer) {
    serializer(proof_pubkey, "proof_pubkey");
    serializer(encrypted_data, "encrypted_data");
    serializer(nonce, "nonce");
    serializer(timestamp, "timestamp");
    serializer(proof_type, "proof_type");
    return true;
}

// Proof Verification Data Implementation
void ProofVerificationData::serialize(Json::Value& json) const {
    json["tx_hash"] = tx_hash;
    json["amount"] = static_cast<Json::UInt64>(amount);
    json["address"] = address;
    json["timestamp"] = static_cast<Json::UInt64>(timestamp);
    json["proof_type"] = proof_type;
}

void ProofVerificationData::deserialize(const Json::Value& json) {
    if (json.isMember("tx_hash")) tx_hash = json["tx_hash"].asString();
    if (json.isMember("amount")) amount = json["amount"].asUInt64();
    if (json.isMember("address")) address = json["address"].asString();
    if (json.isMember("timestamp")) timestamp = json["timestamp"].asUInt64();
    if (json.isMember("proof_type")) proof_type = json["proof_type"].asString();
}

// Generate Burn Proof with User Secret
bool generateBurnProof(const std::string& user_secret, uint64_t amount, const std::string& address, uint64_t timestamp, TransactionExtraBurnProof& proof) {
    try {
        // Derive public key from user secret
        Crypto::Hash secret_hash;
        cn_fast_hash(user_secret.data(), user_secret.size(), secret_hash);
        
        // Use hash as private key to derive public key
        Crypto::SecretKey private_key;
        memcpy(&private_key, secret_hash.data(), sizeof(Crypto::SecretKey));
        
        proof.proof_pubkey = Crypto::secret_key_to_public_key(private_key);
        proof.timestamp = timestamp;
        proof.proof_type = "BURN_PROOF";
        
        // Create verification data
        ProofVerificationData verif_data;
        verif_data.amount = amount;
        verif_data.address = address;
        verif_data.timestamp = timestamp;
        verif_data.proof_type = "BURN_PROOF";
        
        // Serialize verification data to JSON
        Json::Value json_data;
        verif_data.serialize(json_data);
        std::string json_str = json_data.toStyledString();
        
        // Generate encryption key from user secret
        Crypto::Hash key_hash;
        cn_fast_hash(user_secret.data(), user_secret.size(), key_hash);
        
        // Generate random nonce
        proof.nonce.resize(12); // 96-bit nonce for ChaCha20
        memcpy(proof.nonce.data(), secret_hash.data(), 12);
        
        // Encrypt the verification data
        proof.encrypted_data.resize(json_str.size());
        chacha8(json_str.data(), json_str.size(), key_hash, proof.nonce, proof.encrypted_data.data());
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Generate Deposit Proof with User Secret
bool generateDepositProof(const std::string& user_secret, uint64_t amount, const std::string& address, 
                         uint32_t term_months, const std::string& deposit_type, uint64_t timestamp, 
                         TransactionExtraDepositProof& proof) {
    try {
        // Derive public key from user secret
        Crypto::Hash secret_hash;
        cn_fast_hash(user_secret.data(), user_secret.size(), secret_hash);
        
        // Use hash as private key to derive public key
        Crypto::SecretKey private_key;
        memcpy(&private_key, secret_hash.data(), sizeof(Crypto::SecretKey));
        
        proof.proof_pubkey = Crypto::secret_key_to_public_key(private_key);
        proof.timestamp = timestamp;
        proof.proof_type = "DEPOSIT_PROOF";
        
        // Create verification data
        ProofVerificationData verif_data;
        verif_data.amount = amount;
        verif_data.address = address;
        verif_data.timestamp = timestamp;
        verif_data.proof_type = "DEPOSIT_PROOF";
        
        // Serialize verification data to JSON
        Json::Value json_data;
        verif_data.serialize(json_data);
        std::string json_str = json_data.toStyledString();
        
        // Generate encryption key from user secret
        Crypto::Hash key_hash;
        cn_fast_hash(user_secret.data(), user_secret.size(), key_hash);
        
        // Generate random nonce
        proof.nonce.resize(12); // 96-bit nonce for ChaCha20
        memcpy(proof.nonce.data(), secret_hash.data(), 12);
        
        // Encrypt the verification data
        proof.encrypted_data.resize(json_str.size());
        chacha8(json_str.data(), json_str.size(), key_hash, proof.nonce, proof.encrypted_data.data());
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Verify Burn Proof with User Secret
bool verifyBurnProofWithSecret(const std::string& user_secret, const TransactionExtraBurnProof& proof, 
                              const std::string& tx_hash, uint64_t amount, const std::string& address) {
    try {
        // Decrypt the proof data
        std::string decrypted_message;
        if (!decryptProofData(user_secret, proof.encrypted_data, proof.nonce, decrypted_message)) {
            return false;
        }
        
        // Parse decrypted JSON
        Json::Value json_data;
        Json::Reader reader;
        if (!reader.parse(decrypted_message, json_data)) {
            return false;
        }
        
        // Verify the data matches
        ProofVerificationData verif_data;
        verif_data.deserialize(json_data);
        
        return (verif_data.amount == amount && 
                verif_data.address == address &&
                verif_data.proof_type == "BURN_PROOF");
    } catch (const std::exception& e) {
        return false;
    }
}

// Verify Deposit Proof with User Secret
bool verifyDepositProofWithSecret(const std::string& user_secret, const TransactionExtraDepositProof& proof, 
                                 const std::string& tx_hash, uint64_t amount, const std::string& address) {
    try {
        // Decrypt the proof data
        std::string decrypted_message;
        if (!decryptProofData(user_secret, proof.encrypted_data, proof.nonce, decrypted_message)) {
            return false;
        }
        
        // Parse decrypted JSON
        Json::Value json_data;
        Json::Reader reader;
        if (!reader.parse(decrypted_message, json_data)) {
            return false;
        }
        
        // Verify the data matches
        ProofVerificationData verif_data;
        verif_data.deserialize(json_data);
        
        return (verif_data.amount == amount && 
                verif_data.address == address &&
                verif_data.proof_type == "DEPOSIT_PROOF");
    } catch (const std::exception& e) {
        return false;
    }
}

// Decrypt Proof Data
bool decryptProofData(const std::string& user_secret, const std::vector<uint8_t>& encrypted_data, 
                     const std::vector<uint8_t>& nonce, std::string& decrypted_message) {
    try {
        // Generate encryption key from user secret
        Crypto::Hash key_hash;
        cn_fast_hash(user_secret.data(), user_secret.size(), key_hash);
        
        // Decrypt the data
        decrypted_message.resize(encrypted_data.size());
        chacha8(encrypted_data.data(), encrypted_data.size(), key_hash, nonce, &decrypted_message[0]);
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Add Burn Receipt to Transaction Extra
bool addBurnReceiptToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraBurnReceipt& receipt) {
    try {
        BinaryArray ba;
        BinaryOutputStreamSerializer serializer(ba);
        serializer(receipt, "burn_receipt");
        
        tx_extra.push_back(TX_EXTRA_BURN_RECEIPT);
        tx_extra.push_back(static_cast<uint8_t>(ba.size()));
        tx_extra.insert(tx_extra.end(), ba.begin(), ba.end());
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Get Burn Receipt from Transaction Extra
bool getBurnReceiptFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraBurnReceipt& receipt) {
    std::vector<TransactionExtraField> fields;
    if (!parseTransactionExtra(tx_extra, fields)) {
        return false;
    }
    
    for (const auto& field : fields) {
        if (field.type() == typeid(TransactionExtraBurnReceipt)) {
            receipt = boost::get<TransactionExtraBurnReceipt>(field);
            return true;
        }
    }
    return false;
}

// Add Deposit Receipt to Transaction Extra
bool addDepositReceiptToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraDepositReceipt& receipt) {
    try {
        BinaryArray ba;
        BinaryOutputStreamSerializer serializer(ba);
        serializer(receipt, "deposit_receipt");
        
        tx_extra.push_back(TX_EXTRA_DEPOSIT_RECEIPT);
        tx_extra.push_back(static_cast<uint8_t>(ba.size()));
        tx_extra.insert(tx_extra.end(), ba.begin(), ba.end());
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// Get Deposit Receipt from Transaction Extra
bool getDepositReceiptFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraDepositReceipt& receipt) {
    std::vector<TransactionExtraField> fields;
    if (!parseTransactionExtra(tx_extra, fields)) {
        return false;
    }
    
    for (const auto& field : fields) {
        if (field.type() == typeid(TransactionExtraDepositReceipt)) {
            receipt = boost::get<TransactionExtraDepositReceipt>(field);
            return true;
        }
    }
    return false;
}

}
```

### 4. **Enhanced Verification Endpoints**

Now let me update the verification endpoints to use the receipt system:

<tool_call>
<function=edit_file>
<parameter=display_description>
Update verification endpoints to use receipt-based proof
</parameter>
<parameter=path>
fuego-fresh/src/Rpc/RpcServer.cpp
</parameter>
<parameter=mode>
edit
</parameter>
</function>
</tool_call>