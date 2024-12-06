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

/* @DOC_START
This file takes the incoming `GltfShadeMaterial` (material uploaded in a buffer) and
evaluates it, basically sampling the textures, and returns the struct `PbrMaterial`
which is used by the BSDF functions to evaluate and sample the material.
@DOC_END */

#ifndef MAT_EVAL_H
#define MAT_EVAL_H 1

#include "pbr_mat_struct.h"

struct MeshState
{
  vec3 N;      // Normal
  vec3 T;      // Tangent
  vec3 B;      // Bitangent
  vec3 Ng;     // Geometric normal
  vec2 tc[2];  // Texture coordinates
  bool isInside;
};


/* @DOC_START
# `MAT_EVAL_TEXTURE_ARRAY` Define
> The name of the array that contains all texture samplers.

Defaults to `texturesMap`; you can define this before including `pbr_mat_eval.h`
to use textures from a different array name.
@DOC_END */
#ifndef MAT_EVAL_TEXTURE_ARRAY
#define MAT_EVAL_TEXTURE_ARRAY texturesMap
#endif

/* @DOC_START
# `NO_TEXTURES` Define
> Define this before including `pbr_mat_eval.h` to use a color of `vec4(1.0f)`
> for everything instead of reading textures.
@DOC_END */
#ifndef NO_TEXTURES
#define USE_TEXTURES
#endif

vec4 getTexture(in GltfTextureInfo tinfo, in vec2 tc[2])
{
#ifdef USE_TEXTURES
  // KHR_texture_transform
  vec2 texCoord = vec2(vec3(tc[tinfo.texCoord], 1) * tinfo.uvTransform);
  return texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(tinfo.index)], texCoord);
#else
  return vec4(1.0F);
#endif
}

bool isTexturePresent(in GltfTextureInfo tinfo)
{
  return tinfo.index > -1;
}

/* @DOC_START
* Convert PBR specular glossiness to metallic-roughness
* @DOC_END */
vec3 convertSGToMR(vec3 diffuseColor, vec3 specularColor, float glossiness, out float metallic, out vec2 roughness)
{
  // Constants
  const float dielectricSpecular = 0.04f;  // F0 for dielectrics

  // Compute metallic factor
  float specularIntensity = max(specularColor.r, max(specularColor.g, specularColor.b));
  float isMetal           = smoothstep(dielectricSpecular + 0.01f, dielectricSpecular + 0.05f, specularIntensity);
  metallic                = isMetal;

  // Compute base color
  vec3 baseColor;
  if(metallic > 0.0f)
  {
    // Metallic: Use specular as base color
    baseColor = specularColor;
  }
  else
  {
    // Non-metallic: Correct diffuse color for energy conservation
    baseColor = diffuseColor / (1.0f - dielectricSpecular * (1.0f - metallic));
    baseColor = clamp(baseColor, 0.0f, 1.0f);  // Ensure valid color
  }

  // Compute roughness
  float r   = 1.0f - glossiness;
  roughness = vec2(r * r);

  return baseColor;
}

/* @DOC_START
# `MICROFACET_MIN_ROUGHNESS` Define
> Minimum roughness for microfacet models.

This protects microfacet code from dividing by 0, as well as from numerical
instability around roughness == 0. However, it also means even roughness-0
surfaces will be rendered with a tiny amount of roughness.

This value is ad-hoc; it could probably be lowered without issue.
@DOC_END */
#define MICROFACET_MIN_ROUGHNESS 0.0014142f

