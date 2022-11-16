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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2022 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef GGX_GLSL
#define GGX_GLSL 1


#include "constants.glsl"


//-----------------------------------------------------------------------
// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
//-----------------------------------------------------------------------
vec3 fShlick(vec3 f0, vec3 f90, float vDotH)
{
  return f0 + (f90 - f0) * pow(clamp(vec3(1.0F) - vDotH, vec3(0.0F), vec3(1.0F)), vec3(5.0F));
}

float fShlick(float f0, float f90, float vDotH)
{
  return f0 + (f90 - f0) * pow(clamp(1.0 - vDotH, 0.0F, 1.0F), 5.0F);
}

//-----------------------------------------------------------------------
// Smith Joint GGX
// Note: Vis = G / (4 * nDotL * nDotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
//-----------------------------------------------------------------------
float vGgx(float nDotL, float nDotV, float alphaRoughness)
{
  float alphaRoughnessSq = alphaRoughness * alphaRoughness;

  float ggxV = nDotL * sqrt(nDotV * nDotV * (1.0F - alphaRoughnessSq) + alphaRoughnessSq);
  float ggxL = nDotV * sqrt(nDotL * nDotL * (1.0F - alphaRoughnessSq) + alphaRoughnessSq);

  float ggx = ggxV + ggxL;
  if(ggx > 0.0F)
  {
    return 0.5F / ggx;
  }
  return 0.0F;
}

//-----------------------------------------------------------------------
// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
//-----------------------------------------------------------------------
float dGgx(float nDotH, float alphaRoughness)
{
  float alphaRoughnessSq = alphaRoughness * alphaRoughness;
  float f                = (nDotH * nDotH) * (alphaRoughnessSq - 1.0F) + 1.0F;
  return alphaRoughnessSq / (M_PI * f * f);
}

//-----------------------------------------------------------------------
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
//-----------------------------------------------------------------------
vec3 brdfLambertian(vec3 f0, vec3 f90, vec3 diffuseColor, float vDotH)
{
  // see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
  return (1.0F - fShlick(f0, f90, vDotH)) * (diffuseColor / M_PI);
}

vec3 brdfLambertian(vec3 diffuseColor, float metallic)
{
  return (1.0F - metallic) * (diffuseColor / M_PI);
}

//-----------------------------------------------------------------------
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
//-----------------------------------------------------------------------
vec3 brdfSpecularGGX(vec3 f0, vec3 f90, float alphaRoughness, float vDotH, float nDotL, float nDotV, float nDotH)
{
  vec3  f   = fShlick(f0, f90, vDotH);
  float vis = vGgx(nDotL, nDotV, alphaRoughness);
  float d   = dGgx(nDotH, alphaRoughness);

  return f * vis * d;
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
vec3 ggxSampling(float specularAlpha, float r1, float r2)
{
  float phi = r1 * 2.0F * M_PI;

  float cos_theta = sqrt((1.0F - r2) / (1.0F + (specularAlpha * specularAlpha - 1.0F) * r2));
  float sin_theta = clamp(sqrt(1.0F - (cos_theta * cos_theta)), 0.0F, 1.0F);
  float sin_phi   = sin(phi);
  float cos_phi   = cos(phi);

  return vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
}


#endif  // GGX_H