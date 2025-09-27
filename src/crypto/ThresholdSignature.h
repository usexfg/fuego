// Copyright (c) 2024-2025 Fuego Developers
// Distributed under the MIT/X11 software license

#pragma once

#include <vector>
#include "crypto/crypto.h"
#include "crypto/hash.h"

namespace CryptoNote {

class ThresholdSignature {
public:
    // Generate aggregate public key from multiple public keys
    static Crypto::PublicKey aggregatePublicKeys(const std::vector<Crypto::PublicKey>& publicKeys);

    // Generate partial signature for threshold signature scheme
    static std::vector<uint8_t> generatePartialSignature(const Crypto::Hash& message,
                                                        const Crypto::SecretKey& secretKey,
                                                        const Crypto::PublicKey& aggregateKey);

    // Aggregate partial signatures into threshold signature
    static std::vector<uint8_t> aggregateSignatures(const std::vector<std::vector<uint8_t>>& partialSignatures,
                                                   const std::vector<Crypto::PublicKey>& signers,
                                                   const Crypto::PublicKey& aggregateKey);

    // Verify threshold signature
    static bool verifyThresholdSignature(const Crypto::Hash& message,
                                       const std::vector<uint8_t>& thresholdSignature,
                                       const Crypto::PublicKey& aggregateKey);

    // Extract winner bitmap from aggregated signature (for fee distribution)
    static std::vector<uint8_t> extractWinners(const std::vector<uint8_t>& thresholdSignature,
                                             const std::vector<Crypto::PublicKey>& signers);

private:
    // Placeholder implementations - would need proper MuSig2/FROST
    static std::vector<uint8_t> computeAggregateSignature(const std::vector<std::vector<uint8_t>>& partialSignatures);
};

} // namespace CryptoNote