/* @DOC_START
# Function `evaluateMaterial`
> From the incoming `material` and `mesh` info, return a `PbrMaterial` struct
> for the BSDF system.
@DOC_END */
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, MeshState state)
{
  // Material Evaluated
  PbrMaterial pbrMat;

  // pbrMetallicRoughness (standard)
  if(material.usePbrSpecularGlossiness == 0)
  {
    // Base Color/Albedo may be defined from a base texture or a flat color
    vec4 baseColor = material.pbrBaseColorFactor;
    if(isTexturePresent(material.pbrBaseColorTexture))
    {
      baseColor *= getTexture(material.pbrBaseColorTexture, state.tc);
    }
    pbrMat.baseColor = baseColor.rgb;
    pbrMat.opacity   = baseColor.a;

    // Metallic-Roughness
    float roughness = material.pbrRoughnessFactor;
    float metallic  = material.pbrMetallicFactor;
    if(isTexturePresent(material.pbrMetallicRoughnessTexture))
    {
      // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
      vec4 metallicRoughnessSample = getTexture(material.pbrMetallicRoughnessTexture, state.tc);
      roughness *= metallicRoughnessSample.g;
      metallic *= metallicRoughnessSample.b;
    }
    roughness        = max(roughness, MICROFACET_MIN_ROUGHNESS);
    pbrMat.roughness = vec2(roughness * roughness);  // Square roughness for the microfacet model
    pbrMat.metallic  = clamp(metallic, 0.0F, 1.0F);
  }
  else
  {
    // KHR_materials_pbrSpecularGlossiness: deprecated but still used in many places
    vec4  diffuse    = material.pbrDiffuseFactor;
    float glossiness = material.pbrGlossinessFactor;
    vec3  specular   = material.pbrSpecularFactor;

    if(isTexturePresent(material.pbrDiffuseTexture))
    {
      diffuse *= getTexture(material.pbrDiffuseTexture, state.tc);
    }

    if(isTexturePresent(material.pbrSpecularGlossinessTexture))
    {
      vec4 specularGlossinessSample = getTexture(material.pbrSpecularGlossinessTexture, state.tc);
      specular *= specularGlossinessSample.rgb;
      glossiness *= specularGlossinessSample.a;
    }

    pbrMat.baseColor = convertSGToMR(diffuse.rgb, specular, glossiness, pbrMat.metallic, pbrMat.roughness);
    pbrMat.opacity   = diffuse.a;
  }

  // Occlusion Map
  pbrMat.occlusion = material.occlusionStrength;
  if(isTexturePresent(material.occlusionTexture))
  {
    float occlusion  = getTexture(material.occlusionTexture, state.tc).r;
    pbrMat.occlusion = 1.0 + pbrMat.occlusion * (occlusion - 1.0);
  }

  // Normal Map
  pbrMat.N                = state.N;
  pbrMat.T                = state.T;
  pbrMat.B                = state.B;
  pbrMat.Ng               = state.Ng;
  bool needsTangentUpdate = false;

  if(isTexturePresent(material.normalTexture))
  {
    vec3 normal_vector = getTexture(material.normalTexture, state.tc).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    normal_vector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0F);
    mat3 tbn = mat3(state.T, state.B, state.N);
    pbrMat.N = normalize(tbn * normal_vector);

    // Mark that we need to update T and B due to normal perturbation
    needsTangentUpdate = true;
  }

  // Emissive term
  pbrMat.emissive = material.emissiveFactor;
  if(isTexturePresent(material.emissiveTexture))
  {
    pbrMat.emissive *= getTexture(material.emissiveTexture, state.tc).rgb;
  }
  pbrMat.emissive = max(vec3(0.0F), pbrMat.emissive);

  // KHR_materials_specular
  // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
  pbrMat.specularColor = material.specularColorFactor;
  if(isTexturePresent(material.specularColorTexture))
  {
    pbrMat.specularColor *= getTexture(material.specularColorTexture, state.tc).rgb;
  }

  // KHR_materials_specular
  pbrMat.specular = material.specularFactor;
  if(isTexturePresent(material.specularTexture))
  {
    pbrMat.specular *= getTexture(material.specularTexture, state.tc).a;
  }

  // Dielectric Specular
  float ior1 = 1.0F;                                    // IOR of the current medium (e.g., air)
  float ior2 = material.ior;                            // IOR of the material
  if(state.isInside && (material.thicknessFactor > 0))  // If the material is thin-walled, we don't need to consider the inside IOR.
  {
    ior1 = material.ior;
    ior2 = 1.0F;
  }
  pbrMat.ior1 = ior1;
  pbrMat.ior2 = ior2;


  // KHR_materials_transmission
  pbrMat.transmission = material.transmissionFactor;
  if(isTexturePresent(material.transmissionTexture))
  {
    pbrMat.transmission *= getTexture(material.transmissionTexture, state.tc).r;
  }

  // KHR_materials_volume
  pbrMat.attenuationColor    = material.attenuationColor;
  pbrMat.attenuationDistance = material.attenuationDistance;
  pbrMat.isThinWalled        = (material.thicknessFactor == 0.0);

  // KHR_materials_clearcoat
  pbrMat.clearcoat          = material.clearcoatFactor;
  pbrMat.clearcoatRoughness = material.clearcoatRoughness;
  pbrMat.Nc                 = pbrMat.N;
  if(isTexturePresent(material.clearcoatTexture))
  {
    pbrMat.clearcoat *= getTexture(material.clearcoatTexture, state.tc).r;
  }
  if(isTexturePresent(material.clearcoatRoughnessTexture))
  {
    pbrMat.clearcoatRoughness *= getTexture(material.clearcoatRoughnessTexture, state.tc).g;
  }
  if(isTexturePresent(material.clearcoatNormalTexture))
  {
    mat3 tbn           = mat3(pbrMat.T, pbrMat.B, pbrMat.Nc);
    vec3 normal_vector = getTexture(material.clearcoatNormalTexture, state.tc).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    pbrMat.Nc          = normalize(tbn * normal_vector);
  }
  pbrMat.clearcoatRoughness = max(pbrMat.clearcoatRoughness, 0.001F);

  // KHR_materials_iridescence
  float iridescence          = material.iridescenceFactor;
  float iridescenceThickness = material.iridescenceThicknessMaximum;
  pbrMat.iridescenceIor      = material.iridescenceIor;
  if(isTexturePresent(material.iridescenceTexture))
  {
    iridescence *= getTexture(material.iridescenceTexture, state.tc).x;
  }
  if(isTexturePresent(material.iridescenceThicknessTexture))
  {
    const float t        = getTexture(material.iridescenceThicknessTexture, state.tc).y;
    iridescenceThickness = mix(material.iridescenceThicknessMinimum, material.iridescenceThicknessMaximum, t);
  }
  pbrMat.iridescence = (iridescenceThickness > 0.0f) ? iridescence : 0.0f;  // No iridescence when the thickness is zero.
  pbrMat.iridescenceThickness = iridescenceThickness;

  // KHR_materials_anisotropy
  float anisotropyStrength = material.anisotropyStrength;
  // If the anisotropyStrength == 0.0f (default), the roughness is isotropic.
  // No need to rotate the anisotropyDirection or tangent space.
  if(anisotropyStrength > 0.0F)
  {
    vec2 anisotropyDirection = vec2(1.0f, 0.0f);  // By default the anisotropy strength is along the tangent.
    if(isTexturePresent(material.anisotropyTexture))
    {
      const vec4 anisotropyTex = getTexture(material.anisotropyTexture, state.tc);

      // .xy encodes the direction in (tangent, bitangent) space. Remap from [0, 1] to [-1, 1].
      anisotropyDirection = normalize(vec2(anisotropyTex) * 2.0f - 1.0f);
      // .z encodes the strength in range [0, 1].
      anisotropyStrength *= anisotropyTex.z;
    }

    // Adjust the roughness to account for anisotropy.
    pbrMat.roughness.x = mix(pbrMat.roughness.y, 1.0f, anisotropyStrength * anisotropyStrength);

    // Rotate the anisotropy direction in the tangent space.
    const float s = material.anisotropyRotation.x;  // Sin and Cos of the rotation angle.
    const float c = material.anisotropyRotation.y;
    anisotropyDirection =
        vec2(c * anisotropyDirection.x + s * anisotropyDirection.y, c * anisotropyDirection.y - s * anisotropyDirection.x);

    // Update the tangent to be along the anisotropy direction in tangent space.
    const vec3 T_aniso = pbrMat.T * anisotropyDirection.x + pbrMat.B * anisotropyDirection.y;

    pbrMat.T           = T_aniso;
    needsTangentUpdate = true;
  }

  // Perform tangent and bitangent updates if necessary
  if(needsTangentUpdate)
  {
    // Ensure T, B, and N are orthonormal
    pbrMat.B            = cross(pbrMat.N, pbrMat.T);
    float bitangentSign = sign(dot(state.B, pbrMat.B));
    pbrMat.B            = pbrMat.B * bitangentSign;
    pbrMat.T            = cross(pbrMat.B, pbrMat.N) * bitangentSign;
  }

  // KHR_materials_sheen
  pbrMat.sheenColor = material.sheenColorFactor;
  if(isTexturePresent(material.sheenColorTexture))
  {
    pbrMat.sheenColor *= vec3(getTexture(material.sheenColorTexture, state.tc));  // sRGB
  }

  pbrMat.sheenRoughness = material.sheenRoughnessFactor;
  if(isTexturePresent(material.sheenRoughnessTexture))
  {
    pbrMat.sheenRoughness *= getTexture(material.sheenRoughnessTexture, state.tc).w;
  }
  pbrMat.sheenRoughness = max(MICROFACET_MIN_ROUGHNESS, pbrMat.sheenRoughness);

  // KHR_materials_dispersion
  pbrMat.dispersion = material.dispersion;

  return pbrMat;
}

// Compatibility function
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 texCoord)
{
  vec2      tcoords[2] = {texCoord, vec2(0.0F)};
  MeshState mesh       = MeshState(normal, tangent, bitangent, normal, tcoords, false);
  return evaluateMaterial(material, mesh);
}

#endif  // MAT_EVAL_H
