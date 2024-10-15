/*
 * Copyright (c) 2014-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */
/// @DOC_SKIP

#include "NvFoundation.h"

#ifndef NVP_PLATFORM_H__
#define NVP_PLATFORM_H__

// use C++11 atomics
#include <atomic>

#define NV_BARRIER()       std::atomic_thread_fence(std::memory_order_seq_cst);

#if defined(__MSC__) || defined(_MSC_VER)
  #pragma warning(disable:4142) // redefinition of same type
  #if (_MSC_VER >= 1400)      // VC8+
    #pragma warning(disable : 4996)    // Either disable all deprecation warnings,
  #endif   // VC8+
#endif

#endif
