/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2024, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVVKHL_BSDF_FUNCTIONS_H
#define NVVKHL_BSDF_FUNCTIONS_H 1

#include "func.h"            // cosineSampleHemisphere
#include "ggx.h"             // brdfLambertian, ..
#include "bsdf_structs.h"    // Bsdf*,
#include "pbr_mat_struct.h"  // PbrMaterial

#ifdef __cplusplus
#define OUT_TYPE(T) T&
#define INOUT_TYPE(T) T&
#define ARRAY_TYPE(T, N, name) std::array<T, N> name
#else
#define OUT_TYPE(T) out T
#define INOUT_TYPE(T) inout T
#define ARRAY_TYPE(T, N, name) T name[N]
#endif

// Define a value to represent an infinite impulse or singularity
#define DIRAC -1.0


/** @DOC_START
# Function absorptionCoefficient
>  Compute the absorption coefficient of the material
@DOC_END */
vec3 absorptionCoefficient(PbrMaterial mat)
{
  float tmp1 = mat.attenuationDistance;
  return tmp1 <= 0.0F ? vec3(0.0F, 0.0F, 0.0F) :
                        -vec3(log(mat.attenuationColor.x), log(mat.attenuationColor.y), log(mat.attenuationColor.z)) / tmp1;
}

#define LOBE_DIFFUSE_REFLECTION 0
#define LOBE_SPECULAR_TRANSMISSION 1
#define LOBE_SPECULAR_REFLECTION 2
#define LOBE_METAL_REFLECTION 3
#define LOBE_SHEEN_REFLECTION 4
#define LOBE_CLEARCOAT_REFLECTION 5
#define LOBE_COUNT 6

// The Fresnel factor depends on the cosine between the view vector k1 and the
// half vector, H = normalize(k1 + k2). But during sampling, we don't have k2
// until we sample a microfacet. So instead, we approximate it.
// For a mirror surface, we have H == N. For a perfectly diffuse surface, k2
// is sampled in a cosine distribution around N, so H ~ normalize(k1 + N).
// We ad-hoc interpolate between them using the roughness.
float fresnelCosineApproximation(float VdotN, float roughness)
{
  return mix(VdotN, sqrt(0.5F + 0.5F * VdotN), sqrt(roughness));
}

// Calculate the weights of the individual lobes inside the standard PBR material.
ARRAY_TYPE(float, LOBE_COUNT, ) computeLobeWeights(PbrMaterial mat, float VdotN, INOUT_TYPE(vec3) tint)
{
  float frCoat = 0.0F;
  if(mat.clearcoat > 0.0f)
  {
    float frCosineClearcoat = fresnelCosineApproximation(VdotN, mat.clearcoatRoughness);
    frCoat                  = mat.clearcoat * ior_fresnel(1.5f / mat.ior1, frCosineClearcoat);
  }

  // This Fresnel value defines the weighting between dielectric specular reflection and
  // the base dielectric BXDFs (diffuse reflection and specular transmission).
  float frDielectric = 0;
  if(mat.specular > 0)
  {
    float frCosineDielectric = fresnelCosineApproximation(VdotN, (mat.roughness.x + mat.roughness.y) * 0.5F);
    frDielectric             = ior_fresnel(mat.ior2 / mat.ior1, frCosineDielectric);
    frDielectric *= mat.specular;
  }

  // Estimate the iridescence Fresnel factor with the angle to the normal, and
  // blend it in. That's good enough for specular reflections.
  if(mat.iridescence > 0.0f)
  {
    // When there is iridescence enabled, use the maximum of the estimated iridescence factor. (Estimated with VdotN, no half-vector H here.)
    // With the thinfilm decision this handles the mix between non-iridescence and iridescence strength automatically.
    vec3 frIridescence = thin_film_factor(mat.iridescenceThickness, mat.iridescenceIor, mat.ior2, mat.ior1, VdotN);
    frDielectric = mix(frDielectric, max(frIridescence.x, max(frIridescence.y, frIridescence.z)), mat.iridescence);
    // Modulate the dielectric base lobe (diffuse, transmission) colors by the inverse of the iridescence factor,
    // though use the maximum component to not actually generate inverse colors.
    tint = mix_rgb(tint, mat.specularColor, frIridescence * mat.iridescence);
  }

  float sheen = 0.0f;
  if((mat.sheenColor.r != 0.0F || mat.sheenColor.g != 0.0F || mat.sheenColor.b != 0.0F))
  {
    sheen = pow(1.0f - abs(VdotN), mat.sheenRoughness);  // * luminance(mat.sheenColor);
    sheen = sheen / (sheen + 0.5F);
  }

  /*
  Lobe weights:

    - Clearcoat       : clearcoat * schlickFresnel(1.5, VdotN)
    - Sheen           : sheen
    - Metal           : metallic
    - Specular        : specular * schlickFresnel(ior, VdotN)
    - Transmission    : transmission
    - Diffuse         : 1.0 - clearcoat - sheen - metallic - specular - transmission
  */

  ARRAY_TYPE(float, LOBE_COUNT, weightLobe);
  weightLobe[LOBE_CLEARCOAT_REFLECTION]  = 0;
  weightLobe[LOBE_SHEEN_REFLECTION]      = 0;
  weightLobe[LOBE_METAL_REFLECTION]      = 0;
  weightLobe[LOBE_SPECULAR_REFLECTION]   = 0;
  weightLobe[LOBE_SPECULAR_TRANSMISSION] = 0;
  weightLobe[LOBE_DIFFUSE_REFLECTION]    = 0;

  float weightBase = 1.0F;

  weightLobe[LOBE_CLEARCOAT_REFLECTION] = frCoat;  // BRDF clearcoat reflection (GGX-Smith)
  weightBase *= 1.0f - frCoat;

  weightLobe[LOBE_SHEEN_REFLECTION] = weightBase * sheen;  // BRDF sheen reflection (Lambert)
  weightBase *= 1.0f - sheen;

  weightLobe[LOBE_METAL_REFLECTION] = weightBase * mat.metallic;  // BRDF metal (GGX-Smith)
  weightBase *= 1.0f - mat.metallic;

  weightLobe[LOBE_SPECULAR_REFLECTION] = weightBase * frDielectric;  // BRDF dielectric specular reflection (GGX-Smith)
  weightBase *= 1.0f - frDielectric;

  weightLobe[LOBE_SPECULAR_TRANSMISSION] = weightBase * mat.transmission;  // BTDF dielectric specular transmission (GGX-Smith)
  weightLobe[LOBE_DIFFUSE_REFLECTION] = weightBase * (1.0f - mat.transmission);  // BRDF diffuse dielectric reflection (Lambert). // PERF Currently not referenced below.

  return weightLobe;
}

