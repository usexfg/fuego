// Copyright (c) 2017-2022 Fuego Developers
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
#include "Event.h"
#include "InterruptedException.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace System {

template<typename ResultType = void>
class Context {
public:
  // Forward declarations of helper structs
  struct ThreadFunctor;
  struct WaitPredicate;

  Context(Dispatcher& dispatcher, const std::function<ResultType()>& target);
  ~Context();

  ResultType& get();
  void interrupt();
  void wait();

private:
  Context(const Context&);
  Context& operator=(const Context&);

  uint8_t resultStorage[sizeof(ResultType)];
  Dispatcher& dispatcher;
  std::function<ResultType()> target;
  Event ready;
  std::thread thread;
  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<bool> completed;
  std::atomic<bool> interrupted;
  std::exception_ptr exceptionPointer;

  // Helper structs defined after the class
  friend struct ThreadFunctor;
  friend struct WaitPredicate;
};

// Void specialization
template<>
class Context<void> {
public:
  // Forward declarations of helper structs
  struct ThreadFunctor;
  struct WaitPredicate;

  Context(Dispatcher& dispatcher, const std::function<void()>& target);
  ~Context();

  void get();
  void interrupt();
  void wait();

private:
  Context(const Context&);
  Context& operator=(const Context&);

  Dispatcher& dispatcher;
  std::function<void()> target;
  Event ready;
  std::thread thread;
  std::mutex mutex;
  std::condition_variable cv;
  std::atomic<bool> completed;
  std::atomic<bool> interrupted;
  std::exception_ptr exceptionPointer;

  // Helper structs defined after the class
  friend struct ThreadFunctor;
  friend struct WaitPredicate;
};

// Helper structs for non-void Context
template<typename ResultType>
struct Context<ResultType>::ThreadFunctor {
  Context<ResultType>* context;
  ThreadFunctor(Context<ResultType>* ctx) : context(ctx) {}
  
  void operator()() {
    try {
      if (!std::is_void<ResultType>::value) {
        new(context->resultStorage) ResultType(context->target());
      } else {
        context->target();
      }
    } catch (...) {
      context->exceptionPointer = std::current_exception();
    }
    
    {
      std::lock_guard<std::mutex> lock(context->mutex);
      context->completed = true;
    }
    context->cv.notify_one();
    context->ready.set();
  }
};

template<typename ResultType>
struct Context<ResultType>::WaitPredicate {
  Context<ResultType>* context;
  WaitPredicate(Context<ResultType>* ctx) : context(ctx) {}
  
  bool operator()() const {
    return context->completed || context->interrupted;
  }
};

// Helper structs for void Context
struct Context<void>::ThreadFunctor {
  Context<void>* context;
  ThreadFunctor(Context<void>* ctx) : context(ctx) {}
  
  void operator()() {
    try {
      context->target();
    } catch (...) {
      context->exceptionPointer = std::current_exception();
    }
    
    {
      std::lock_guard<std::mutex> lock(context->mutex);
      context->completed = true;
    }
    context->cv.notify_one();
    context->ready.set();
  }
};

struct Context<void>::WaitPredicate {
  Context<void>* context;
  WaitPredicate(Context<void>* ctx) : context(ctx) {}
  
  bool operator()() const {
    return context->completed || context->interrupted;
  }
};

// Template method implementations
template<typename ResultType>
Context<ResultType>::Context(Dispatcher& dispatcher, const std::function<ResultType()>& target) :
  dispatcher(dispatcher), 
  target(target), 
  ready(dispatcher),
  completed(false),
  interrupted(false) {
    
  thread = std::thread(ThreadFunctor(this));
}

template<typename ResultType>
Context<ResultType>::~Context() {
  interrupt();
  wait();
  if (thread.joinable()) {
    thread.join();
  }
}

template<typename ResultType>
ResultType& Context<ResultType>::get() {
  wait();
  if (exceptionPointer != 0) {
    std::rethrow_exception(exceptionPointer);
  }

  if (!std::is_void<ResultType>::value) {
    return *reinterpret_cast<ResultType*>(resultStorage);
  }
}

template<typename ResultType>
void Context<ResultType>::interrupt() {
  {
    std::lock_guard<std::mutex> lock(mutex);
    interrupted = true;
  }
  cv.notify_one();
}

template<typename ResultType>
void Context<ResultType>::wait() {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, WaitPredicate(this));

  if (interrupted) {
    dispatcher.interrupt();
  }
}

// Void specialization implementation
inline Context<void>::Context(Dispatcher& dispatcher, const std::function<void()>& target) :
  dispatcher(dispatcher), 
  target(target), 
  ready(dispatcher),
  completed(false),
  interrupted(false) {
    
  thread = std::thread(ThreadFunctor(this));
}

inline Context<void>::~Context() {
  interrupt();
  wait();
  if (thread.joinable()) {
    thread.join();
  }
}

inline void Context<void>::get() {
  wait();
  if (exceptionPointer != 0) {
    std::rethrow_exception(exceptionPointer);
  }
}

inline void Context<void>::interrupt() {
  {
    std::lock_guard<std::mutex> lock(mutex);
    interrupted = true;
  }
  cv.notify_one();
}

inline void Context<void>::wait() {
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, WaitPredicate(this));

  if (interrupted) {
    dispatcher.interrupt();
  }
}

}  // namespace System
