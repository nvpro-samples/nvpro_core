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


//-------------------------------------------------------------------------------------------------
// This file takes the incoming GltfShadeMaterial (material uploaded in a buffer) and
// evaluates it, basically sample the textures and return the struct PbrMaterial
// which is used by the Bsdf functions to evaluate and sample the material
//

#ifndef MAT_EVAL_H
#define MAT_EVAL_H 1

#include "pbr_mat_struct.h"

struct MeshState
{
  vec3 N;   // Normal
  vec3 T;   // Tangent
  vec3 B;   // Bitangent
  vec3 Ng;  // Geometric normal
  vec2 tc;  // Texture coordinates
  bool isInside;
};


// This is the list of all textures
#ifndef MAT_EVAL_TEXTURE_ARRAY
#define MAT_EVAL_TEXTURE_ARRAY texturesMap
#endif

#ifndef NO_TEXTURES
#define USE_TEXTURES
#endif

vec4 getTexture(in int textureIndex, in vec2 texCoord)
{
#ifdef USE_TEXTURES
  return texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(textureIndex)], texCoord);
#else
  return vec4(1.0F);
#endif
}

// If both anisotropic roughness values fall below this threshold, the BSDF switches to specular.
#define MICROFACET_MIN_ROUGHNESS 0.0014142f

