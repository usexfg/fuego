// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "ErrorMessage.h"

#include <cstddef>
#include <windows.h>

namespace System {

std::string lastErrorMessage() {
  return errorMessage(GetLastError());
}

std::string errorMessage(int error) {
  struct Buffer {
    ~Buffer() {
      if (pointer != nullptr) {
        LocalFree(pointer);
      }
    }

    LPTSTR pointer = nullptr;
  } buffer;

  auto size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, error,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&buffer.pointer), 0, nullptr);
  return "result=" + std::to_string(error) + ", " + std::string(buffer.pointer, size);
}

}