// Calculate the weights of the individual lobes inside the standard PBR material
// and randomly select one.
int findLobe(PbrMaterial mat, float VdotN, float rndVal, INOUT_TYPE(vec3) tint)
{
  ARRAY_TYPE(float, LOBE_COUNT, weightLobe) = computeLobeWeights(mat, VdotN, tint);

  int   lobe   = LOBE_COUNT;
  float weight = 0.0f;
  while(--lobe > 0)  // Stops when lobe reaches 0!
  {
    weight += weightLobe[lobe];
    if(rndVal < weight)
    {
      break;  // Sample and evaluate this lobe!
    }
  }

  return lobe;  // Last one is the diffuse reflection
}

void brdf_diffuse_eval(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat, vec3 tint)
{
  // If the incoming light direction is on the backside, there is nothing to evaluate for a BRDF.
  // Note that the state normals have been flipped to the ray side by the caller.
  // Include edge-on (== 0.0f) as "no light" case.
  if(dot(data.k2, mat.Ng) <= 0.0f)  // if (backside)
  {
    return;  // absorb
  }

  data.pdf = max(0.0f, dot(data.k2, mat.N) * M_1_PI);

  // For a white Lambert material, the bxdf components match the evaluation pdf. (See MDL_renderer.)
  data.bsdf_diffuse = tint * data.pdf;
}

void brdf_diffuse_eval(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat)
{
  brdf_diffuse_eval(data, mat, mat.baseColor);
}

void brdf_diffuse_sample(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat, vec3 tint)
{
  data.k2  = cosineSampleHemisphere(data.xi.x, data.xi.y);
  data.k2  = mat.T * data.k2.x + mat.B * data.k2.y + mat.N * data.k2.z;
  data.k2  = normalize(data.k2);
  data.pdf = dot(data.k2, mat.N) * M_1_PI;

  data.bsdf_over_pdf = tint;  // bsdf * dot(wi, normal) / pdf;
  data.event_type    = (0.0f < dot(data.k2, mat.Ng)) ? BSDF_EVENT_DIFFUSE_REFLECTION : BSDF_EVENT_ABSORB;
}

void brdf_diffuse_sample(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat)
{
  brdf_diffuse_sample(data, mat, mat.baseColor);
}

