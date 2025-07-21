// Copyright (c) 2017-2022 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
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

#pragma once

#include <cstdint>
#include "crypto.h"

namespace Crypto {

/* Subaddress derivation functions
 * Based on Monero's subaddress implementation
 * 
 * Correct derivation uses (viewPublicKey, viewSecretKey) for key derivation,
 * not (viewSecretKey, viewSecretKey) as initially implemented.
 */
void derive_subaddress_spend_key(const SecretKey& viewSecretKey, const SecretKey& spendSecretKey, 
  uint32_t subaddressIndex, SecretKey& derivedSpendKey);

void derive_subaddress_view_key(const SecretKey& viewSecretKey, uint32_t subaddressIndex, 
  SecretKey& derivedViewKey);

void derive_subaddress_keys(const SecretKey& viewSecretKey, const SecretKey& spendSecretKey,
  uint32_t subaddressIndex, SecretKey& derivedSpendKey, SecretKey& derivedViewKey);

} // namespace Crypto 