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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <queue>
#include <stack>

namespace System {

struct NativeContextGroup;

struct NativeContext {
  void* ucontext;
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
  NativeContext *context;
  bool interrupted;
  uint32_t events;
};

struct ContextPair {
  OperationContext *readContext;
  OperationContext *writeContext;
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

  // system-dependent
  int getEpoll() const;
  NativeContext& getReusableContext();
  void pushReusableContext(NativeContext&);
  int getTimer();
  void pushTimer(int timer);

#ifdef __x86_64__
#if __WORDSIZE == 64
  static const int SIZEOF_PTHREAD_MUTEX_T = 40;
#else
  static const int SIZEOF_PTHREAD_MUTEX_T = 32;
#endif
#else
#if defined(__aarch64__)
  static const int SIZEOF_PTHREAD_MUTEX_T = 48;
#else
  static const int SIZEOF_PTHREAD_MUTEX_T = 24;
#endif
#endif

private:
  void spawn(std::function<void()>&& procedure);
  int epoll;
  alignas(void*) uint8_t mutex[SIZEOF_PTHREAD_MUTEX_T];
  int remoteSpawnEvent;
  ContextPair remoteSpawnEventContext;
  std::queue<std::function<void()>> remoteSpawningProcedures;
  std::stack<int> timers;

  NativeContext mainContext;
  NativeContextGroup contextGroup;
  NativeContext* currentContext;
  NativeContext* firstResumingContext;
  NativeContext* lastResumingContext;
  NativeContext* firstReusableContext;
  size_t runningContextCount;

  void contextProcedure(void* ucontext);
  static void contextProcedureStatic(void* context);
};

}
