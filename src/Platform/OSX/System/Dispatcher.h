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

#include <atomic>
#include <cstddef>
#include <functional>
#include <queue>
#include <stack>

namespace System {

struct NativeContextGroup;

struct NativeContext {
  void* uctx;
  void* stackPtr;
  bool interrupted;
  bool inExecutionQueue;
  NativeContext* next;
  NativeContextGroup* group;
  NativeContext* groupPrev;
  NativeContext* groupNext;
  std::function<void()> procedure;
  std::function<void()> interruptProcedure;
};

struct NativeContextGroup {
  NativeContext* firstContext;
  NativeContext* lastContext;
  NativeContext* firstWaiter;
  NativeContext* lastWaiter;
};

struct OperationContext {
  NativeContext* context;
  bool interrupted;
};

class Dispatcher {
public:
  Dispatcher();
  Dispatcher(const Dispatcher&) = delete;
  ~Dispatcher();
  Dispatcher& operator=(const Dispatcher&) = delete;
  void clear();
  void dispatch();
  NativeContext* getCurrentContext() const;
  void interrupt();
  void interrupt(NativeContext* context);
  bool interrupted();
  void pushContext(NativeContext* context);
  void remoteSpawn(std::function<void()>&& procedure);
  void yield();

  int getKqueue() const;
  NativeContext& getReusableContext();
  void pushReusableContext(NativeContext&);
  int getTimer();
  void pushTimer(int timer);

#ifdef __LP64__
  static const int SIZEOF_PTHREAD_MUTEX_T = 56 + sizeof(long);
#else
  static const int SIZEOF_PTHREAD_MUTEX_T = 40 + sizeof(long);
#endif

private:
  void spawn(std::function<void()>&& procedure);

  int kqueue;
  int lastCreatedTimer;
  alignas(std::max_align_t) uint8_t mutex[SIZEOF_PTHREAD_MUTEX_T];
  std::atomic<bool> remoteSpawned;
  std::queue<std::function<void()>> remoteSpawningProcedures;
  std::stack<int> timers;

  NativeContext mainContext;
  NativeContextGroup contextGroup;
  NativeContext* currentContext;
  NativeContext* firstResumingContext;
  NativeContext* lastResumingContext;
  NativeContext* firstReusableContext;
  size_t runningContextCount;

  void contextProcedure(void* uctx);
  static void contextProcedureStatic(intptr_t context);
};

}