void brdf_ggx_smith_eval(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat, const int lobe, vec3 tint)
{
  // BRDF or BTDF eval?
  // If the incoming light direction is on the backface.
  // Include edge-on (== 0.0f) as "no light" case.
  const bool backside = (dot(data.k2, mat.Ng) <= 0.0f);
  // Nothing to evaluate for given directions?
  if(backside && false)  // && scatter_reflect
  {
    data.pdf         = 0.0f;
    data.bsdf_glossy = vec3(0.0f);
    return;
  }

  const float nk1 = abs(dot(data.k1, mat.N));
  const float nk2 = abs(dot(data.k2, mat.N));

  // compute_half_vector() for scatter_reflect.
  const vec3 h = normalize(data.k1 + data.k2);

  // Invalid for reflection / refraction?
  const float nh  = dot(mat.N, h);
  const float k1h = dot(data.k1, h);
  const float k2h = dot(data.k2, h);

  // nk1 and nh must not be 0.0f or state.pdf == NaN.
  if(nk1 <= 0.0f || nh <= 0.0f || k1h < 0.0f || k2h < 0.0f)
  {
    data.pdf         = 0.0f;
    data.bsdf_glossy = vec3(0.0f);
    return;
  }

  // Compute BSDF and pdf.
  const vec3 h0 = vec3(dot(mat.T, h), dot(mat.B, h), nh);

  data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0);
  float G1;
  float G2;

  float G12;
  G12 = ggx_smith_shadow_mask(G1, G2, vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1),
                              vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), nk2), mat.roughness);

  data.pdf *= 0.25f / (nk1 * nh);

  vec3 bsdf = vec3(G12 * data.pdf);

  data.pdf *= G1;

  if(mat.iridescence > 0.0f)
  {
    const vec3 factor = thin_film_factor(mat.iridescenceThickness, mat.iridescenceIor, mat.ior2, mat.ior1, k1h);

    switch(lobe)
    {
      case LOBE_SPECULAR_REFLECTION:
        tint *= mix(vec3(1.f), factor, mat.iridescence);
        break;

      case LOBE_METAL_REFLECTION:
        tint = mix_rgb(tint, mat.specularColor, factor * mat.iridescence);
        break;
    }
  }

  // eval output: (glossy part of the) bsdf * dot(k2, normal)
  data.bsdf_glossy = bsdf * tint;
}

