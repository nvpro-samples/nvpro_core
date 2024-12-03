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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef DH_TONEMAMP_H
#define DH_TONEMAMP_H 1

/* @DOC_START
> Contains implementations for several local tone mappers.
@DOC_END */

#ifdef __cplusplus
namespace nvvkhl_shaders {
using mat3 = glm::mat3;
using uint = uint32_t;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
#define INLINE inline
#else
#define INLINE
#endif


const int eTonemapFilmic     = 0;
const int eTonemapUncharted2 = 1;
const int eTonemapClip       = 2;
const int eTonemapACES       = 3;
const int eTonemapAgX        = 4;
const int eTonemapKhronosPBR = 5;


/* @DOC_START
# Struct `Tonemapper`
> Tonemapper settings.
@DOC_END */
struct Tonemapper
{
  int   method; // One of the `eTonemap` values above.
  int   isActive;
  float exposure;
  float brightness;
  float contrast;
  float saturation;
  float vignette;
};

/* @DOC_START
# Struct `defaultTonemapper`
> Returns default tonemapper settings: filmic tone mapping, no additional color
> correction.
@DOC_END */
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
  return t;
}

// Bindings
const int eTonemapperInput  = 0;
const int eTonemapperOutput = 1;

/** @DOC_START
# Function `toSrgb`
> Converts a color from linear RGB to sRGB.
@DOC_END */
INLINE vec3 toSrgb(vec3 rgb)
{
  vec3 low  = rgb * 12.92f;
  vec3 high = fma(pow(rgb, vec3(1.0F / 2.4F)), vec3(1.055F), vec3(-0.055F));
  return mix(low, high, greaterThan(rgb, vec3(0.0031308F)));
}

/** @DOC_START
# Function `toLinear`
> Converts a color from sRGB to linear RGB.
@DOC_END */
INLINE vec3 toLinear(vec3 srgb)
{
  vec3 low  = srgb / 12.92F;
  vec3 high = pow((srgb + vec3(0.055F)) / 1.055F, vec3(2.4F));
  return mix(low, high, greaterThan(srgb, vec3(0.04045F)));
}

/** @DOC_START
# Function `tonemapFilmic`
> Filmic tonemapping operator by Jim Hejl and Richard Burgess-Dawson,
> approximating the Digital Fusion Cineon mode, but more saturated and with
> darker midtones. sRGB correction is built in.

http://filmicworlds.com/blog/filmic-tonemapping-operators/
@DOC_END */
INLINE vec3 tonemapFilmic(vec3 color)
{
  vec3 temp   = max(vec3(0.0F), color - vec3(0.004F));
  vec3 result = (temp * (vec3(6.2F) * temp + vec3(0.5F))) / (temp * (vec3(6.2F) * temp + vec3(1.7F)) + vec3(0.06F));
  return result;
}

/** @DOC_START
# Function `tonemapUncharted`
> Tone mapping operator from Uncharted 2 by John Hable. sRGB correction is built in.

See: http://filmicworlds.com/blog/filmic-tonemapping-operators/
@DOC_END */

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

INLINE vec3 tonemapUncharted2(vec3 color)
{
  const float W             = 11.2F;
  const float exposure_bias = 2.0F;
  color                     = tonemapUncharted2Impl(color * exposure_bias);
  vec3 white_scale          = vec3(1.0F) / tonemapUncharted2Impl(vec3(W));
  // We apply pow() here instead of calling toSrgb to match the
  // original implementation.
  return pow(color * white_scale, vec3(1.0F / 2.2F));
}

/** @DOC_START
# Function `tonemapACES`
> An approximation by Stephen Hill to the Academy Color Encoding System's
> filmic curve for displaying HDR images on LDR output devices.

From https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl,
via https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
@DOC_END */
INLINE vec3 tonemapACES(vec3 color)
{
  // Input transform
  const mat3 ACESInputMat = mat3(0.59719F, 0.07600F, 0.02840F,   // Column 1
                                 0.35458F, 0.90834F, 0.13383F,   // Column 2
                                 0.04823F, 0.01566F, 0.83777F);  // Column 3
  color                   = ACESInputMat * color;
  // RRT and ODT fit
  vec3 a = color * (color + vec3(0.0245786F)) - vec3(0.000090537F);
  vec3 b = color * (vec3(0.983729F) * color + vec3(0.4329510F)) + vec3(0.238081F);
  color  = a / b;  // Always OK because of the large constant term in b's polynomial
  // Output transform
  const mat3 ACESOutputMat = mat3(1.60475F, -0.10208F, -0.00327F,  //
                                  -0.53108F, 1.10813F, -0.07276F,  //
                                  -0.07367F, -0.00605F, 1.07602F);
  color                    = ACESOutputMat * color;
  return toSrgb(color);
}

