// Copyright (c) 2017-2022, Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019, The Karbowanec developers
// Copyright (c) 2012-2018, The CryptoNote developers
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

#include "WalletAsyncContextCounter.h"
#include <mutex>
#include <condition_variable>

// Ensure std namespace is properly resolved at global scope
using namespace std;

namespace CryptoNote {

struct WalletAsyncContextCounter::Impl {
  uint32_t m_asyncContexts;
  std::condition_variable m_cv;
  std::mutex m_mutex;
  
  Impl() : m_asyncContexts(0) {}
};

WalletAsyncContextCounter::WalletAsyncContextCounter()
  : m_impl(::std::make_unique<Impl>()) {
}

WalletAsyncContextCounter::~WalletAsyncContextCounter() = default;

void WalletAsyncContextCounter::addAsyncContext() {
  ::std::unique_lock<::std::mutex> lock(m_impl->m_mutex);
  m_impl->m_asyncContexts++;
}

void WalletAsyncContextCounter::delAsyncContext() {
  ::std::unique_lock<::std::mutex> lock(m_impl->m_mutex);
  m_impl->m_asyncContexts--;

  if (!m_impl->m_asyncContexts) m_impl->m_cv.notify_one();
}

void WalletAsyncContextCounter::waitAsyncContextsFinish() {
  ::std::unique_lock<::std::mutex> lock(m_impl->m_mutex);
  while (m_impl->m_asyncContexts > 0)
    m_impl->m_cv.wait(lock);
}

} //namespace CryptoNote