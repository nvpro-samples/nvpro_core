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

#include "parallel_work.hpp"

namespace nvh {

BS::thread_pool& get_thread_pool()
{
  // Marking this as static ensures it's only initialized the first time
  // execution enters this function, even if it's called from multiple threads,
  // since C++11. See
  // https://en.cppreference.com/w/cpp/language/storage_duration#Static_block_variables
  static BS::thread_pool threadPool;
  return threadPool;
}

}  // namespace nvh