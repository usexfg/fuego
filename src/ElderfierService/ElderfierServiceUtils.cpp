// Copyright (c) 2024 Fuego Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../../include/ElderfierServiceTypesSimple.h"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace CryptoNote {

std::string serviceModifierToString(const ElderfierServiceModifier& modifier) {
    std::ostringstream oss;
    
    oss << "ElderfierServiceModifier{";
    oss << "last_pow_block_height=" << modifier.last_pow_block_height << ", ";
    oss << "modifier_timestamp=" << modifier.modifier_timestamp << ", ";
    oss << "modifier_sequence=" << modifier.modifier_sequence << ", ";
    
    // Convert hash to hex string
    oss << "last_pow_block_hash=";
    for (size_t i = 0; i < modifier.last_pow_block_hash.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(modifier.last_pow_block_hash[i]);
    }
    oss << "}";
    
    return oss.str();
}

std::string serviceProofToString(const ElderfierServiceProof& proof) {
    std::ostringstream oss;
    
    oss << "ElderfierServiceProof{";
    oss << "proof_version=" << static_cast<int>(proof.proof_version) << ", ";
    oss << "service_type=" << static_cast<int>(proof.service_type) << ", ";
    oss << "minimum_stake_atomic=" << proof.minimum_stake_atomic << ", ";
    oss << "proof_timestamp=" << proof.proof_timestamp << ", ";
    oss << "proof_sequence=" << proof.proof_sequence << ", ";
    
    // Convert hashes to hex strings
    oss << "proof_hash=";
    for (size_t i = 0; i < proof.proof_hash.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(proof.proof_hash[i]);
    }
    oss << ", ";
    
    oss << "service_modifier_hash=";
    for (size_t i = 0; i < proof.service_modifier_hash.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(proof.service_modifier_hash[i]);
    }
    oss << ", ";
    
    oss << "fee_address_hash=";
    for (size_t i = 0; i < proof.fee_address_hash.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(proof.fee_address_hash[i]);
    }
    oss << "}";
    
    return oss.str();
}

std::string serviceKernelToString(const ElderfierServiceKernel& kernel) {
    std::ostringstream oss;
    
    oss << "ElderfierServiceKernel{";
    oss << "kernel_timestamp=" << kernel.kernel_timestamp << ", ";
    oss << "minimum_stake_atomic=" << kernel.minimum_stake_atomic << ", ";
    
    // Convert hashes to hex strings
    oss << "fee_address_hash=";
    for (size_t i = 0; i < kernel.fee_address_hash.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(kernel.fee_address_hash[i]);
    }
    oss << ", ";
    
    oss << "stake_key_image=";
    for (size_t i = 0; i < kernel.stake_key_image.size(); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(kernel.stake_key_image[i]);
    }
    oss << ", ";
    
    oss << "service_modifier=" << serviceModifierToString(kernel.service_modifier);
    oss << "}";
    
    return oss.str();
}

} // namespace CryptoNote
