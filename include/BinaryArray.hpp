// Copyright (c) 2012-2018, The CryptoNote developers, The Bytecoin developers.
// Copyright (c) 2018-2021, Fandom Gold Society
// Copyright (c) 2019-2021, Fango Developers
//
// This file is part of Fango.
//
// Fango is free software distributed in the hope that it
// will be useful- but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You are encouraged to redistribute it and/or modify it
// under the terms of the GNU General Public License v3 or later
// versions as published by the Free Software Foundation.
// You should have received a copy of the GNU General Public License
// along with Fango. If not, see <https://www.gnu.org/licenses/>.


#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <CryptoNote.h>

using namespace CryptoNote;

namespace Common {

template<class It>
inline BinaryArray::iterator append(BinaryArray &ba, It be, It en) {
	return ba.insert(ba.end(), be, en);
}
inline BinaryArray::iterator append(BinaryArray &ba, size_t add, BinaryArray::value_type va) {
	return ba.insert(ba.end(), add, va);
}
inline BinaryArray::iterator append(BinaryArray &ba, const BinaryArray &other) {
	return ba.insert(ba.end(), other.begin(), other.end());
}

} // namespace Common
