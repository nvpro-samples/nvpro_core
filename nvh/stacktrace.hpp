/*
 * Copyright (c) 2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NV_STACKTRACE_INCLUDED
#define NV_STACKTRACE_INCLUDED

#include <string>

namespace nvh {

/* @DOC_START

# function nvh::getStacktrace
> Returns a string listing the function call stack at the current point.

`numFramesToSkip` is the number of frames around the call to `getStacktrace`
to skip.

Returns "<stacktrace not supported on this system>" if there's no available
backtrace backend. On internal error, returns an empty string.

@DOC_END */
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
std::string
getStacktrace(unsigned int numFramesToSkip = 0) noexcept;

}  // namespace nvh

#endif