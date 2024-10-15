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

/* @DOC_START
Contains functions for aligning numbers to power-of-two boundaries.
@DOC_END */

#pragma once

#ifndef NVH_ALIGNMENT_HPP
#define NVH_ALIGNMENT_HPP 1

#include <stddef.h>  // for size_t

namespace nvh {
/* @DOC_START
# Function `is_aligned<integral>(x, a)`
Returns whether `x` is a multiple of `a`. `a` must be a power of two.
@DOC_END */
template <class integral>
constexpr bool is_aligned(integral x, size_t a) noexcept
{
  return (x & (integral(a) - 1)) == 0;
}

/* @DOC_START
# Function `align_up<integral>(x, a)`
Rounds `x` up to a multiple of `a`. `a` must be a power of two.
@DOC_END */
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
  return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

/* @DOC_START
# Function `align_down<integral>(x, a)`
Rounds `x` down to a multiple of `a`. `a` must be a power of two.
@DOC_END */
template <class integral>
constexpr integral align_down(integral x, size_t a) noexcept
{
  return integral(x & ~integral(a - 1));
}
}  // namespace nvh

#endif  // !NVH_ALIGNMENT_HPP
