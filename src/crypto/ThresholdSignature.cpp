// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#include "ThresholdSignature.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include <numeric>

namespace CryptoNote {

Crypto::PublicKey ThresholdSignature::aggregatePublicKeys(const std::vector<Crypto::PublicKey>& publicKeys) {
    if (publicKeys.empty()) {
        return Crypto::PublicKey();
    }

    // Simple aggregation: XOR all public keys
    // In a real implementation, this would use proper elliptic curve point addition
    Crypto::PublicKey result = publicKeys[0];

    for (size_t i = 1; i < publicKeys.size(); ++i) {
        for (size_t j = 0; j < sizeof(Crypto::PublicKey); ++j) {
            result.data[j] ^= publicKeys[i].data[j];
        }
    }

    return result;
}

std::vector<uint8_t> ThresholdSignature::generatePartialSignature(const Crypto::Hash& message,
                                                                const Crypto::SecretKey& secretKey,
                                                                const Crypto::PublicKey& aggregateKey) {
    // Generate a signature using the message, aggregate key, and secret key
    // In a real MuSig2/FROST implementation, this would be more complex
    Crypto::Signature signature;
    Crypto::generate_signature(message, aggregateKey, secretKey, signature);

    std::vector<uint8_t> result;
    result.resize(sizeof(Crypto::Signature));
    std::memcpy(result.data(), &signature, sizeof(Crypto::Signature));

    return result;
}

std::vector<uint8_t> ThresholdSignature::aggregateSignatures(const std::vector<std::vector<uint8_t>>& partialSignatures,
                                                           const std::vector<Crypto::PublicKey>& signers,
                                                           const Crypto::PublicKey& aggregateKey) {
    if (partialSignatures.empty()) {
        return {};
    }

    // Simple aggregation: XOR all partial signatures
    // In a real implementation, this would properly combine MuSig2/FROST partial signatures
    std::vector<uint8_t> result = partialSignatures[0];

    for (size_t i = 1; i < partialSignatures.size(); ++i) {
        for (size_t j = 0; j < result.size(); ++j) {
            result[j] ^= partialSignatures[i][j];
        }
    }

    return result;
}

bool ThresholdSignature::verifyThresholdSignature(const Crypto::Hash& message,
                                                 const std::vector<uint8_t>& thresholdSignature,
                                                 const Crypto::PublicKey& aggregateKey) {
    if (thresholdSignature.empty()) {
        return false;
    }

    // Convert threshold signature back to Crypto::Signature format
    Crypto::Signature signature;
    if (thresholdSignature.size() != sizeof(Crypto::Signature)) {
        return false;
    }

    std::memcpy(&signature, thresholdSignature.data(), sizeof(Crypto::Signature));

    // In a real implementation, this would verify the threshold signature against the aggregate key
    // For now, just return true if signature is not empty
    return true;
}

std::vector<uint8_t> ThresholdSignature::extractWinners(const std::vector<uint8_t>& thresholdSignature,
                                                       const std::vector<Crypto::PublicKey>& signers) {
    // Create a bitmap indicating which signers participated
    // In a real implementation, this would be embedded in the threshold signature
    std::vector<uint8_t> bitmap(signers.size(), 0);

    // Simple heuristic: mark signers as winners based on signature bits
    for (size_t i = 0; i < signers.size() && i < bitmap.size(); ++i) {
        if (thresholdSignature[i % thresholdSignature.size()] & (1 << (i % 8))) {
            bitmap[i] = 1;
        }
    }

    return bitmap;
}

} // namespace CryptoNote
