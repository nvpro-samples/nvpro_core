/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include "func.glsl"         // cosineSampleHemisphere
#include "ggx.glsl"          // brdfLambertian, ..
#include "bsdf_structs.h"    // Bsdf*,
#include "pbr_mat_struct.h"  // PbrMaterial

vec3 absorptionCoefficient(in PbrMaterial mat)
{
  float tmp1 = mat.attenuationDistance;
  return tmp1 <= 0.0 ? vec3(0.0) :
                       -vec3(log(mat.attenuationColor.x), log(mat.attenuationColor.y), log(mat.attenuationColor.z)) / tmp1.xxx;
}


void bsdfEvaluate(inout BsdfEvaluateData data, in PbrMaterial mat)
{
  // Initialization
  vec3  surfaceNormal = mat.normal;
  vec3  viewDir       = data.k1;
  vec3  lightDir      = data.k2;
  vec3  albedo        = mat.albedo.rgb;
  float metallic      = mat.metallic;
  float roughness     = mat.roughness;
  vec3  f0            = mat.f0;
  vec3  f90           = vec3(1.0F);

  // Specular roughness
  float alpha = roughness * roughness;

  // Compute half vector
  vec3 halfVector = normalize(viewDir + lightDir);

  // Compute various "angles" between vectors
  float NdotV = clampedDot(surfaceNormal, viewDir);
  float NdotL = clampedDot(surfaceNormal, lightDir);
  float VdotH = clampedDot(viewDir, halfVector);
  float NdotH = clampedDot(surfaceNormal, halfVector);
  float LdotH = clampedDot(lightDir, halfVector);

  // Contribution
  vec3 f_diffuse  = brdfLambertian(albedo, metallic);
  vec3 f_specular = brdfSpecularGGX(f0, f90, alpha, VdotH, NdotL, NdotV, NdotH);

  // Calculate PDF (probability density function)
  float diffuseRatio = 0.5F * (1.0F - metallic);
  float diffusePDF   = (NdotL * M_1_OVER_PI);
  float specularPDF  = distributionGGX(NdotH, alpha) * NdotH / (4.0F * LdotH);

  // Results
  data.bsdf_diffuse = f_diffuse * NdotL;
  data.bsdf_glossy  = f_specular * NdotL;
  data.pdf          = mix(specularPDF, diffusePDF, diffuseRatio);
}

void bsdfSample(inout BsdfSampleData data, in PbrMaterial mat)
{
  // Initialization
  vec3  surfaceNormal = mat.normal;
  vec3  viewDir       = data.k1;
  float roughness     = mat.roughness;
  float metallic      = mat.metallic;

  // Random numbers for importance sampling
  float r1 = data.xi.x;
  float r2 = data.xi.y;
  float r3 = data.xi.z;

  // Create tangent space
  vec3 tangent, binormal;
  orthonormalBasis(surfaceNormal, tangent, binormal);

  // Specular roughness
  float alpha = roughness * roughness;

  // Find Half vector for diffuse or glossy reflection
  float diffuseRatio = 0.5F * (1.0F - metallic);
  vec3  halfVector;
  if(r3 < diffuseRatio)
    halfVector = cosineSampleHemisphere(r1, r2);  // Diffuse
  else
    halfVector = ggxSampling(alpha, r1, r2);  // Glossy

  // Transform the half vector to the hemisphere's tangent space
  halfVector = tangent * halfVector.x + binormal * halfVector.y + surfaceNormal * halfVector.z;

  // Compute the reflection direction from the sampled half vector and view direction
  vec3 reflectVector = reflect(-viewDir, halfVector);

  // Early out: avoid internal reflection
  if(dot(surfaceNormal, reflectVector) < 0.0F)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // Evaluate the refection coefficient with this new ray direction
  BsdfEvaluateData evalData;
  evalData.ior1 = data.ior1;
  evalData.ior2 = data.ior2;
  evalData.k1   = viewDir;
  evalData.k2   = reflectVector;
  bsdfEvaluate(evalData, mat);

  // Return values
  data.bsdf_over_pdf = (evalData.bsdf_diffuse + evalData.bsdf_glossy) / evalData.pdf;
  data.pdf           = evalData.pdf;
  data.event_type    = BSDF_EVENT_GLOSSY_REFLECTION;
  data.k2            = reflectVector;

  // Avoid internal reflection
  if(data.pdf <= 0.00001)
    data.event_type = BSDF_EVENT_ABSORB;

  if(isnan(data.bsdf_over_pdf.x) || isnan(data.bsdf_over_pdf.y) || isnan(data.bsdf_over_pdf.z))
    data.event_type = BSDF_EVENT_ABSORB;


  return;
}
