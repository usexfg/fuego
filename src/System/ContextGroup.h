// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
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

#include "Dispatcher.h"
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace System {

class ContextGroup {
public:
  explicit ContextGroup(Dispatcher& dispatcher);
  ContextGroup(const ContextGroup&);
  ContextGroup& operator=(const ContextGroup&);
  ContextGroup(ContextGroup& other);
  ContextGroup& operator=(ContextGroup& other);
  ~ContextGroup();

  void interrupt();
  void spawn(std::function<void()>&& procedure);
  void wait();

private:
#ifdef ARCH_ARM64
  void threadFunction(const std::function<void()>& procedure, size_t threadIndex);

  Dispatcher& dispatcher;
  std::vector<std::thread> threads_;
  std::vector<bool> running_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> interrupted_;
#else
  Dispatcher* dispatcher;
  struct NativeContextGroup {
    void* firstContext;
    void* lastContext;
    void* firstWaiter;
    void* lastWaiter;
  } contextGroup;
#endif
};

}  // namespace System
