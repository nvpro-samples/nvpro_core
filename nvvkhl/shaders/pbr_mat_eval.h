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

vec4 getTexture(in GltfTextureInfo tinfo, in MeshState mstate)
{
#ifdef USE_TEXTURES
  // KHR_texture_transform
  vec2 texCoord = vec2(vec3(mstate.tc[tinfo.texCoord], 1) * tinfo.uvTransform);
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
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, MeshState mesh)
{
  // Material Evaluated
  PbrMaterial pbrMat;

  // Base Color/Albedo may be defined from a base texture or a flat color
  vec4 baseColor = material.pbrBaseColorFactor;
  if(isTexturePresent(material.pbrBaseColorTexture))
  {
    baseColor *= getTexture(material.pbrBaseColorTexture, mesh);
  }
  pbrMat.baseColor = baseColor.rgb;
  pbrMat.opacity   = baseColor.a;

  pbrMat.occlusion = material.occlusionStrength;
  if(isTexturePresent(material.occlusionTexture))
  {
    float occlusion  = getTexture(material.occlusionTexture, mesh).r;
    pbrMat.occlusion = 1.0 + pbrMat.occlusion * (occlusion - 1.0);
  }

  // Metallic-Roughness
  float roughness = material.pbrRoughnessFactor;
  float metallic  = material.pbrMetallicFactor;
  if(isTexturePresent(material.pbrMetallicRoughnessTexture))
  {
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    vec4 mr_sample = getTexture(material.pbrMetallicRoughnessTexture, mesh);
    roughness *= mr_sample.g;
    metallic *= mr_sample.b;
  }
  roughness        = max(roughness, MICROFACET_MIN_ROUGHNESS);
  pbrMat.roughness = vec2(roughness * roughness);  // Square roughness for the microfacet model
  pbrMat.metallic  = clamp(metallic, 0.0F, 1.0F);

  // Normal Map
  vec3 normal    = mesh.N;
  vec3 tangent   = mesh.T;
  vec3 bitangent = mesh.B;
  if(isTexturePresent(material.normalTexture))
  {
    mat3 tbn           = mat3(mesh.T, mesh.B, mesh.N);
    vec3 normal_vector = getTexture(material.normalTexture, mesh).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    normal_vector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0F);
    normal = normalize(tbn * normal_vector);
  }
  // We should always have B = cross(N, T) * bitangentSign. Orthonormalize,
  // in case the normal was changed or the input wasn't orthonormal:
  pbrMat.N            = normal;
  pbrMat.B            = cross(pbrMat.N, tangent);
  float bitangentSign = sign(dot(bitangent, pbrMat.B));
  pbrMat.B            = normalize(pbrMat.B) * bitangentSign;
  pbrMat.T            = normalize(cross(pbrMat.B, pbrMat.N)) * bitangentSign;
  pbrMat.Ng           = mesh.Ng;

  // Emissive term
  vec3 emissive = material.emissiveFactor;
  if(isTexturePresent(material.emissiveTexture))
  {
    emissive *= vec3(getTexture(material.emissiveTexture, mesh));
  }
  pbrMat.emissive = max(vec3(0.0F), emissive);

  // KHR_materials_specular
  // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
  pbrMat.specularColor = material.specularColorFactor;
  if(isTexturePresent(material.specularColorTexture))
  {
    pbrMat.specularColor *= getTexture(material.specularColorTexture, mesh).rgb;
  }

  pbrMat.specular = material.specularFactor;
  if(isTexturePresent(material.specularTexture))
  {
    pbrMat.specular *= getTexture(material.specularTexture, mesh).a;
  }


  // Dielectric Specular
  float ior1 = 1.0F;                                   // IOR of the current medium (e.g., air)
  float ior2 = material.ior;                           // IOR of the material
  if(mesh.isInside && (material.thicknessFactor > 0))  // If the material is thin-walled, we don't need to consider the inside IOR.
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
    pbrMat.transmission *= getTexture(material.transmissionTexture, mesh).r;
  }

  // KHR_materials_volume
  pbrMat.attenuationColor    = material.attenuationColor;
  pbrMat.attenuationDistance = material.attenuationDistance;
  pbrMat.thickness           = material.thicknessFactor;

  // KHR_materials_clearcoat
  pbrMat.clearcoat          = material.clearcoatFactor;
  pbrMat.clearcoatRoughness = material.clearcoatRoughness;
  pbrMat.Nc                 = pbrMat.N;
  if(isTexturePresent(material.clearcoatTexture))
  {
    pbrMat.clearcoat *= getTexture(material.clearcoatTexture, mesh).r;
  }

  if(isTexturePresent(material.clearcoatRoughnessTexture))
  {
    pbrMat.clearcoatRoughness *= getTexture(material.clearcoatRoughnessTexture, mesh).g;
  }
  pbrMat.clearcoatRoughness = max(pbrMat.clearcoatRoughness, 0.001F);

  if(isTexturePresent(material.clearcoatNormalTexture))
  {
    mat3 tbn           = mat3(pbrMat.T, pbrMat.B, pbrMat.Nc);
    vec3 normal_vector = getTexture(material.clearcoatNormalTexture, mesh).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    pbrMat.Nc          = normalize(tbn * normal_vector);
  }

  // Iridescence
  float iridescence = material.iridescenceFactor;
  if(isTexturePresent(material.iridescenceTexture))
  {
    iridescence *= getTexture(material.iridescenceTexture, mesh).x;
  }
  float iridescenceThickness = material.iridescenceThicknessMaximum;
  if(isTexturePresent(material.iridescenceThicknessTexture))
  {
    const float t        = getTexture(material.iridescenceThicknessTexture, mesh).y;
    iridescenceThickness = mix(material.iridescenceThicknessMinimum, material.iridescenceThicknessMaximum, t);
  }
  pbrMat.iridescence = (iridescenceThickness > 0.0f) ? iridescence : 0.0f;  // No iridescence when the thickness is zero.
  pbrMat.iridescenceIor       = material.iridescenceIor;
  pbrMat.iridescenceThickness = iridescenceThickness;

  // KHR_materials_anisotropy
  float anisotropyStrength  = material.anisotropyStrength;
  vec2  anisotropyDirection = vec2(1.0f, 0.0f);  // By default the anisotropy strength is along the tangent.
  if(isTexturePresent(material.anisotropyTexture))
  {
    const vec4 anisotropyTex = getTexture(material.anisotropyTexture, mesh);

    // .xy encodes the direction in (tangent, bitangent) space. Remap from [0, 1] to [-1, 1].
    anisotropyDirection = normalize(vec2(anisotropyTex) * 2.0f - 1.0f);
    // .z encodes the strength in range [0, 1].
    anisotropyStrength *= anisotropyTex.z;
  }

  // If the anisotropyStrength == 0.0f (default), the roughness is isotropic.
  // No need to rotate the anisotropyDirection or tangent space.
  if(anisotropyStrength > 0.0F)
  {
    pbrMat.roughness.x = mix(pbrMat.roughness.y, 1.0f, anisotropyStrength * anisotropyStrength);

    // Rotate the anisotropy direction in the tangent space.
    const float s = material.anisotropyRotation.x;  // Sin and Cos of the rotation angle.
    const float c = material.anisotropyRotation.y;
    anisotropyDirection =
        vec2(c * anisotropyDirection.x + s * anisotropyDirection.y, c * anisotropyDirection.y - s * anisotropyDirection.x);

    const vec3 T_aniso = pbrMat.T * anisotropyDirection.x + pbrMat.B * anisotropyDirection.y;

    pbrMat.B = normalize(cross(pbrMat.N, T_aniso));
    pbrMat.T = cross(pbrMat.B, pbrMat.N);
  }


  // KHR_materials_sheen
  vec3 sheenColor = material.sheenColorFactor;
  if(isTexturePresent(material.sheenColorTexture))
  {
    sheenColor *= vec3(getTexture(material.sheenColorTexture, mesh));  // sRGB
  }
  pbrMat.sheenColor = sheenColor;  // No sheen if this is black.

  float sheenRoughness = material.sheenRoughnessFactor;
  if(isTexturePresent(material.sheenRoughnessTexture))
  {
    sheenRoughness *= getTexture(material.sheenRoughnessTexture, mesh).w;
  }
  sheenRoughness        = max(MICROFACET_MIN_ROUGHNESS, sheenRoughness);
  pbrMat.sheenRoughness = sheenRoughness;

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
