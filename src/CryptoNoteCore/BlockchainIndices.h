// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope
// that it will be useful, but WITHOUT ANY WARRANTY; without even
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>

#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <parallel_hashmap/phmap.h>
#include "crypto/hash.h"
#include "CryptoNoteBasic.h"
using phmap::flat_hash_map;
namespace CryptoNote {

class ISerializer;

class PaymentIdIndex {
public:
  PaymentIdIndex() = default;

  bool add(const Transaction& transaction);
  bool remove(const Transaction& transaction);
  bool find(const Crypto::Hash& paymentId, std::vector<Crypto::Hash>& transactionHashes);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive> 
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
  }
private:
  std::unordered_multimap<Crypto::Hash, Crypto::Hash> index;
};

class TimestampBlocksIndex {
public:
  TimestampBlocksIndex() = default;

  bool add(uint64_t timestamp, const Crypto::Hash& hash);
  bool remove(uint64_t timestamp, const Crypto::Hash& hash);
  bool find(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t hashesNumberLimit, std::vector<Crypto::Hash>& hashes, uint32_t& hashesNumberWithinTimestamps);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive> 
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
  }
private:
  std::multimap<uint64_t, Crypto::Hash> index;
};

class TimestampTransactionsIndex {
public:
  TimestampTransactionsIndex() = default;

  bool add(uint64_t timestamp, const Crypto::Hash& hash);
  bool remove(uint64_t timestamp, const Crypto::Hash& hash);
  bool find(uint64_t timestampBegin, uint64_t timestampEnd, uint64_t hashesNumberLimit, std::vector<Crypto::Hash>& hashes, uint64_t& hashesNumberWithinTimestamps);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive>
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
  }
private:
  std::multimap<uint64_t, Crypto::Hash> index;
};

class GeneratedTransactionsIndex {
public:
  GeneratedTransactionsIndex();

  bool add(const Block& block);
  bool remove(const Block& block);
  bool find(uint32_t height, uint64_t& generatedTransactions);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive> 
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
    archive & lastGeneratedTxNumber;
  }
private:
  flat_hash_map<uint32_t, uint64_t> index;
  uint64_t lastGeneratedTxNumber;
};

class OrphanBlocksIndex {
public:
  OrphanBlocksIndex() = default;

  bool add(const Block& block);
  bool remove(const Block& block);
  bool find(uint32_t height, std::vector<Crypto::Hash>& blockHashes);
  void clear();
private:
  std::unordered_multimap<uint32_t, Crypto::Hash> index;
};

}
