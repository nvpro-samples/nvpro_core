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

/** @DOC_START
# Function absorptionCoefficient
>  Compute the absorption coefficient of the material
@DOC_END */
vec3 absorptionCoefficient(in PbrMaterial mat)
{
  float tmp1 = mat.attenuationDistance;
  return tmp1 <= 0.0F ? vec3(0.0F, 0.0F, 0.0F) :
                        -vec3(log(mat.attenuationColor.x), log(mat.attenuationColor.y), log(mat.attenuationColor.z)) / tmp1;
}

struct EvalData
{
  float pdf;
  vec3  bsdf;
};

void evalDiffuse(in BsdfEvaluateData data, in PbrMaterial mat, out EvalData eval)
{
  // Diffuse reflection
  float NdotL = clampedDot(mat.normal, data.k2);
  eval.bsdf   = brdfLambertian(mat.albedo.rgb, mat.metallic) * NdotL;
  eval.pdf    = M_1_OVER_PI;
}

void evalSpecular(in BsdfEvaluateData data, in PbrMaterial mat, out EvalData eval)
{
  // Specular reflection
  vec3 H = normalize(data.k1 + data.k2);

  float alphaRoughness = mat.roughness * mat.roughness;
  float NdotV          = clampedDot(mat.normal, data.k1);
  float NdotL          = clampedDot(mat.normal, data.k2);
  float VdotH          = clampedDot(data.k1, H);
  float NdotH          = clampedDot(mat.normal, H);
  float LdotH          = clampedDot(data.k2, H);

  vec3 f_specular = brdfSpecularGGX(mat.f0, mat.f90, alphaRoughness, VdotH, NdotL, NdotV, NdotH);
  eval.bsdf       = f_specular * NdotL;
  eval.pdf        = distributionGGX(NdotH, alphaRoughness) * NdotH / (4.0F * LdotH);
}

void evalTransmission(in BsdfEvaluateData data, in PbrMaterial mat, out EvalData eval)
{
  eval.pdf  = 0;
  eval.bsdf = vec3(0.0F, 0.0F, 0.0F);

  if(mat.transmissionFactor <= 0.0F)
    return;

  vec3 refractedDir;
  bool totalInternalRefraction = refract(data.k2, mat.normal, mat.eta, refractedDir);

  if(!totalInternalRefraction)
  {
    //eval.bsdf = mat.albedo.rgb * mat.transmissionFactor;
    eval.pdf = abs(dot(refractedDir, mat.normal));
  }
}


/** @DOC_START
# Function bsdfEvaluate
>  Evaluate the BSDF for the given material.
@DOC_END */
void bsdfEvaluate(inout BsdfEvaluateData data, in PbrMaterial mat)
{
  // Initialization
  float diffuseRatio      = 0.5F * (1.0F - mat.metallic);
  float specularRatio     = 1.0F - diffuseRatio;
  float transmissionRatio = (1.0F - mat.metallic) * mat.transmissionFactor;

  // Contribution
  EvalData f_diffuse;
  EvalData f_specular;
  EvalData f_transmission;

  evalDiffuse(data, mat, f_diffuse);
  evalSpecular(data, mat, f_specular);
  evalTransmission(data, mat, f_transmission);

  // Combine the results
  float brdfPdf = 0;
  brdfPdf += f_diffuse.pdf * diffuseRatio;
  brdfPdf += f_specular.pdf * specularRatio;
  brdfPdf = mix(brdfPdf, f_transmission.pdf, transmissionRatio);

  vec3 bsdfDiffuse = mix(f_diffuse.bsdf, f_transmission.bsdf, transmissionRatio);
  vec3 bsdfGlossy  = mix(f_specular.bsdf, f_transmission.bsdf, transmissionRatio);

  // Return results
  data.bsdf_diffuse = bsdfDiffuse;
  data.bsdf_glossy  = bsdfGlossy;
  data.pdf          = brdfPdf;
}

//-------------------------------------------------------------------------------------------------

vec3 sampleDiffuse(inout BsdfSampleData data, in PbrMaterial mat)
{
  vec3 surfaceNormal = mat.normal;
  vec3 tangent, bitangent;
  orthonormalBasis(surfaceNormal, tangent, bitangent);
  float r1              = data.xi.x;
  float r2              = data.xi.y;
  vec3  sampleDirection = cosineSampleHemisphere(r1, r2);  // Diffuse
  sampleDirection = tangent * sampleDirection.x + bitangent * sampleDirection.y + surfaceNormal * sampleDirection.z;
  data.event_type = BSDF_EVENT_DIFFUSE;
  return sampleDirection;
}

vec3 sampleSpecular(inout BsdfSampleData data, in PbrMaterial mat)
{
  vec3 surfaceNormal = mat.normal;
  vec3 tangent, bitangent;
  orthonormalBasis(surfaceNormal, tangent, bitangent);
  float alphaRoughness = mat.roughness * mat.roughness;
  float r1             = data.xi.x;
  float r2             = data.xi.y;
  vec3  halfVector     = ggxSampling(alphaRoughness, r1, r2);  // Glossy
  halfVector           = tangent * halfVector.x + bitangent * halfVector.y + surfaceNormal * halfVector.z;
  vec3 sampleDirection = reflect(-data.k1, halfVector);
  data.event_type      = BSDF_EVENT_SPECULAR;

  return sampleDirection;
}

