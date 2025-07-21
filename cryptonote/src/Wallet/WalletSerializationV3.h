#pragma once
#include <string>
#include <vector>
#include "crypto/aegis256x.h"
#include "CryptoNote.h"

namespace CryptoNote {

class WalletSerializationV3 {
public:
    static const uint32_t WALLET_SERIALIZATION_VERSION_3 = 3;
    static bool encrypt(const std::string& plain, const std::string& password, std::vector<uint8_t>& cipher, Crypto::aegis256x_nonce& nonce, Crypto::aegis256x_tag& tag);
    static bool decrypt(const std::vector<uint8_t>& cipher, const std::string& password, const Crypto::aegis256x_nonce& nonce, const Crypto::aegis256x_tag& tag, std::string& plain);
    // Subaddress serialization
    static void serializeSubaddresses(const std::vector<Subaddress>& subaddresses, std::vector<uint8_t>& out);
    static void deserializeSubaddresses(const std::vector<uint8_t>& in, std::vector<Subaddress>& subaddresses);
};

} // namespace CryptoNote 