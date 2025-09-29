// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#pragma once 

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

template < typename T, typename Container = std::deque<T> >
class BlockingQueue {
public:

  typedef BlockingQueue<T, Container> ThisType;

  BlockingQueue(size_t maxSize = 1) : 
    m_maxSize(maxSize), m_closed(false) {}

  template <typename TT>
  bool push(TT&& v) {
    std::unique_lock<std::mutex> lk(m_mutex);

    while (!m_closed && m_queue.size() >= m_maxSize) {
      m_haveSpace.wait(lk);
    }

    if (m_closed) {
      return false;
    }

    m_queue.push_back(std::forward<TT>(v));
    m_haveData.notify_one();
    return true;
  }

  bool pop(T& v) {
    std::unique_lock<std::mutex> lk(m_mutex);

    while (m_queue.empty()) {
      if (m_closed) {
        // all data has been processed, queue is closed
        return false;
      }
      m_haveData.wait(lk);
    }
   
    v = std::move(m_queue.front());
    m_queue.pop_front();

    // we can have several waiting threads to unblock
    if (m_closed && m_queue.empty() )
      m_haveSpace.notify_all();
    else
      m_haveSpace.notify_one();

    return true;
  }

  void close(bool wait = false) {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_closed = true;
    m_haveData.notify_all(); // wake up threads in pop()
    m_haveSpace.notify_all();

    if (wait) {
      while (!m_queue.empty()) {
        m_haveSpace.wait(lk);
      }
    }
  }

  size_t size() {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_queue.size();
  }

  size_t capacity() const {
    return m_maxSize;
  }

private:

  const size_t m_maxSize;
  Container m_queue;
  bool m_closed;
  
  std::mutex m_mutex;
  std::condition_variable m_haveData;
  std::condition_variable m_haveSpace;
};

template <typename QueueT>
class GroupClose {
public:

  GroupClose(QueueT& queue, size_t groupSize)
    : m_queue(queue), m_count(groupSize) {}

  void close() {
    if (m_count == 0)
      return;
    if (m_count.fetch_sub(1) == 1)
      m_queue.close();
  }

private:

  std::atomic<size_t> m_count;
  QueueT& m_queue;

};
