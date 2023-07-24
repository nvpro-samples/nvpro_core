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


float getPerceivedBrightness(vec3 vector)
{
  return sqrt(0.299f * vector.x * vector.x + 0.587f * vector.y * vector.y + 0.114f * vector.z * vector.z);
}


//-------------------------------------------------------------------------------------------------
// Specular-Glossiness converter
// See: // https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js#L34
//-------------------------------------------------------------------------------------------------
float solveMetallic(vec3 diffuse, vec3 specular, float one_minus_specular_strength)
{
  float specular_brightness = getPerceivedBrightness(specular);

  if(specular_brightness < g_min_reflectance)
  {
    return 0.f;
  }

  float diffuse_brightness = getPerceivedBrightness(diffuse);

  float a = g_min_reflectance;
  float b = diffuse_brightness * one_minus_specular_strength / (1.f - g_min_reflectance) + specular_brightness - 2.f * g_min_reflectance;
  float c = g_min_reflectance - specular_brightness;
  float d = max(b * b - 4.0F * a * c, 0.0F);

  return clamp((-b + sqrt(d)) / (2.0F * a), 0.0F, 1.0F);
}


//-----------------------------------------------------------------------
// From the incoming material return the material for evaluating PBR
//-----------------------------------------------------------------------
PbrMaterial evaluateMaterial(in GltfShadeMaterial material, in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 texCoord)
{
  float perceptual_roughness = 0.0F;
  float metallic             = 0.0F;
  vec3  f0                   = vec3(0.0F);
  vec4  base_color           = vec4(0.0F, 0.0F, 0.0F, 1.0F);

  // Normal Map
  if(material.normalTexture > -1)
  {
    mat3 tbn           = mat3(tangent, bitangent, normal);
    vec3 normal_vector = texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.normalTexture)], texCoord).xyz;
    normal_vector      = normal_vector * 2.0F - 1.0F;
    normal_vector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0F);
    normal = normalize(tbn * normal_vector);
  }


  if(material.shadingModel == MATERIAL_METALLICROUGHNESS)
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
    base_color = material.pbrBaseColorFactor;
    if(material.pbrBaseColorTexture > -1.0F)
    {
      base_color *= texture(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.pbrBaseColorTexture)], texCoord);
    }
    vec3 specular_color = mix(vec3(g_min_reflectance), vec3(base_color), float(metallic));
    f0                  = specular_color;
  }
  // Specular-Glossiness which will be converted to metallic-roughness
  else if(material.shadingModel == MATERIAL_SPECULARGLOSSINESS)
  {
    f0                   = material.khrSpecularFactor;
    perceptual_roughness = material.khrGlossinessFactor;

    if(material.khrSpecularGlossinessTexture > -1.0F)
    {
      vec4 sg_sample =
          srgbToLinear(textureLod(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.khrSpecularGlossinessTexture)], texCoord, 0));
      perceptual_roughness *= sg_sample.a;  // glossiness to roughness
      f0 *= sg_sample.rgb;                  // specular
    }

    perceptual_roughness = 1.0F - perceptual_roughness;

    vec4 diffuse_color = material.khrDiffuseFactor;
    if(material.khrDiffuseTexture > -1.0F)
    {
      diffuse_color *= srgbToLinear(textureLod(MAT_EVAL_TEXTURE_ARRAY[nonuniformEXT(material.khrDiffuseTexture)], texCoord, 0));
    }

    vec3  specular_color              = f0;  // f0 = specular
    float one_minus_specular_strength = 1.0F - max(max(f0.r, f0.g), f0.b);
    base_color.rgb                    = diffuse_color.rgb * one_minus_specular_strength;
    metallic                          = solveMetallic(diffuse_color.rgb, specular_color, one_minus_specular_strength);
  }

  // Emissive term
  vec3 emissive = material.emissiveFactor;
  if(material.emissiveTexture > -1.0F)
  {
    emissive *= vec3(texture(MAT_EVAL_TEXTURE_ARRAY[material.emissiveTexture], texCoord));
  }


  // Material Evaluated
  PbrMaterial pbrMat;
  pbrMat.albedo    = base_color;
  pbrMat.f0        = f0;
  pbrMat.roughness = perceptual_roughness;
  pbrMat.metallic  = clamp(metallic, 0.0F, 1.0F);
  pbrMat.emissive  = max(vec3(0), emissive);
  pbrMat.normal    = normal;

  // KHR_materials_transmission
  pbrMat.transmission = material.transmissionFactor;
  if(material.transmissionTexture > -1)
  {
    pbrMat.transmission *= textureLod(texturesMap[nonuniformEXT(material.transmissionTexture)], texCoord, 0).r;
  }

  // KHR_materials_ior
  pbrMat.ior = material.ior;
  // KHR_materials_volume
  pbrMat.attenuationColor    = material.attenuationColor;
  pbrMat.attenuationDistance = material.attenuationDistance;
  pbrMat.thinWalled          = material.thicknessFactor == 0;
  // KHR_materials_clearcoat
  pbrMat.clearcoat = material.clearcoatFactor;
  pbrMat.clearcoatRoughness= material.clearcoatRoughness;
  if(material.clearcoatTexture > -1)
  {
    pbrMat.clearcoat *= textureLod(texturesMap[nonuniformEXT(material.clearcoatTexture)], texCoord, 0).r;
  }
  if(material.clearcoatRoughnessTexture > -1)
  {
    pbrMat.clearcoatRoughness *=
        textureLod(texturesMap[nonuniformEXT(material.clearcoatRoughnessTexture)], texCoord, 0).g;
  }
  pbrMat.clearcoatRoughness = max(pbrMat.clearcoatRoughness, 0.001);

  return pbrMat;
}

#endif  // MAT_EVAL_H
