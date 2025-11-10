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

#include "Ipv4Resolver.h"
#include <cassert>
#include <random>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ws2tcpip.h>
#include <System/Dispatcher.h>
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>
#include <System/Ipv4Address.h>
#include <stdexcept>

namespace System {

Ipv4Resolver::Ipv4Resolver() : dispatcher(nullptr) {
}

Ipv4Resolver::Ipv4Resolver(Dispatcher& dispatcher) : dispatcher(&dispatcher) {
}

Ipv4Resolver::Ipv4Resolver(Ipv4Resolver&& other) : dispatcher(other.dispatcher) {
  if (dispatcher != nullptr) {
    other.dispatcher = nullptr;
  }
}

Ipv4Resolver::~Ipv4Resolver() {
}

Ipv4Resolver& Ipv4Resolver::operator=(Ipv4Resolver&& other) {
  dispatcher = other.dispatcher;
  if (dispatcher != nullptr) {
    other.dispatcher = nullptr;
  }

  return *this;
}

Ipv4Address Ipv4Resolver::resolve(const std::string& host) {
  assert(dispatcher != nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  addrinfo hints = { 0, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL };
  addrinfo* addressInfos;
  int result = getaddrinfo(host.c_str(), NULL, &hints, &addressInfos);
  if (result != 0) {
    throw std::runtime_error("Ipv4Resolver::resolve, getaddrinfo failed, " + errorMessage(result));
  }

  size_t count = 0;
  for (addrinfo* addressInfo = addressInfos; addressInfo != nullptr; addressInfo = addressInfo->ai_next) {
    ++count;
  }

  std::mt19937 generator{ std::random_device()() };
  size_t index = std::uniform_int_distribution<size_t>(0, count - 1)(generator);
  addrinfo* addressInfo = addressInfos;
  for (size_t i = 0; i < index; ++i) {
    addressInfo = addressInfo->ai_next;
  }

  Ipv4Address address(ntohl(reinterpret_cast<sockaddr_in*>(addressInfo->ai_addr)->sin_addr.S_un.S_addr));
  freeaddrinfo(addressInfo);
  return address;
}

}