void brdf_ggx_smith_sample(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat, const int lobe, vec3 tint)
{
  // When the sampling returns eventType = BSDF_EVENT_ABSORB, the path ends inside the ray generation program.
  // Make sure the returned values are valid numbers when manipulating the PRD.
  data.bsdf_over_pdf = vec3(0.0f);
  data.pdf           = 0.0f;

  // Transform to local coordinate system
  const float nk1 = dot(data.k1, mat.N);
  if(nk1 <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }
  const vec3 k10 = vec3(dot(data.k1, mat.T), dot(data.k1, mat.B), nk1);

  // Sample half-vector, microfacet normal.
  const vec3 h0 = hvd_ggx_sample_vndf(k10, mat.roughness, data.xi.xy);
  if(h0.z == 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // Transform to world
  const vec3  h  = h0.x * mat.T + h0.y * mat.B + h0.z * mat.N;
  const float kh = dot(data.k1, h);

  if(kh <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // BRDF: reflect
  data.k2 = (2.0f * kh) * h - data.k1;

  // Check if the resulting direction is on the correct side of the actual geometry
  const float gnk2 = dot(data.k2, mat.Ng);  // * ((data.typeEvent == BSDF_EVENT_GLOSSY_REFLECTION) ? 1.0f : -1.0f);

  if(gnk2 <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  const float nk2 = abs(dot(data.k2, mat.N));
  const float k2h = abs(dot(data.k2, h));

  float G1;
  float G2;

  float G12 = ggx_smith_shadow_mask(G1, G2, k10, vec3(dot(data.k2, mat.T), dot(data.k2, mat.B), nk2), mat.roughness);

  if(G12 <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  data.bsdf_over_pdf = vec3(G2);
  data.event_type    = BSDF_EVENT_GLOSSY_REFLECTION;

  // Compute pdf
  data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0) * G1;
  data.pdf *= 0.25f / (nk1 * h0.z);

  if(mat.iridescence > 0.0f)
  {
    const vec3 factor = thin_film_factor(mat.iridescenceThickness, mat.iridescenceIor, mat.ior2, mat.ior1, kh);

    switch(lobe)
    {
      case LOBE_SPECULAR_REFLECTION:
        tint *= mix(vec3(1.f), factor, mat.iridescence);
        break;

      case LOBE_METAL_REFLECTION:
        tint = mix_rgb(tint, mat.specularColor, factor * mat.iridescence);
        break;
    }
  }

  data.bsdf_over_pdf *= tint;
}

/*
 * In rare cases (mainly dispersion), we need to get an additional random
 * number. This function takes a random number and applies the hash function
 * from pcg_output_rxs_m_xs_32_32 to it.
 * Its quality hasn't been tested and it's not very principled, but it's better
 * than using a random number that's correlated with material sampling.
*/
float rerandomize(float v)
{
  uint word = floatBitsToUint(v);
  word      = ((word >> ((word >> 28) + 4)) ^ word) * 277803737U;
  word      = (word >> 22) ^ word;
  return float(word) / uintBitsToFloat(0x4f800000U);
}

/**
 * Calculates the IOR at a given wavelength (in nanometers) given the base IOR
 * and glTF dispersion factor.
 * This function calculates the IOR for red, green, and blue wavelengths by spreading the base IOR
 * according to the dispersion factor. The dispersion factor determines how much the IOR varies
 * across different wavelengths. See: 
*/
float compute_dispersed_ior(float base_ior, float dispersion, float wavelength_nm)
{
  // Since the glTF extension stores 20 / V_d
  float abbeNumber = 20.0F / dispersion;
  // Last equation of https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_dispersion
  return max(base_ior + (base_ior - 1.0F) * (523655.0F / (wavelength_nm * wavelength_nm) - 1.5168F) / abbeNumber, 1.0F);
}

/**
 * Given a wavelength of light, returns an approximation to the linear RGB
 * color of a D65 illuminant (sRGB whitepoint) sampled at a wavelength of `x`
 * nanometers.
 *
 * This is normalized so that sum(dispersion_tint(i), {i, 350, 750}) == vec3(1.),
 * which means that the values it returns are usually low. You'll need to multiply
 * by an appropriate normalization factor if you're randomly sampling it.
 *
 * Additionally, note that this sometimes returns negative values. This is
 * because perfect wavelengths of light are outside the sRGB gamut.
 */
#define WAVELENGTH_MIN 399.43862850585765F
#define WAVELENGTH_MAX 668.6617899434457F
vec3 dispersion_tint(float x)
{
  // This uses a piecewise-linear approximation generated using the CIE 2015
  // 2-degree standard observer times the D65 illuminant, minimizing the L2
  // norm, then normalized to have an integral of 1.
  vec3 rgb = vec3(0.);
  if(399.43862850585765 < x)
  {
    if(x < 435.3450352446586)
      rgb.r = 2.6268757476158464e-05 * x + -0.010492756458829732;
    else if(x < 452.7741480943567)
      rgb.r = -5.383671438883332e-05 * x + 0.024380763013525125;
    else if(x < 550.5919453498173)
      rgb.r = 1.2536207000814165e-07 * x + -5.187018452935683e-05;
    else if(x < 600.8694441891222)
      rgb.r = 0.00032842519537482 * x + -0.18081111406184644;
    else if(x < 668.6617899434457)
      rgb.r = -0.0002438262071743009 * x + 0.16303726812428945;
  }
  if(467.41924217251835 < x)
  {
    if(x < 532.3927928594046)
      rgb.g = 0.00020126149345609334 * x + -0.0940734947497564;
    else if(x < 552.5312202450474)
      rgb.g = -4.3718474429905034e-05 * x + 0.03635207454767751;
    else if(x < 605.5304635656746)
      rgb.g = -0.00023012125757884968 * x + 0.13934543177803685;
  }
  if(400.68666327204835 < x)
  {
    if(x < 447.59688835108466)
      rgb.b = 0.00042519082480799777 * x + -0.1703682928462067;
    else if(x < 501.2110070697423)
      rgb.b = -0.00037202508909921054 * x + 0.18646306956262593;
  }
  return rgb;
}


void btdf_ggx_smith_eval(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat, vec3 tint)
{
  bool isThinWalled = (mat.thickness == 0.0f);

  vec2 ior = vec2(mat.ior1, mat.ior2);
  if(mat.dispersion > 0.0f)
  {
    // Randomly choose a wavelength; for now, uniformly choose from 399-669 nm.
    float wavelength = mix(WAVELENGTH_MIN, WAVELENGTH_MAX, rerandomize(data.xi.z));
    ior.x            = compute_dispersed_ior(ior.x, mat.dispersion, wavelength);
    tint *= (WAVELENGTH_MAX - WAVELENGTH_MIN) * dispersion_tint(wavelength);
  }

  const float nk1 = abs(dot(data.k1, mat.N));
  const float nk2 = abs(dot(data.k2, mat.N));

  // BRDF or BTDF eval?
  // If the incoming light direction is on the backface.
  // Do NOT include edge-on (== 0.0f) as backside here to take the reflection path.
  const bool backside = (dot(data.k2, mat.Ng) < 0.0f);

  const vec3 h = compute_half_vector(data.k1, data.k2, mat.N, ior, nk2, backside, isThinWalled);

  // Invalid for reflection / refraction?
  const float nh  = dot(mat.N, h);
  const float k1h = dot(data.k1, h);
  const float k2h = dot(data.k2, h) * (backside ? -1.0f : 1.0f);

  // nk1 and nh must not be 0.0f or state.pdf == NaN.
  if(nk1 <= 0.0f || nh <= 0.0f || k1h < 0.0f || k2h < 0.0f)
  {
    data.pdf         = 0.0f;  // absorb
    data.bsdf_glossy = vec3(0.0f);
    return;
  }

  float fr;

  if(!backside)
  {
    // For scatter_transmit: Only allow TIR with BRDF eval.
    if(!isTIR(ior, k1h))
    {
      data.pdf         = 0.0f;  // absorb
      data.bsdf_glossy = vec3(0.0f);
      return;
    }
    else
    {
      fr = 1.0f;
    }
  }
  else
  {
    fr = 0.0f;
  }

  // Compute BSDF and pdf
  const vec3 h0 = vec3(dot(mat.T, h), dot(mat.B, h), nh);
  data.pdf      = hvd_ggx_eval(1.0f / mat.roughness, h0);

  float G1;
  float G2;
  float G12 = ggx_smith_shadow_mask(G1, G2, vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1),
                                    vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), nk2), mat.roughness);

  if(!isThinWalled && backside)  // Refraction?
  {
    // Refraction pdf and BTDF
    const float tmp = k1h * ior.x - k2h * ior.y;

    data.pdf *= k1h * k2h / (nk1 * nh * tmp * tmp);
  }
  else
  {
    // Reflection pdf and BRDF (and pseudo-BTDF for thin-walled)
    data.pdf *= 0.25f / (nk1 * nh);
  }

  const float prob = (backside) ? 1.0f - fr : fr;

  const vec3 bsdf = vec3(prob * G12 * data.pdf);

  data.pdf *= prob * G1;

  // eval output: (glossy part of the) bsdf * dot(k2, normal)
  data.bsdf_glossy = bsdf * tint;
}


