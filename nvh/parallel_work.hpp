/*
 * Copyright (c) 2022-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include "BS_thread_pool.hpp"

#include <cstdint>
#include <execution>
#include <functional>

namespace nvh {
/* @DOC_START
Distributes batches of loops over BATCHSIZE items across multiple threads. numItems reflects the total number
of items to process.

```cpp
batches:         fn (uint64_t itemIndex)
                 callback does single item

batches_indexed: fn (uint64_t itemIndex, uint32_t threadIndex)
                 callback does single item
                 uses at most get_thread_pool().thread_count() threads

ranges:          fn (uint64_t itemBegin, uint64_t itemEnd, uint32_t threadIndex)
                 callback does loop: for (uint64_t itemIndex = itemBegin; itemIndex < itemEnd; itemIndex++)
                 uses at most get_thread_pool().thread_count() threads
```

`BATCHSIZE` will also be used as the threshold for when to switch from
single-threaded to multi-threaded execution. For this reason, it should be set
to a power of 2 around where multi-threaded is faster than single-threaded for
the given function. Some examples are:
* 8192 for trivial workloads (a * x + y)
* 2048 for animation workloads (multiplication by a single matrix)
* 512 for more computationally heavy workloads (run XTEA)
* 1 for full parallelization (load an image)

If numThreads is equal to 1, runs single-threaded. Otherwise, `numThreads`
is ignored.

All functions here are thread-safe. However, if the callback does
synchronization (e.g. locking, mutexes), then it is only safe to use
batches_indexed and ranges.

@DOC_END */

// Utility to support parallel execution with indices without unnecessarily
// creating an array containing indices. This is a random access iota_view since
// std::views::iota_view does not satisfy random_access. Maybe support will be
// added/fixed, but this is useful for now.
// https://stackoverflow.com/questions/69078426/why-doesnt-stdexecutionpar-launch-threads-with-stdviewsiota-iterators/
// Additionally, this allows us to use something iota-like without requiring
// std:c++latest on MSVC 2019.
template <class T>
struct iota_iterator
{
  using value_type = T;
  // [iterator.traits] in the C++ standard requires this to be a signed type.
  // We choose int64_t here, because it's conceivable someone could use
  // T == uint32_t and then iterate over more than 2^31 - 1 elements.
  using difference_type                                         = int64_t;
  using pointer                                                 = T*;
  using reference                                               = T&;
  using iterator_category                                       = std::random_access_iterator_tag;
  iota_iterator()                                               = default;
  iota_iterator(const iota_iterator& other) noexcept            = default;
  iota_iterator(iota_iterator&& other) noexcept                 = default;
  iota_iterator& operator=(const iota_iterator& other) noexcept = default;
  iota_iterator& operator=(iota_iterator&& other) noexcept      = default;
  iota_iterator(T i_)
      : i(i_)
  {
  }
  value_type     operator*() const { return i; }
  iota_iterator& operator++()
  {
    ++i;
    return *this;
  }
  iota_iterator operator++(int)
  {
    iota_iterator t(*this);
    ++*this;
    return t;
  }
  iota_iterator& operator--()
  {
    --i;
    return *this;
  }
  iota_iterator operator--(int)
  {
    iota_iterator t(*this);
    --*this;
    return t;
  }
  iota_iterator  operator+(difference_type d) const { return {static_cast<T>(static_cast<difference_type>(i) + d)}; }
  iota_iterator  operator-(difference_type d) const { return {static_cast<T>(static_cast<difference_type>(i) - d)}; }
  iota_iterator& operator+=(difference_type d)
  {
    i = static_cast<T>(static_cast<difference_type>(i) + d);
    return *this;
  }
  iota_iterator& operator-=(difference_type d)
  {
    i = static_cast<T>(static_cast<difference_type>(i) - d);
    return *this;
  }
  bool                 operator==(const iota_iterator& other) const { return i == other.i; }
  bool                 operator!=(const iota_iterator& other) const { return i != other.i; }
  bool                 operator<(const iota_iterator& other) const { return i < other.i; }
  bool                 operator<=(const iota_iterator& other) const { return i <= other.i; }
  bool                 operator>(const iota_iterator& other) const { return i > other.i; }
  bool                 operator>=(const iota_iterator& other) const { return i >= other.i; }
  difference_type      operator-(const iota_iterator& other) const { return i - other.i; }
  friend iota_iterator operator+(difference_type n, const iota_iterator& it) { return it + n; }
  T operator[](difference_type d) const { return static_cast<T>(static_cast<difference_type>(i) + d); }

private:
  T i = 0;
};

template <class T>
struct iota_view
{
  using iterator = iota_iterator<T>;
  iota_view(T begin, T end)
      : m_begin(begin)
      , m_end(end)
  {
  }
  iterator begin() const { return {m_begin}; };
  iterator end() const { return {m_end}; };

private:
  T m_begin, m_end;
};

// This `typename F` makes the C++ compiler generate a different
// parallel_batches implementation per workload. That's important because it
// allows the compiler to optimize based on the value of `fn` -- otherwise,
// every call to parallel_batches will go through the same implementation,
// which will use an indirect function call (preventing auto-vectorization).

