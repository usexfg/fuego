// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2017-2021 Fandom Gold Society
//
// This file is part of Fango.
//
// FANGO is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// FANGO is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public License
// along with FANGO.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

namespace CryptoNote {
class ISerializer;

class DepositIndex {
public:
  using DepositAmount = int64_t;
/*using DepositInterest = uint64_t;*/
  using DepositHeight = uint32_t;
  DepositIndex();
  explicit DepositIndex(DepositHeight expectedHeight);
  void pushBlock(DepositAmount amount/*, DepositInterest interest*/); 
  void popBlock(); 
  void reserve(DepositHeight expectedHeight);
  size_t popBlocks(DepositHeight from); 
  DepositAmount depositAmountAtHeight(DepositHeight height) const;
  DepositAmount fullDepositAmount() const; 
/*DepositInterest depositInterestAtHeight(DepositHeight height) const;
  DepositInterest fullInterestAmount() const; */
  DepositHeight size() const;
  void serialize(ISerializer& s);

private:
  struct DepositIndexEntry {
    DepositHeight height;
    DepositAmount amount;
/*  DepositInterest interest;*/

    void serialize(ISerializer& s);
  };

  using IndexType = std::vector<DepositIndexEntry>;
  IndexType::const_iterator upperBound(DepositHeight height) const;
  IndexType index;
  DepositHeight blockCount;
};
}
