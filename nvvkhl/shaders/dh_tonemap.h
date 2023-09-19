/*
 * Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.
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


#ifndef DH_TONEMAMP_H
#define DH_TONEMAMP_H 1

#ifdef __cplusplus
using vec3 = nvmath::vec3f;
using vec2 = nvmath::vec2f;
using uint = uint32_t;
#define INLINE inline
#else
#define INLINE
#endif


const int eTonemapFilmic    = 0;
const int eTonemapUncharted = 1;
const int eTonemapGamma     = 2;


// Tonemapper settings
struct Tonemapper
{
  int   method;
  int   isActive;
  float exposure;
  float brightness;
  float contrast;
  float saturation;
  float vignette;
  float gamma;
};

INLINE Tonemapper defaultTonemapper()
{
  Tonemapper t;
  t.method     = 0;
  t.isActive   = 1;
  t.exposure   = 1.0F;
  t.brightness = 1.0F;
  t.contrast   = 1.0F;
  t.saturation = 1.0F;
  t.vignette   = 0.0F;
  t.gamma      = 2.2F;
  return t;
}

// Bindings
const int eTonemapperInput  = 0;
const int eTonemapperOutput = 1;


// http://filmicworlds.com/blog/filmic-tonemapping-operators/
INLINE vec3 tonemapFilmic(vec3 color)
{
  vec3 temp   = max(vec3(0.0F), color - vec3(0.004F));
  vec3 result = (temp * (vec3(6.2F) * temp + vec3(0.5F))) / (temp * (vec3(6.2F) * temp + vec3(1.7F)) + vec3(0.06F));
  return result;
}

// Gamma Correction in Computer Graphics
// see https://www.teamten.com/lawrence/graphics/gamma/
INLINE vec3 gammaCorrection(vec3 color, float gamma)
{
  return pow(color, vec3(1.0F / gamma));
}

// Uncharted 2 tone map
// see: http://filmicworlds.com/blog/filmic-tonemapping-operators/
INLINE vec3 tonemapUncharted2Impl(vec3 color)
{
  const float a = 0.15F;
  const float b = 0.50F;
  const float c = 0.10F;
  const float d = 0.20F;
  const float e = 0.02F;
  const float f = 0.30F;
  return ((color * (a * color + c * b) + d * e) / (color * (a * color + b) + d * f)) - e / f;
}

INLINE vec3 tonemapUncharted(vec3 color, float gamma)
{
  const float W             = 11.2F;
  const float exposure_bias = 2.0F;
  color                     = tonemapUncharted2Impl(color * exposure_bias);
  vec3 white_scale          = vec3(1.0F) / tonemapUncharted2Impl(vec3(W));
  return gammaCorrection(color * white_scale, gamma);
}

INLINE vec3 applyTonemap(Tonemapper tm, vec3 color, vec2 uv)
{
  // Exposure
  color *= tm.exposure;
  vec3 c;
  // Tonemap
  switch(tm.method)
  {
    case eTonemapFilmic:
      c = tonemapFilmic(color);
      break;
    case eTonemapUncharted:
      c = tonemapUncharted(color, tm.gamma);
      break;
    default:
      c = gammaCorrection(color, tm.gamma);
      break;
  }
  //contrast
  c = clamp(mix(vec3(0.5F), c, vec3(tm.contrast)), vec3(0.F), vec3(1.F));
  // brighness
  c = pow(c, vec3(1.0F / tm.brightness));
  // saturation
  vec3 i = vec3(dot(c, vec3(0.299F, 0.587F, 0.114F)));
  c      = mix(i, c, tm.saturation);
  // vignette
  vec2 center_uv = ((uv)-vec2(0.5F)) * vec2(2.0F);
  c *= 1.0F - dot(center_uv, center_uv) * tm.vignette;

  return c;
}

// http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
INLINE vec3 toSrgb(vec3 rgb)
{
  vec3 s1 = sqrt(rgb);
  vec3 s2 = sqrt(s1);
  vec3 s3 = sqrt(s2);
  return vec3(0.662002687F) * s1 + vec3(0.684122060F) * s2 - vec3(0.323583601F) * s3 - vec3(0.0225411470F) * rgb;
}

INLINE vec3 toLinear(vec3 srgb)
{
  return srgb * (srgb * (srgb * vec3(0.305306011F) + vec3(0.682171111F)) + vec3(0.012522878F));
}
#endif  // DH_TONEMAMP_H
