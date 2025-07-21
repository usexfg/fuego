#pragma once
#include <cstddef>
#include <cstdint>

namespace Crypto {

struct aegis256x_key {
    uint8_t data[32];
};

struct aegis256x_nonce {
    uint8_t data[16];
};

struct aegis256x_tag {
    uint8_t data[16];
};

// Encrypts plaintext to ciphertext with AEGIS-256X
// Returns true on success
bool aegis256x_encrypt(const void* plaintext, size_t length, const aegis256x_key& key, const aegis256x_nonce& nonce, void* ciphertext, aegis256x_tag& tag);

// Decrypts ciphertext to plaintext with AEGIS-256X
// Returns true on success, false if tag fails
bool aegis256x_decrypt(const void* ciphertext, size_t length, const aegis256x_key& key, const aegis256x_nonce& nonce, const aegis256x_tag& tag, void* plaintext);

// Utility: generate random key/nonce
aegis256x_key rand_aegis256x_key();
aegis256x_nonce rand_aegis256x_nonce();

enum class AEGIS256XError {
    SUCCESS = 0,
    AUTH_FAIL = 1,
    INVALID_ARG = 2
};

} // namespace Crypto 