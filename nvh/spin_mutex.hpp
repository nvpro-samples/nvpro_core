/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NV_SPINMUTEX_INCLUDED
#define NV_SPINMUTEX_INCLUDED

#include <thread>
#include <atomic>

namespace nvh {

  // spin_mutex is a light-weight mutex relying on the processor's atomic instructions.
  // It avoids the use of operating-system specific mechanisms but requires a busy wait.
  // It is compatible with the std::lock_guard template.
  //
  // example usage:
  //
  // class MyQueue {
  //   std::vector<Data>  m_items;
  //   nvh::spin_mutex    m_mutex;
  //
  // public:
  //
  //  // thread-safe enqueue
  //   void enqueue(Data item) {
  //     std::lock_guard<nvh::spin_mutex> lock(m_mutex);
  //     m_items.push_back(item);
  //  }

  class spin_mutex
  {
  private:
    std::atomic_flag m_lock = ATOMIC_FLAG_INIT;

  public:

    void lock()
    {
      while (m_lock.test_and_set(std::memory_order_acquire)) {
        std::this_thread::yield();
      }
    }

    void unlock()
    {
      m_lock.clear(std::memory_order_release);
    }
  

  };

}

#endif