void btdf_ggx_smith_sample(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat, vec3 tint)
{
  bool isThinWalled = (mat.thickness == 0.0f);

  // When the sampling returns eventType = BSDF_EVENT_ABSORB, the path ends inside the ray generation program.
  // Make sure the returned values are valid numbers when manipulating the PRD.
  data.bsdf_over_pdf = vec3(0.0f);
  data.pdf           = 0.0f;

  vec2 ior = vec2(mat.ior1, mat.ior2);
  if(mat.dispersion > 0.0f)
  {
    // Randomly choose a wavelength; for now, uniformly choose from 361-716 nm.
    float wavelength = mix(WAVELENGTH_MIN, WAVELENGTH_MAX, rerandomize(data.xi.z));
    ior.x            = compute_dispersed_ior(ior.x, mat.dispersion, wavelength);
    tint *= (WAVELENGTH_MAX - WAVELENGTH_MIN) * dispersion_tint(wavelength);
  }

  const float nk1 = abs(dot(data.k1, mat.N));

  const vec3 k10 = vec3(dot(data.k1, mat.T), dot(data.k1, mat.B), nk1);

  // Sample half-vector, microfacet normal.
  const vec3 h0 = hvd_ggx_sample_vndf(k10, mat.roughness, data.xi.xy);

  if(abs(h0.z) == 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // Transform to world
  const vec3 h = h0.x * mat.T + h0.y * mat.B + h0.z * mat.N;

  const float kh = dot(data.k1, h);

  if(kh <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // Case scatter_transmit
  bool tir = false;
  if(isThinWalled)  // No refraction!
  {
    // pseudo-BTDF: flip a reflected reflection direction to the back side
    data.k2 = (2.0f * kh) * h - data.k1;
    data.k2 = normalize(data.k2 - 2.0f * mat.N * dot(data.k2, mat.N));
  }
  else
  {
    // BTDF: refract
    data.k2 = refract(data.k1, h, ior.x / ior.y, kh, tir);
  }

  data.bsdf_over_pdf = vec3(1.0f);  // Was: (vec3(1.0f) - fr) / prob; // PERF Always white with the original setup.
  data.event_type    = (tir) ? BSDF_EVENT_GLOSSY_REFLECTION : BSDF_EVENT_GLOSSY_TRANSMISSION;

  // Check if the resulting direction is on the correct side of the actual geometry
  const float gnk2 = dot(data.k2, mat.Ng) * ((data.event_type == BSDF_EVENT_GLOSSY_REFLECTION) ? 1.0f : -1.0f);

  if(gnk2 <= 0.0f || isnan(data.k2.x))
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }


  const float nk2 = abs(dot(data.k2, mat.N));
  const float k2h = abs(dot(data.k2, h));

  float G1;
  float G2;
  float G12 = ggx_smith_shadow_mask(G1, G2, k10, vec3(dot(data.k2, mat.T), dot(data.k2, mat.B), nk2), mat.roughness);

  if(G12 <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  data.bsdf_over_pdf *= G2;

  // Compute pdf
  data.pdf = hvd_ggx_eval(1.0f / mat.roughness, h0) * G1;  // * prob;


  if(!isThinWalled && (data.event_type == BSDF_EVENT_GLOSSY_TRANSMISSION))  // if (refraction)
  {
    const float tmp = kh * ior.x - k2h * ior.y;
    if(tmp > 0)
    {
      data.pdf *= kh * k2h / (nk1 * h0.z * tmp * tmp);
    }
  }
  else
  {
    data.pdf *= 0.25f / (nk1 * h0.z);
  }

  data.bsdf_over_pdf *= tint;
}

void brdf_sheen_eval(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat)
{
  // BRDF or BTDF eval?
  // If the incoming light direction is on the backface.
  // Include edge-on (== 0.0f) as "no light" case.
  const bool backside = (dot(data.k2, mat.Ng) <= 0.0f);
  // Nothing to evaluate for given directions?
  if(backside)  // && scatter_reflect
  {
    return;  // absorb
  }

  const float nk1 = abs(dot(data.k1, mat.N));
  const float nk2 = abs(dot(data.k2, mat.N));

  // compute_half_vector() for scatter_reflect.
  const vec3 h = normalize(data.k1 + data.k2);

  // Invalid for reflection / refraction?
  const float nh  = dot(mat.N, h);
  const float k1h = dot(data.k1, h);
  const float k2h = dot(data.k2, h);

  // nk1 and nh must not be 0.0f or state.pdf == NaN.
  if(nk1 <= 0.0f || nh <= 0.0f || k1h < 0.0f || k2h < 0.0f)
  {
    return;  // absorb
  }

  const float invRoughness = 1.0f / (mat.sheenRoughness * mat.sheenRoughness);  // Perceptual roughness to alpha G.

  // Compute BSDF and pdf
  const vec3 h0 = vec3(dot(mat.T, h), dot(mat.B, h), nh);

  data.pdf = hvd_sheen_eval(invRoughness, h0.z);

  float G1;
  float G2;

  const float G12 = vcavities_shadow_mask(G1, G2, h0.z, vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1), k1h,
                                          vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), nk2), k2h);
  data.pdf *= 0.25f / (nk1 * nh);

  const vec3 bsdf = vec3(G12 * data.pdf);

  data.pdf *= G1;

  // eval output: (glossy part of the) bsdf * dot(k2, normal)
  data.bsdf_glossy = bsdf * mat.sheenColor;
}

