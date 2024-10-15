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

#ifndef RANDOM_GLSL
#define RANDOM_GLSL 1

/* @DOC_START
Random number generation functions.

For even more hash functions, check out
[Jarzynski and Olano, "Hash Functions for GPU Rendering"](https://jcgt.org/published/0009/03/02/)
@DOC_END */

precision highp float;

/* @DOC_START
# Function `xxhash32`
> High-quality hash that takes 96 bits of data and outputs 32, roughly twice
> as slow as `pcg`.

You can use this to generate a seed for subsequent random number generators;
for instance, provide `uvec3(pixel.x, pixel.y, frame_number).

From https://github.com/Cyan4973/xxHash and https://www.shadertoy.com/view/XlGcRh.
@DOC_END */
uint xxhash32(uvec3 p)
{
  const uvec4 primes = uvec4(2246822519U, 3266489917U, 668265263U, 374761393U);
  uint        h32;
  h32 = p.z + primes.w + p.x * primes.y;
  h32 = primes.z * ((h32 << 17) | (h32 >> (32 - 17)));
  h32 += p.y * primes.y;
  h32 = primes.z * ((h32 << 17) | (h32 >> (32 - 17)));
  h32 = primes.x * (h32 ^ (h32 >> 15));
  h32 = primes.y * (h32 ^ (h32 >> 13));
  return h32 ^ (h32 >> 16);
}

/* @DOC_START
# Function `pcg`
> Fast, reasonably good hash that updates 32 bits of state and outputs 32 bits.

This is a version of `pcg32i_random_t` from the
[PCG random number generator library](https://www.pcg-random.org/index.html),
which updates its internal state using a linear congruential generator and
outputs a hash using `pcg_output_rxs_m_xs_32_32`, a more complex hash.

There's a section of vk_mini_path_tracer on this random number generator
[here](https://nvpro-samples.github.io/vk_mini_path_tracer/#antialiasingandpseudorandomnumbergeneration/pseudorandomnumbergenerationinglsl).
@DOC_END */
uint pcg(inout uint state)
{
  uint prev = state * 747796405u + 2891336453u;
  uint word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
  state     = prev;
  return (word >> 22u) ^ word;
}

/* @DOC_START
# Function `rand`
> Generates a random float in [0, 1], updating an RNG state.

This can be used to generate many random numbers! Here's an example:

```glsl
uint seed = xxhash32(vec3(pixel.xy, frame_number));
for(int bounce = 0; bounce < 50; bounce++)
{
  ...
  BsdfSampleData sampleData;
  sampleData.xi = vec3(rand(seed), rand(seed), rand(seed));
  ...
}
```
@DOC_END */
float rand(inout uint seed)
{
  uint r = pcg(seed);
  return float(r) * (1.F / float(0xffffffffu));
}


#endif  // RANDOM_GLSL
