// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2014-2024 The Monero Project
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope
// it will be useful, but WITHOUT ANY WARRANTY; without even an
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.


#include "subaddress.h"
#include "crypto.h"

namespace Crypto {

void derive_subaddress_spend_key(const SecretKey& viewSecretKey, const SecretKey& spendSecretKey, 
  uint32_t subaddressIndex, SecretKey& derivedSpendKey) {
  // Convert view secret key to public key for proper key derivation
  PublicKey viewPublicKey;
  secret_key_to_public_key(viewSecretKey, viewPublicKey);
  
  // Generate key derivation using proper key types (PublicKey, SecretKey)
  KeyDerivation derivation;
  generate_key_derivation(viewPublicKey, viewSecretKey, derivation);
  
  // Derive subaddress spend key: Hs(a || index) + b
  // where a = view secret key, b = spend secret key
  derive_secret_key(derivation, subaddressIndex, spendSecretKey, derivedSpendKey);
}

void derive_subaddress_view_key(const SecretKey& viewSecretKey, uint32_t subaddressIndex, 
  SecretKey& derivedViewKey) {
  // For subaddresses, view key remains the same as main address
  derivedViewKey = viewSecretKey;
}

void derive_subaddress_keys(const SecretKey& viewSecretKey, const SecretKey& spendSecretKey,
  uint32_t subaddressIndex, SecretKey& derivedSpendKey, SecretKey& derivedViewKey) {
  derive_subaddress_spend_key(viewSecretKey, spendSecretKey, subaddressIndex, derivedSpendKey);
  derive_subaddress_view_key(viewSecretKey, subaddressIndex, derivedViewKey);
}

} // namespace Crypto 