template <uint64_t BATCHSIZE = 512, typename F>
inline void parallel_batches(uint64_t numItems, F&& fn, uint32_t numThreads = 0)
{
  // Check to see if we should perform the single-threaded fallback.
  if(numItems <= BATCHSIZE || numThreads == 1)
  {
    for(uint64_t i = 0; i < numItems; i++)
    {
      fn(i);
    }
  }
  else
  {
    // Manually unroll the loop.
    const uint64_t numBatches = (numItems + BATCHSIZE - 1) / BATCHSIZE;
    auto           worker     = [&numItems, &fn](const uint64_t batchIndex) {
      const uint64_t start          = BATCHSIZE * batchIndex;
      const uint64_t itemsRemaining = numItems - start;
      if(itemsRemaining >= BATCHSIZE)
      {
        // We have BATCHSIZE items to process.
        // You might think using a range of i = 0 ... BATCHSIZE - 1 would be the
        // easiest to autovectorize. But surprisingly, MSVC with /Qvec-report
        // produces error 500 on this loop unless you express a range like this.
        for(uint64_t i = start; i < start + BATCHSIZE; i++)
        {
          fn(i);
        }
      }
      else
      {
        // Variable-length loop
        for(uint64_t i = start; i < numItems; i++)
        {
          fn(i);
        }
      }
    };

    iota_view<uint64_t> batches(0, numBatches);
    std::for_each(std::execution::par_unseq, batches.begin(), batches.end(), worker);
  }
}

// Returns the thread pool; creates it if it hasn't been created yet.
// Safe to call from multiple threads, but for performance reasons, should
// only be called if you know you'll run on multiple threads.
BS::thread_pool& get_thread_pool();

// Like parallel_batches, but provides the thread index.
template <uint64_t BATCHSIZE = 512, typename F>
inline void parallel_batches_indexed(uint64_t numItems, F&& fn, uint32_t numThreads = 0)
{
  // This implementation uses BS::thread_pool, because std::execution::par*
  // might not run on a constant number of threads.

  // Check to see if we should perform the single-threaded fallback.
  // This last check detects if this function is being called from within
  // another parallel launch that used the same thread pool. If so, since it is
  // possible for the BS thread pool to deadlock if work is recursively
  // submitted to it, we switch to a serial call.
  if(numItems <= BATCHSIZE || numThreads == 1 || BS::this_thread::get_index().has_value())
  {
    for(uint64_t i = 0; i < numItems; i++)
    {
      fn(i, 0);
    }
  }
  else
  {
    const uint64_t numBatches = (numItems + BATCHSIZE - 1) / BATCHSIZE;
    auto           worker     = [&numItems, &fn](const uint64_t batchIndex) {
      const uint64_t start          = BATCHSIZE * batchIndex;
      const uint64_t itemsRemaining = numItems - start;
      // get_index() cannot return `nullopt`, since we're guaranteed to be
      // inside a thread pool thread here.
      const uint32_t threadIndex = static_cast<uint32_t>(BS::this_thread::get_index().value());
      if(itemsRemaining >= BATCHSIZE)
      {
        for(uint64_t i = start; i < start + BATCHSIZE; i++)
        {
          fn(i, threadIndex);
        }
      }
      else
      {
        for(uint64_t i = start; i < numItems; i++)
        {
          fn(i, threadIndex);
        }
      }
    };

    // When BATCHSIZE is 1 (which indicates large, typically variable-length
    // work items), it's a good idea to make the thread pool generate 1 block
    // per work item, instead of 1 block per thread. This allows threads that
    // finished their work early to pick up other blocks. We don't always do
    // this because it allocates more memory in the multi_future and incurs
    // more synchronization overhead; the ideal `numBlocks` is probably
    // somewhere between these limiting cases.
    const uint64_t         numBlocks  = ((BATCHSIZE == 1) ? numBatches : 0);
    BS::thread_pool&       threadPool = get_thread_pool();
    BS::multi_future<void> future     = threadPool.submit_loop<uint64_t>(0, numBatches, worker, numBlocks);
    future.wait();
  }
}

template <uint64_t BATCHSIZE = 512, typename F>
inline void parallel_ranges(uint64_t numItems, F&& fn, uint32_t numThreads = 0)
{
  // Almost the same as the implementation of parallel_batches_indexed.
  if(numItems <= BATCHSIZE || numThreads == 1 || BS::this_thread::get_index().has_value())
  {
    fn(0, numItems, 0);
  }
  else
  {
    const uint64_t numBatches = (numItems + BATCHSIZE - 1) / BATCHSIZE;
    const auto     worker     = [&numItems, &fn](const uint64_t batchIndex) {
      const uint64_t start          = BATCHSIZE * batchIndex;
      const uint64_t itemsRemaining = numItems - start;
      // get_index() cannot return `nullopt`, since we're guaranteed to be
      // inside a thread pool thread here.
      const uint32_t threadIndex = static_cast<uint32_t>(BS::this_thread::get_index().value());
      if(itemsRemaining >= BATCHSIZE)
      {
        fn(start, start + BATCHSIZE, threadIndex);
      }
      else
      {
        fn(start, numItems, threadIndex);
      }
    };

    const uint64_t         numBlocks  = ((BATCHSIZE == 1) ? numBatches : 0);
    BS::thread_pool&       threadPool = get_thread_pool();
    BS::multi_future<void> future     = threadPool.submit_loop<uint64_t>(0, numBatches, worker, numBlocks);
    future.wait();
  }
}

}  // namespace nvh
