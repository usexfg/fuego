#include "WalletSerializationV3.h"
#include "crypto/hash.h"
#include <cstring>
#include "CryptoNote.h"
#include <sstream>

namespace CryptoNote {

static void password_to_key(const std::string& password, Crypto::aegis256x_key& key) {
    // Simple KDF: hash password to 32 bytes (replace with PBKDF2/Argon2 in production)
    Crypto::Hash hash = Crypto::cn_fast_hash(password.data(), password.size());
    std::memcpy(key.data, &hash, 32);
}

bool WalletSerializationV3::encrypt(const std::string& plain, const std::string& password, std::vector<uint8_t>& cipher, Crypto::aegis256x_nonce& nonce, Crypto::aegis256x_tag& tag) {
    Crypto::aegis256x_key key;
    password_to_key(password, key);
    nonce = Crypto::rand_aegis256x_nonce();
    cipher.resize(plain.size());
    return Crypto::aegis256x_encrypt(plain.data(), plain.size(), key, nonce, cipher.data(), tag);
}

bool WalletSerializationV3::decrypt(const std::vector<uint8_t>& cipher, const std::string& password, const Crypto::aegis256x_nonce& nonce, const Crypto::aegis256x_tag& tag, std::string& plain) {
    Crypto::aegis256x_key key;
    password_to_key(password, key);
    plain.resize(cipher.size());
    return Crypto::aegis256x_decrypt(cipher.data(), cipher.size(), key, nonce, tag, &plain[0]);
}

void WalletSerializationV3::serializeSubaddresses(const std::vector<Subaddress>& subaddresses, std::vector<uint8_t>& out) {
    std::ostringstream oss(std::ios::binary);
    uint32_t count = static_cast<uint32_t>(subaddresses.size());
    oss.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& sub : subaddresses) {
        oss.write(reinterpret_cast<const char*>(&sub.index), sizeof(sub.index));
        oss.write(reinterpret_cast<const char*>(&sub.spendPublicKey), sizeof(sub.spendPublicKey));
        oss.write(reinterpret_cast<const char*>(&sub.viewPublicKey), sizeof(sub.viewPublicKey));
        uint32_t labelLen = static_cast<uint32_t>(sub.label.size());
        oss.write(reinterpret_cast<const char*>(&labelLen), sizeof(labelLen));
        oss.write(sub.label.data(), labelLen);
    }
    std::string str = oss.str();
    out.assign(str.begin(), str.end());
}

void WalletSerializationV3::deserializeSubaddresses(const std::vector<uint8_t>& in, std::vector<Subaddress>& subaddresses) {
    std::istringstream iss(std::string(in.begin(), in.end()), std::ios::binary);
    uint32_t count = 0;
    iss.read(reinterpret_cast<char*>(&count), sizeof(count));
    subaddresses.clear();
    for (uint32_t i = 0; i < count; ++i) {
        Subaddress sub;
        iss.read(reinterpret_cast<char*>(&sub.index), sizeof(sub.index));
        iss.read(reinterpret_cast<char*>(&sub.spendPublicKey), sizeof(sub.spendPublicKey));
        iss.read(reinterpret_cast<char*>(&sub.viewPublicKey), sizeof(sub.viewPublicKey));
        uint32_t labelLen = 0;
        iss.read(reinterpret_cast<char*>(&labelLen), sizeof(labelLen));
        sub.label.resize(labelLen);
        iss.read(&sub.label[0], labelLen);
        subaddresses.push_back(sub);
    }
}

} // namespace CryptoNote 