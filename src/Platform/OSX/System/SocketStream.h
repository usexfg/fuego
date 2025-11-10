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


#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <streambuf>

namespace System {

class SocketStreambuf: public std::streambuf {
  public:
    SocketStreambuf(char *data, size_t lenght);
    ~SocketStreambuf();
    void getRespdata(std::vector<uint8_t> &data);
    void setRespdata(const std::vector<uint8_t> &data);
  private:
    size_t lenght;
    bool read_t;
    std::array<uint8_t, 1024> writeBuf;
    std::vector<uint8_t> readBuf;
    std::vector<uint8_t> resp_data;
    std::streambuf::int_type overflow(std::streambuf::int_type ch) override;
    std::streambuf::int_type underflow() override;
    int sync() override;
    bool dumpBuffer(bool finalize);
};

}
