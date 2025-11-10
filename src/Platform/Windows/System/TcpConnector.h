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

#include <cstdint>
#include <string>

namespace System {

class Dispatcher;
class Ipv4Address;
class TcpConnection;

class TcpConnector {
public:
  TcpConnector();
  explicit TcpConnector(Dispatcher& dispatcher);
  TcpConnector(const TcpConnector&) = delete;
  TcpConnector(TcpConnector&& other);
  ~TcpConnector();
  TcpConnector& operator=(const TcpConnector&) = delete;
  TcpConnector& operator=(TcpConnector&& other);
  TcpConnection connect(const Ipv4Address& address, uint16_t port);

private:
  Dispatcher* dispatcher;
  void* context;
};

}
