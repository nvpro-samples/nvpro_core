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
vec3 fresnelSchlick(vec3 f0, vec3 f90, float VdotH)
{
  return f0 + (f90 - f0) * pow(clamp(vec3(1.0F) - VdotH, vec3(0.0F), vec3(1.0F)), vec3(5.0F));
}

float fresnelSchlick(float f0, float f90, float VdotH)
{
  return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0F, 1.0F), 5.0F);
}

//-----------------------------------------------------------------------
// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
//-----------------------------------------------------------------------
float smithJointGGX(float NdotL, float NdotV, float alphaRoughness)
{
  float alphaRoughnessSq = max(alphaRoughness * alphaRoughness, 1e-07);

  float ggxV = NdotL * sqrt(NdotV * NdotV * (1.0F - alphaRoughnessSq) + alphaRoughnessSq);
  float ggxL = NdotV * sqrt(NdotL * NdotL * (1.0F - alphaRoughnessSq) + alphaRoughnessSq);

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
float distributionGGX(float NdotH, float alphaRoughness)  // alphaRoughness    = roughness * roughness;
{
  float alphaSqr = max(alphaRoughness * alphaRoughness, 1e-07);

  float NdotHSqr = NdotH * NdotH;
  float denom    = NdotHSqr * (alphaSqr - 1.0) + 1.0;

  return alphaSqr / (M_PI * denom * denom);
}


//-----------------------------------------------------------------------
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
//-----------------------------------------------------------------------
vec3 brdfLambertian(vec3 diffuseColor, float metallic)
{
  return (1.0F - metallic) * (diffuseColor / M_PI);
}

//-----------------------------------------------------------------------
// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
//-----------------------------------------------------------------------
vec3 brdfSpecularGGX(vec3 f0, vec3 f90, float alphaRoughness, float VdotH, float NdotL, float NdotV, float nDotH)
{
  vec3  f   = fresnelSchlick(f0, f90, VdotH);
  float vis = smithJointGGX(NdotL, NdotV, alphaRoughness);  // Vis = G / (4 * NdotL * NdotV)
  float d   = distributionGGX(nDotH, alphaRoughness);

  return f * vis * d;
}


//-----------------------------------------------------------------------
// Sample the GGX distribution
// - Return the half vector
//-----------------------------------------------------------------------
vec3 ggxSampling(float alphaRoughness, float r1, float r2)
{
  float alphaSqr = max(alphaRoughness * alphaRoughness, 1e-07);

  float phi      = 2.0 * M_PI * r1;
  float cosTheta = sqrt((1.0 - r2) / (1.0 + (alphaSqr - 1.0) * r2));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}


#endif  // GGX_H