vec3 sampleThinTransmission(in BsdfSampleData data, in PbrMaterial mat)
{
  vec3  incomingDir    = data.k1;
  float r1             = data.xi.x;
  float r2             = data.xi.y;
  float alphaRoughness = mat.roughness * mat.roughness;
  vec3  halfVector     = ggxSampling(alphaRoughness, r1, r2);
  vec3  tangent, bitangent;
  orthonormalBasis(incomingDir, tangent, bitangent);
  vec3 transformedHalfVector = tangent * halfVector.x + bitangent * halfVector.y + incomingDir * halfVector.z;
  vec3 refractedDir          = -transformedHalfVector;

  return refractedDir;
}

vec3 sampleSolidTransmission(inout BsdfSampleData data, in PbrMaterial mat, out bool refracted)
{
  vec3 surfaceNormal = mat.normal;
  if(mat.roughness > 0.0F)
  {
    vec3 tangent, bitangent;
    orthonormalBasis(surfaceNormal, tangent, bitangent);
    float alphaRoughness = mat.roughness * mat.roughness;
    float r1             = data.xi.x;
    float r2             = data.xi.y;
    vec3  halfVector     = ggxSampling(alphaRoughness, r1, r2);  // Glossy
    halfVector           = tangent * halfVector.x + bitangent * halfVector.y + surfaceNormal * halfVector.z;
    surfaceNormal        = halfVector;
  }

  vec3 refractedDir;
  refracted = refract(-data.k1, surfaceNormal, mat.eta, refractedDir);

  return refractedDir;
}

void sampleTransmission(inout BsdfSampleData data, in PbrMaterial mat)
{
  // Calculate transmission direction using Snell's law
  vec3  refractedDir;
  vec3  sampleDirection;
  bool  refracted = true;
  float r4        = data.xi.w;

  // Thin film approximation
  if(mat.thicknessFactor == 0.0F && mat.roughness > 0.0F)
  {
    refractedDir = sampleThinTransmission(data, mat);
  }
  else
  {
    refractedDir = sampleSolidTransmission(data, mat, refracted);
  }

  // Fresnel term
  float VdotH         = dot(data.k1, mat.normal);
  vec3  reflectance   = fresnelSchlick(mat.f0, mat.f90, VdotH);
  vec3  surfaceNormal = mat.normal;

  if(!refracted || r4 < luminance(reflectance))
  {
    // Total internal reflection or reflection based on Fresnel term
    sampleDirection = reflect(-data.k1, surfaceNormal);  // Reflective direction
    data.event_type = BSDF_EVENT_SPECULAR;
  }
  else
  {
    // Transmission
    sampleDirection = refractedDir;
    data.event_type = BSDF_EVENT_TRANSMISSION;
  }

  // Attenuate albedo for transmission
  vec3 bsdf = mat.albedo.rgb;  // * mat.transmissionFactor;

  // Result
  data.bsdf_over_pdf = bsdf;
  data.pdf           = abs(dot(surfaceNormal, sampleDirection));  //transmissionRatio;
  data.k2            = sampleDirection;
}


/** @DOC_START
# Function bsdfSample
>  Sample the BSDF for the given material
@DOC_END */
void bsdfSample(inout BsdfSampleData data, in PbrMaterial mat)
{
  // Random numbers for importance sampling
  float r3 = data.xi.z;

  // Initialization
  float diffuseRatio      = 0.5F * (1.0F - mat.metallic);
  float specularRatio     = 1.0F - diffuseRatio;
  float transmissionRatio = (1.0F - mat.metallic) * mat.transmissionFactor;

  // Calculate if the ray goes through
  if(r3 < transmissionRatio)
  {
    sampleTransmission(data, mat);
    return;
  }

  // Choose between diffuse and glossy reflection
  vec3 sampleDirection = vec3(0.0F, 0.0F, 0.0F);
  if(r3 < diffuseRatio)
  {
    sampleDirection = sampleDiffuse(data, mat);
  }
  else
  {
    // Specular roughness
    sampleDirection = sampleSpecular(data, mat);
  }

  // Evaluate the reflection coefficient with the new ray direction
  BsdfEvaluateData evalData;
  evalData.k1 = data.k1;
  evalData.k2 = sampleDirection;
  bsdfEvaluate(evalData, mat);

  // Return values
  data.pdf           = evalData.pdf;
  data.bsdf_over_pdf = (evalData.bsdf_diffuse + evalData.bsdf_glossy) / data.pdf;
  data.k2            = sampleDirection;

  // Avoid internal reflection
  if(data.pdf <= 0.00001F || any(isnan(data.bsdf_over_pdf)))
    data.event_type = BSDF_EVENT_ABSORB;

  return;
}
