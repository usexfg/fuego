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
#include <functional>
#include <map>
#include <queue>

namespace System {

struct NativeContextGroup;

struct NativeContext {
  void* fiber;
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

  // Platform-specific
  void addTimer(uint64_t time, NativeContext* context);
  void* getCompletionPort() const;
  NativeContext& getReusableContext();
  void pushReusableContext(NativeContext&);
  void interruptTimer(uint64_t time, NativeContext* context);

private:
  void spawn(std::function<void()>&& procedure);
  void* completionPort;
  uint8_t criticalSection[2 * sizeof(long) + 4 * sizeof(void*)];
  bool remoteNotificationSent;
  std::queue<std::function<void()>> remoteSpawningProcedures;
  uint8_t remoteSpawnOverlapped[4 * sizeof(void*)];
  uint32_t threadId;
  std::multimap<uint64_t, NativeContext*> timers;

  NativeContext mainContext;
  NativeContextGroup contextGroup;
  NativeContext* currentContext;
  NativeContext* firstResumingContext;
  NativeContext* lastResumingContext;
  NativeContext* firstReusableContext;
  size_t runningContextCount;

  void contextProcedure();
  static void __stdcall contextProcedureStatic(void* context);
};

}
