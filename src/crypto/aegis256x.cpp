#include "aegis256x.h"
#include <cstring>
#include <random>

namespace Crypto {

bool aegis256x_encrypt(const void* plaintext, size_t length, const aegis256x_key& key, const aegis256x_nonce& nonce, void* ciphertext, aegis256x_tag& tag) {
    // Placeholder: XOR with key+nonce, not real AEGIS-256X
    const uint8_t* in = reinterpret_cast<const uint8_t*>(plaintext);
    uint8_t* out = reinterpret_cast<uint8_t*>(ciphertext);
    for (size_t i = 0; i < length; ++i) {
        out[i] = in[i] ^ key.data[i % 32] ^ nonce.data[i % 16];
    }
    // Fake tag: XOR of all bytes
    uint8_t t = 0;
    for (size_t i = 0; i < length; ++i) t ^= out[i];
    for (size_t i = 0; i < 16; ++i) tag.data[i] = t ^ key.data[i];
    return true;
}

bool aegis256x_decrypt(const void* ciphertext, size_t length, const aegis256x_key& key, const aegis256x_nonce& nonce, const aegis256x_tag& tag, void* plaintext) {
    // Placeholder: XOR with key+nonce, not real AEGIS-256X
    const uint8_t* in = reinterpret_cast<const uint8_t*>(ciphertext);
    uint8_t* out = reinterpret_cast<uint8_t*>(plaintext);
    for (size_t i = 0; i < length; ++i) {
        out[i] = in[i] ^ key.data[i % 32] ^ nonce.data[i % 16];
    }
    // Fake tag check
    uint8_t t = 0;
    for (size_t i = 0; i < length; ++i) t ^= in[i];
    for (size_t i = 0; i < 16; ++i) {
        if ((t ^ key.data[i]) != tag.data[i]) return false;
    }
    return true;
}

aegis256x_key rand_aegis256x_key() {
    aegis256x_key k;
    std::random_device rd;
    for (int i = 0; i < 32; ++i) k.data[i] = rd();
    return k;
}

aegis256x_nonce rand_aegis256x_nonce() {
    aegis256x_nonce n;
    std::random_device rd;
    for (int i = 0; i < 16; ++i) n.data[i] = rd();
    return n;
}

} // namespace Crypto 