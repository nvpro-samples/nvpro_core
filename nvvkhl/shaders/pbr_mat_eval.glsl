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


//-------------------------------------------------------------------------------------------------
// This file takes the incoming GltfShadeMaterial (material uploaded in a buffer) and
// evaluates it, basically sample the textures and return the struct PbrMaterial
// which is used by the Bsdf functions to evaluate and sample the material
//

#ifndef MAT_EVAL_H
#define MAT_EVAL_H 1

#include "pbr_mat_struct.h"

// This is the list of all textures
#ifndef MAT_EVAL_TEXTURE_ARRAY
#define MAT_EVAL_TEXTURE_ARRAY texturesMap
#endif

//  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// MATERIAL FOR EVALUATION
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
const float g_min_reflectance = 0.04F;
//-----------------------------------------------------------------------


// sRGB to linear approximation, see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec4 srgbToLinear(in vec4 sRgb)
{
  //return vec4(pow(sRgb.xyz, vec3(2.2f)), sRgb.w);
  vec3 rgb = sRgb.xyz * (sRgb.xyz * (sRgb.xyz * 0.305306011F + 0.682171111F) + 0.012522878F);
  return vec4(rgb, sRgb.a);
}


//-----------------------------------------------------------------------
// From the incoming material return the material for evaluating PBR
//-----------------------------------------------------------------------
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 texCoord, in bool isInside)
{
  float perceptual_roughness = 0.0F;
  float metallic             = 0.0F;
  vec3  f0                   = vec3(0.0F);
  vec3  f90                  = vec3(1.0F);
  vec4  baseColor            = vec4(0.0F, 0.0F, 0.0F, 1.0F);

  // KHR_texture_transform
  texCoord = vec2(vec3(texCoord, 1) * material.uvTransform);

  // Normal Map
  if(material.normalTexture > -1)
  {
    mat3 tbn           = mat3(tangent, bitangent, normal);
    vec3 normal_vector = texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.normalTexture)], texCoord).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    normal_vector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0F);
    normal = normalize(tbn * normal_vector);
  }

  // Metallic-Roughness
  {
    perceptual_roughness = material.pbrRoughnessFactor;
    metallic             = material.pbrMetallicFactor;
    if(material.pbrMetallicRoughnessTexture > -1.0F)
    {
      // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
      vec4 mr_sample = texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.pbrMetallicRoughnessTexture)], texCoord);
      perceptual_roughness *= mr_sample.g;
      metallic *= mr_sample.b;
    }

    // The albedo may be defined from a base texture or a flat color
    baseColor = material.pbrBaseColorFactor;
    if(material.pbrBaseColorTexture > -1.0F)
    {
      baseColor *= texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.pbrBaseColorTexture)], texCoord);
    }
    vec3 specular_color = mix(vec3(g_min_reflectance), vec3(baseColor), float(metallic));
    f0                  = specular_color;
  }

  // Protection
  metallic = clamp(metallic, 0.0F, 1.0F);


  // Emissive term
  vec3 emissive = material.emissiveFactor;
  if(material.emissiveTexture > -1.0F)
  {
    emissive *= vec3(texture(MAT_EVAL_TEXTURE_ARRAY[material.emissiveTexture], texCoord));
  }

  // KHR_materials_specular
  // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular
  vec4 specularColorTexture = vec4(1.0F);
  if(material.specularColorTexture > -1)
  {
    specularColorTexture = textureLod(texturesMap[nonuniformEXT(material.specularColorTexture)], texCoord, 0);
  }
  float specularTexture = 1.0F;
  if(material.specularTexture > -1)
  {
    specularTexture = textureLod(texturesMap[nonuniformEXT(material.specularTexture)], texCoord, 0).a;
  }


  // Dielectric Specular
  float ior1 = 1.0F;
  float ior2 = material.ior;
  if(isInside)
  {
    ior1 = material.ior;
    ior2 = 1.0F;
  }
  float iorRatio    = ((ior1 - ior2) / (ior1 + ior2));
  float iorRatioSqr = iorRatio * iorRatio;

  vec3  dielectricSpecularF0  = material.specularColorFactor * specularColorTexture.rgb;
  float dielectricSpecularF90 = material.specularFactor * specularTexture;

  f0  = mix(min(iorRatioSqr * dielectricSpecularF0, vec3(1.0F)) * dielectricSpecularF0, baseColor.rgb, metallic);
  f90 = vec3(mix(dielectricSpecularF90, 1.0F, metallic));


  // Material Evaluated
  PbrMaterial pbrMat;
  pbrMat.albedo    = baseColor;
  pbrMat.f0        = f0;
  pbrMat.f90       = f90;
  pbrMat.roughness = perceptual_roughness;
  pbrMat.metallic  = metallic;
  pbrMat.emissive  = max(vec3(0.0F), emissive);
  pbrMat.normal    = normal;
  pbrMat.eta       = (material.thicknessFactor == 0.0F) ? 1.0F : ior1 / ior2;

  // KHR_materials_transmission
  pbrMat.transmissionFactor = material.transmissionFactor;
  if(material.transmissionTexture > -1)
  {
    pbrMat.transmissionFactor *= textureLod(texturesMap[nonuniformEXT(material.transmissionTexture)], texCoord, 0).r;
  }

  // KHR_materials_ior
  pbrMat.ior = material.ior;

  // KHR_materials_volume
  pbrMat.attenuationColor    = material.attenuationColor;
  pbrMat.attenuationDistance = material.attenuationDistance;
  pbrMat.thicknessFactor     = material.thicknessFactor;

  // KHR_materials_clearcoat
  pbrMat.clearcoatFactor    = material.clearcoatFactor;
  pbrMat.clearcoatRoughness = material.clearcoatRoughness;
  if(material.clearcoatTexture > -1)
  {
    pbrMat.clearcoatFactor *= textureLod(texturesMap[nonuniformEXT(material.clearcoatTexture)], texCoord, 0).r;
  }
  if(material.clearcoatRoughnessTexture > -1)
  {
    pbrMat.clearcoatRoughness *= textureLod(texturesMap[nonuniformEXT(material.clearcoatRoughnessTexture)], texCoord, 0).g;
  }
  pbrMat.clearcoatRoughness = max(pbrMat.clearcoatRoughness, 0.001F);

  return pbrMat;
}

PbrMaterial evaluateMaterial(in GltfShadeMaterial material, in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 texCoord)
{
  return evaluateMaterial(material, normal, tangent, bitangent, texCoord, false);
}

#endif  // MAT_EVAL_H