//-----------------------------------------------------------------------
// From the incoming material return the material for evaluating PBR
//-----------------------------------------------------------------------
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, MeshState mesh)
{
  // Material Evaluated
  PbrMaterial pbrMat;

  // KHR_texture_transform
  vec2 texCoord = vec2(vec3(mesh.tc, 1) * material.uvTransform);

  // Base Color/Albedo may be defined from a base texture or a flat color
  vec4 baseColor = material.pbrBaseColorFactor;
  if(material.pbrBaseColorTexture > -1.0F)
  {
    baseColor *= getTexture(material.pbrBaseColorTexture, texCoord);
  }
  pbrMat.baseColor = baseColor.rgb;
  pbrMat.opacity   = baseColor.a;


  // Metallic-Roughness
  float roughness = material.pbrRoughnessFactor;
  float metallic  = material.pbrMetallicFactor;
  if(material.pbrMetallicRoughnessTexture > -1.0F)
  {
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    vec4 mr_sample = getTexture(material.pbrMetallicRoughnessTexture, texCoord);
    roughness *= mr_sample.g;
    metallic *= mr_sample.b;
  }
  pbrMat.roughness = vec2(roughness * roughness);  // Square roughness for the microfacet model
  pbrMat.metallic  = clamp(metallic, 0.0F, 1.0F);

  // Normal Map
  vec3 normal    = mesh.N;
  vec3 tangent   = mesh.T;
  vec3 bitangent = mesh.B;
  if(material.normalTexture > -1)
  {
    mat3 tbn           = mat3(mesh.T, mesh.B, mesh.N);
    vec3 normal_vector = getTexture(material.normalTexture, texCoord).xyz;
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
  if(material.emissiveTexture > -1.0F)
  {
    emissive *= vec3(getTexture(material.emissiveTexture, texCoord));
  }
  pbrMat.emissive = max(vec3(0.0F), emissive);

  // KHR_materials_specular
  // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
  pbrMat.specularColor = material.specularColorFactor;
  if(material.specularColorTexture > -1)
  {
    pbrMat.specularColor *= getTexture(material.specularColorTexture, texCoord).rgb;
  }

  pbrMat.specular = material.specularFactor;
  if(material.specularTexture > -1)
  {
    pbrMat.specular *= getTexture(material.specularTexture, texCoord).a;
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
  if(material.transmissionTexture > -1)
  {
    pbrMat.transmission *= getTexture(material.transmissionTexture, texCoord).r;
  }

  // KHR_materials_volume
  pbrMat.attenuationColor    = material.attenuationColor;
  pbrMat.attenuationDistance = material.attenuationDistance;
  pbrMat.thickness           = material.thicknessFactor;

  // KHR_materials_clearcoat
  pbrMat.clearcoat          = material.clearcoatFactor;
  pbrMat.clearcoatRoughness = material.clearcoatRoughness;
  pbrMat.Nc                 = pbrMat.N;
  if(material.clearcoatTexture > -1)
  {
    pbrMat.clearcoat *= getTexture(material.clearcoatTexture, texCoord).r;
  }

  if(material.clearcoatRoughnessTexture > -1)
  {
    pbrMat.clearcoatRoughness *= getTexture(material.clearcoatRoughnessTexture, texCoord).g;
  }
  pbrMat.clearcoatRoughness = max(pbrMat.clearcoatRoughness, 0.001F);

  if(material.clearcoatNormalTexture > -1)
  {
    mat3 tbn           = mat3(pbrMat.T, pbrMat.B, pbrMat.Nc);
    vec3 normal_vector = getTexture(material.clearcoatNormalTexture, texCoord).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    pbrMat.Nc          = normalize(tbn * normal_vector);
  }

  // Iridescence
  float iridescence = material.iridescenceFactor;
  if(material.iridescenceTexture > -1)
  {
    iridescence *= getTexture(material.iridescenceTexture, texCoord).x;
  }
  float iridescenceThickness = material.iridescenceThicknessMaximum;
  if(material.iridescenceThicknessTexture > -1)
  {
    const float t        = getTexture(material.iridescenceThicknessTexture, texCoord).y;
    iridescenceThickness = mix(material.iridescenceThicknessMinimum, material.iridescenceThicknessMaximum, t);
  }
  pbrMat.iridescence = (iridescenceThickness > 0.0f) ? iridescence : 0.0f;  // No iridescence when the thickness is zero.
  pbrMat.iridescenceIor       = material.iridescenceIor;
  pbrMat.iridescenceThickness = iridescenceThickness;

  // KHR_materials_anisotropy
  float anisotropyStrength  = material.anisotropyStrength;
  vec2  anisotropyDirection = vec2(1.0f, 0.0f);  // By default the anisotropy strength is along the tangent.
  if(material.anisotropyTexture > -1)
  {
    const vec4 anisotropyTex = getTexture(material.anisotropyTexture, texCoord);

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

    const float s = sin(material.anisotropyRotation);  // FIXME PERF Precalculate sin, cos on host.
    const float c = cos(material.anisotropyRotation);

    anisotropyDirection =
        vec2(c * anisotropyDirection.x + s * anisotropyDirection.y, c * anisotropyDirection.y - s * anisotropyDirection.x);

    const vec3 T_aniso = pbrMat.T * anisotropyDirection.x + pbrMat.B * anisotropyDirection.y;

    pbrMat.B = normalize(cross(pbrMat.N, T_aniso));
    pbrMat.T = cross(pbrMat.B, pbrMat.N);
  }


  // KHR_materials_sheen
  vec3 sheenColor = material.sheenColorFactor;
  if(material.sheenColorTexture > -1)
  {
    sheenColor *= vec3(getTexture(material.sheenColorTexture, texCoord));  // sRGB
  }
  pbrMat.sheenColor = sheenColor;  // No sheen if this is black.

  float sheenRoughness = material.sheenRoughnessFactor;
  if(material.sheenRoughnessTexture > -1)
  {
    sheenRoughness *= getTexture(material.sheenRoughnessTexture, texCoord).w;
  }
  sheenRoughness        = max(MICROFACET_MIN_ROUGHNESS, sheenRoughness);
  pbrMat.sheenRoughness = sheenRoughness;

  return pbrMat;
}

// Compatibility function
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 texCoord)
{
  MeshState mesh = MeshState(normal, tangent, bitangent, normal, texCoord, false);
  return evaluateMaterial(material, mesh);
}

#endif  // MAT_EVAL_H