void brdf_sheen_sample(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat)
{
  // When the sampling returns eventType = BSDF_EVENT_ABSORB, the path ends inside the ray generation program.
  // Make sure the returned values are valid numbers when manipulating the PRD.
  data.bsdf_over_pdf = vec3(0.0f);
  data.pdf           = 0.0f;

  const float invRoughness = 1.0f / (mat.sheenRoughness * mat.sheenRoughness);  // Perceptual roughness to alpha G.

  const float nk1 = abs(dot(data.k1, mat.N));

  const vec3 k10 = vec3(dot(data.k1, mat.T), dot(data.k1, mat.B), nk1);

  float      xiFlip = data.xi.z;
  const vec3 h0     = flip(hvd_sheen_sample(data.xi.xy, invRoughness), k10, xiFlip);

  if(abs(h0.z) == 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // Transform to world
  const vec3 h = h0.x * mat.T + h0.y * mat.B + h0.z * mat.N;

  const float k1h = dot(data.k1, h);

  if(k1h <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  // BRDF: reflect
  data.k2            = (2.0f * k1h) * h - data.k1;
  data.bsdf_over_pdf = vec3(1.0f);  // PERF Always white with the original setup.
  data.event_type    = BSDF_EVENT_GLOSSY_REFLECTION;

  // Check if the resulting reflection direction is on the correct side of the actual geometry.
  const float gnk2 = dot(data.k2, mat.Ng);

  if(gnk2 <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  const float nk2 = abs(dot(data.k2, mat.N));
  const float k2h = abs(dot(data.k2, h));

  float G1;
  float G2;

  const float G12 = vcavities_shadow_mask(G1, G2, h0.z, k10, k1h, vec3(dot(data.k2, mat.T), dot(data.k2, mat.B), nk2), k2h);
  if(G12 <= 0.0f)
  {
    data.event_type = BSDF_EVENT_ABSORB;
    return;
  }

  data.bsdf_over_pdf *= G12 / G1;

  // Compute pdf.
  data.pdf = hvd_sheen_eval(invRoughness, h0.z) * G1;

  data.pdf *= 0.25f / (nk1 * h0.z);

  data.bsdf_over_pdf *= mat.sheenColor;
}


/** @DOC_START
# Function bsdfEvaluate
>  Evaluate the BSDF for the given material.
@DOC_END */
void bsdfEvaluate(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat)
{
  vec3  tint        = mat.baseColor;
  float VdotN       = dot(data.k1, mat.N);
  int   lobe        = findLobe(mat, VdotN, data.xi.z, tint);
  data.bsdf_diffuse = vec3(0, 0, 0);
  data.bsdf_glossy  = vec3(0, 0, 0);
  data.pdf          = 0.0;

  if(lobe == LOBE_DIFFUSE_REFLECTION)
  {
    brdf_diffuse_eval(data, mat, tint);
  }
  else if(lobe == LOBE_SPECULAR_REFLECTION)
  {
    brdf_ggx_smith_eval(data, mat, LOBE_SPECULAR_REFLECTION, mat.specularColor);
  }
  else if(lobe == LOBE_SPECULAR_TRANSMISSION)
  {
    btdf_ggx_smith_eval(data, mat, tint);
  }
  else if(lobe == LOBE_METAL_REFLECTION)
  {
    brdf_ggx_smith_eval(data, mat, LOBE_METAL_REFLECTION, mat.baseColor);
  }
  else if(lobe == LOBE_CLEARCOAT_REFLECTION)
  {
    mat.roughness   = vec2(mat.clearcoatRoughness * mat.clearcoatRoughness);
    mat.N           = mat.Nc;
    mat.iridescence = 0.0f;
    brdf_ggx_smith_eval(data, mat, LOBE_CLEARCOAT_REFLECTION, vec3(1, 1, 1));
  }
  else if(lobe == LOBE_SHEEN_REFLECTION)
  {
    brdf_sheen_eval(data, mat);
  }

  // Occlusion effect (glTF occlusion texture)
  data.bsdf_diffuse *= mat.occlusion;
  data.bsdf_glossy *= mat.occlusion;
}

/** @DOC_START
# Function bsdfSample
>  Sample the BSDF for the given material
@DOC_END */
void bsdfSample(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat)
{
  vec3  tint         = mat.baseColor;
  float VdotN        = dot(data.k1, mat.N);
  int   lobe         = findLobe(mat, VdotN, data.xi.z, tint);
  data.pdf           = 0;
  data.bsdf_over_pdf = vec3(0.0F);
  data.event_type    = BSDF_EVENT_ABSORB;

  if(lobe == LOBE_DIFFUSE_REFLECTION)
  {
    brdf_diffuse_sample(data, mat, tint);
  }
  else if(lobe == LOBE_SPECULAR_REFLECTION)
  {
    brdf_ggx_smith_sample(data, mat, LOBE_SPECULAR_REFLECTION, mat.specularColor);
    data.event_type = BSDF_EVENT_SPECULAR;
  }
  else if(lobe == LOBE_SPECULAR_TRANSMISSION)
  {
    btdf_ggx_smith_sample(data, mat, tint);
  }
  else if(lobe == LOBE_METAL_REFLECTION)
  {
    brdf_ggx_smith_sample(data, mat, LOBE_METAL_REFLECTION, mat.baseColor);
  }
  else if(lobe == LOBE_CLEARCOAT_REFLECTION)
  {
    mat.roughness   = vec2(mat.clearcoatRoughness * mat.clearcoatRoughness);
    mat.N           = mat.Nc;
    mat.B           = normalize(cross(mat.N, mat.T));  // Assumes Nc and Tc are not collinear!
    mat.T           = cross(mat.B, mat.N);
    mat.iridescence = 0.0f;
    brdf_ggx_smith_sample(data, mat, LOBE_CLEARCOAT_REFLECTION, vec3(1, 1, 1));
  }
  else if(lobe == LOBE_SHEEN_REFLECTION)
  {
    // Sheen is using the state.sheenColor and state.sheenInvRoughness values directly.
    // Only brdf_sheen_sample needs a third random sample for the v-cavities flip. Put this as argument.
    brdf_sheen_sample(data, mat);
  }

  // Avoid internal reflection
  if(data.pdf <= 0.00001F || any(isnan(data.bsdf_over_pdf)))
  {
    data.event_type = BSDF_EVENT_ABSORB;
  }
  if(isnan(data.pdf) || isinf(data.pdf))
  {
    data.pdf = DIRAC;
  }
}

//--------------------------------------------------------------------------------------------------
// Those functions are used to evaluate and sample the BSDF for a simple PBR material.
// without any additional lobes like clearcoat, sheen, etc. and without the need of random numbers.
// This is based on the metallic/roughness BRDF in Appendix B of the glTF specification.
// For one sample of pure reflection, use xi == vec2(0,0).
//--------------------------------------------------------------------------------------------------

// Returns the probability that bsdfSampleSimple samples a glossy lobe.
float bsdfSimpleGlossyProbability(float NdotV, float metallic)
{
  return mix(schlickFresnel(0.04F, 1.0F, NdotV), 1.0F, metallic);
}

void bsdfEvaluateSimple(INOUT_TYPE(BsdfEvaluateData) data, PbrMaterial mat)
{
  // Specular reflection
  vec3  H     = normalize(data.k1 + data.k2);
  float NdotV = clampedDot(mat.N, data.k1);
  float NdotL = clampedDot(mat.N, data.k2);
  float VdotH = clampedDot(data.k1, H);
  float NdotH = clampedDot(mat.N, H);

  if(NdotV == 0.0f || NdotL == 0.0f || VdotH == 0.0f || NdotH == 0.0f)
  {
    data.bsdf_diffuse = data.bsdf_glossy = vec3(0.0f);
    data.pdf                             = 0.0f;
    return;
  }

  // We combine the metallic and specular lobes into a single glossy lobe.
  // The metallic weight is     metallic *    fresnel(f0 = baseColor)
  // The specular weight is (1-metallic) *    fresnel(f0 = c_min_reflectance)
  // The diffuse weight is  (1-metallic) * (1-fresnel(f0 = c_min_reflectance)) * baseColor

  // Fresnel terms
  float c_min_reflectance = 0.04F;
  vec3  f0                = mix(vec3(c_min_reflectance), mat.baseColor, mat.metallic);
  vec3  fGlossy           = schlickFresnel(f0, vec3(1.0F), VdotH);  // Metallic + specular
  float fDiffuse          = schlickFresnel(1.0F - c_min_reflectance, 0.0F, VdotH) * (1.0F - mat.metallic);

  // Specular GGX
  vec3  localH  = vec3(dot(mat.T, H), dot(mat.B, H), NdotH);
  float d       = hvd_ggx_eval(1.0f / mat.roughness, localH);
  vec3  localK1 = vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), NdotV);
  vec3  localK2 = vec3(dot(mat.T, data.k2), dot(mat.B, data.k2), NdotL);
  float G1 = 0.0f, G2 = 0.0f;
  ggx_smith_shadow_mask(G1, G2, localK1, localK2, mat.roughness);

  float diffusePdf  = M_1_PI * NdotL;
  float specularPdf = G1 * d * 0.25f / (NdotV * NdotH);
  data.pdf          = mix(diffusePdf, specularPdf, bsdfSimpleGlossyProbability(NdotV, mat.metallic));

  data.bsdf_diffuse = mat.baseColor * fDiffuse * diffusePdf;  // Lambertian
  data.bsdf_glossy  = fGlossy * G2 * specularPdf;             // GGX-Smith
}

void bsdfSampleSimple(INOUT_TYPE(BsdfSampleData) data, PbrMaterial mat)
{
  vec3 tint          = mat.baseColor;
  data.bsdf_over_pdf = vec3(0.0F);

  float nk1 = clampedDot(mat.N, data.k1);
  if(data.xi.z <= bsdfSimpleGlossyProbability(nk1, mat.metallic))
  {
    // Glossy GGX
    data.event_type = BSDF_EVENT_GLOSSY_REFLECTION;
    // Transform to local space
    vec3 localK1    = vec3(dot(mat.T, data.k1), dot(mat.B, data.k1), nk1);
    vec3 halfVector = hvd_ggx_sample_vndf(localK1, mat.roughness, data.xi.xy);
    // Transform from local space
    halfVector = mat.T * halfVector.x + mat.B * halfVector.y + mat.N * halfVector.z;
    data.k2    = reflect(-data.k1, halfVector);
  }
  else
  {
    // Diffuse
    data.event_type = BSDF_EVENT_DIFFUSE;
    vec3 localDir   = cosineSampleHemisphere(data.xi.x, data.xi.y);
    data.k2         = mat.T * localDir.x + mat.B * localDir.y + mat.N * localDir.z;
  }

  BsdfEvaluateData evalData;
  evalData.k1 = data.k1;
  evalData.k2 = data.k2;
  bsdfEvaluateSimple(evalData, mat);
  data.pdf        = evalData.pdf;
  vec3 bsdf_total = evalData.bsdf_diffuse + evalData.bsdf_glossy;
  if(data.pdf <= 0.00001F || any(isnan(bsdf_total)))
  {
    data.bsdf_over_pdf = vec3(0.0f);
    data.event_type    = BSDF_EVENT_ABSORB;
  }
  else
  {
    data.bsdf_over_pdf = bsdf_total / data.pdf;
  }
}

// Returns the approximate average reflectance of the Simple BSDF -- that is,
// average_over_k2(f(k1, k2)) -- if GGX didn't lose energy.
// This is useful for things like the variance reduction algorithm in
// Tomasz Stachowiak's *Stochastic Screen-Space Reflections*; see also
// Ray-Tracing Gems 1, chapter 32, *Accurate Real-Time Specular Reflections
// with Radiance Caching*.
vec3 bsdfSimpleAverageReflectance(vec3 k1, PbrMaterial mat)
{
  float NdotV             = clampedDot(mat.N, k1);
  float c_min_reflectance = 0.04F;
  vec3  f0                = mix(vec3(c_min_reflectance), mat.baseColor, mat.metallic);
  // This is approximate because
  // average_over_k2(fresnel(f0, 1.0f, VdotH)) != fresnel(f0, 1.0f, NdotV).
  vec3 bsdf_glossy_average = schlickFresnel(f0, vec3(1.0F), NdotV);
  vec3 bsdf_diffuse_average = mat.baseColor * schlickFresnel(1.0F - c_min_reflectance, 0.0F, NdotV) * (1.0F - mat.metallic);
  return bsdf_glossy_average + bsdf_diffuse_average;
}

#undef OUT_TYPE
#undef INOUT_TYPE
#undef ARRAY_TYPE

#endif  // NVVKHL_BSDF_FUNCTIONS_H
