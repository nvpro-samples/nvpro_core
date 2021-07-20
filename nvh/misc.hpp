/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef NV_MISC_INCLUDED
#define NV_MISC_INCLUDED

#include <algorithm>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "nvprint.hpp"

/**
 # functions in nvh
 
 - mipMapLevels : compute number of mip maps
 - stringFormat : sprintf for std::string
 - frand : random float using rand()
 - permutation : fills uint vector with random permutation of values [0... vec.size-1]
 */

namespace nvh {

inline std::string stringFormat(const char* msg, ...)
{
  char    text[8192];
  va_list list;

  if(msg == 0)
    return std::string();

  va_start(list, msg);
  vsnprintf(text, sizeof(text), msg, list);
  va_end(list);

  return std::string(text);
}

inline float frand()
{
  return float(rand() % RAND_MAX) / float(RAND_MAX);
}

inline int mipMapLevels(int size)
{
  int num = 0;
  while(size)
  {
    num++;
    size /= 2;
  }
  return num;
}

// permutation creates a random permutation of all integer values
// 0..data.size-1 occuring once within data.

inline void permutation(std::vector<unsigned int>& data)
{
  size_t size = data.size();
  assert(size < RAND_MAX);

  for(size_t i = 0; i < size; i++)
  {
    data[i] = (unsigned int)(i);
  }

  for(size_t i = size - 1; i > 0; i--)
  {
    size_t other = rand() % (i + 1);
    std::swap(data[i], data[other]);
  }
}
}  // namespace nvh

#endif