/** @DOC_START
# Function `tonemapAgX`
> Benjamin Wrensch's approximation to the AgX tone mapping curve by Troy Sobotka.

From https://iolite-engine.com/blog_posts/minimal_agx_implementation
@DOC_END */
INLINE vec3 tonemapAgX(vec3 color)
{
  // Input transform
  const mat3 agx_mat = mat3(0.842479062253094F, 0.0423282422610123F, 0.0423756549057051F,  //
                            0.0784335999999992F, 0.878468636469772F, 0.0784336F,           //
                            0.0792237451477643F, 0.0791661274605434F, 0.879142973793104F);
  color              = agx_mat * color;

  // Log2 space encoding
  const float min_ev = -12.47393f;
  const float max_ev = 4.026069f;
  color              = clamp(log2(color), min_ev, max_ev);
  color              = (color - min_ev) / (max_ev - min_ev);

  // Apply 6th-order sigmoid function approximation
  vec3 v = fma(vec3(15.5F), color, vec3(-40.14F));
  v      = fma(color, v, vec3(31.96F));
  v      = fma(color, v, vec3(-6.868F));
  v      = fma(color, v, vec3(0.4298F));
  v      = fma(color, v, vec3(0.1191F));
  v      = fma(color, v, vec3(-0.0023F));

  // Output transform
  const mat3 agx_mat_inv = mat3(1.19687900512017F, -0.0528968517574562F, -0.0529716355144438F,  //
                                -0.0980208811401368F, 1.15190312990417F, -0.0980434501171241F,  //
                                -0.0990297440797205F, -0.0989611768448433F, 1.15107367264116F);
  v                      = agx_mat_inv * v;

  // Skip the pow(..., vec3(2.2)), because we want sRGB output here.
  return v;
}

/** @DOC_START
# Function `tonemapKhronosPBR`
> The Khronos PBR neutral tone mapper.

Adapted from https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
@DOC_END */
INLINE vec3 tonemapKhronosPBR(vec3 color)
{
  const float startCompression = 0.8F - 0.04F;
  const float desaturation     = 0.15F;

#ifdef __cplusplus
  float x    = glm::min(color.x, glm::min(color.y, color.z));
  float peak = glm::max(color.x, glm::max(color.y, color.z));
#else
  float x    = min(color.x, min(color.y, color.z));
  float peak = max(color.x, max(color.y, color.z));
#endif

  float offset = x < 0.08F ? x * (-6.25F * x + 1.F) : 0.04F;
  color -= offset;

  if(peak >= startCompression)
  {
    const float d       = 1.F - startCompression;
    float       newPeak = 1.F - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.F - 1.F / (desaturation * (peak - newPeak) + 1.F);
    color   = mix(color, vec3(newPeak), g);
  }
  return toSrgb(color);
}

/* @DOC_START
> Applies the given tone mapper and color grading settings to a given color.

Requires the UV coordinate so that it can apply vignetting.
@DOC_END */
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
    case eTonemapUncharted2:
      c = tonemapUncharted2(color);
      break;
    case eTonemapClip:
      c = toSrgb(color);
      break;
    case eTonemapACES:
      c = tonemapACES(color);
      break;
    case eTonemapAgX:
      c = tonemapAgX(color);
      break;
    case eTonemapKhronosPBR:
      c = tonemapKhronosPBR(color);
      break;
  }
  // contrast and clamp
  c = clamp(mix(vec3(0.5F), c, vec3(tm.contrast)), vec3(0.F), vec3(1.F));
  // brightness
  c = pow(c, vec3(1.0F / tm.brightness));
  // saturation
  vec3 i = vec3(dot(c, vec3(0.299F, 0.587F, 0.114F)));
  c      = mix(i, c, tm.saturation);
  // vignette
  vec2 center_uv = ((uv)-vec2(0.5F)) * vec2(2.0F);
  c *= 1.0F - dot(center_uv, center_uv) * tm.vignette;

  return c;
}

#ifdef __cplusplus
}  // namespace nvvkhl_shaders
#endif

#endif  // DH_TONEMAMP_H
