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


/*
 *  HDR sampling is loading an HDR image and creating an acceleration structure for 
 *  sampling the environment. 
 */

#define _USE_MATH_DEFINES
#include <numeric>
#include <chrono>
#include <array>

#include "hdr_env.hpp"

#include "nvmath/nvmath.h"
#include "stb_image.h"
#include "nvh/fileoperations.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/context_vk.hpp"

#include "shaders/dh_hdr.h"
#include "nvh/timesampler.hpp"

namespace nvvkhl {
// Forward declaration
std::vector<EnvAccel> createEnvironmentAccel(float*& pixels, const uint32_t& width, const uint32_t& height, float& average, float& integral);

HdrEnv::HdrEnv(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator, uint32_t queueFamilyIndex)
{
  setup(ctx->m_device, ctx->m_physicalDevice, queueFamilyIndex, allocator);
}


//--------------------------------------------------------------------------------------------------
//
//
void HdrEnv::setup(const VkDevice& device, const VkPhysicalDevice& /*physicalDevice*/, uint32_t familyIndex, nvvk::ResourceAllocator* allocator)
{
  m_device      = device;
  m_alloc       = allocator;
  m_familyIndex = familyIndex;
  m_debug.setup(device);
}

//--------------------------------------------------------------------------------------------------
//
//
void HdrEnv::destroy()
{
  vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_descSetLayout, nullptr);
  m_descPool      = {};
  m_descSetLayout = {};

  m_alloc->destroy(m_texHdr);
  m_alloc->destroy(m_accelImpSmpl);
}

//--------------------------------------------------------------------------------------------------
// Loading the HDR environment texture (HDR) and create the important accel structure
//
void HdrEnv::loadEnvironment(const std::string& hrdImage)
{
  nvh::ScopedTimer st(__FUNCTION__);

  m_valid = false;

  if(!hrdImage.empty())
  {
    int32_t width{0};
    int32_t height{0};
    int32_t component{0};
    float*  pixels = nullptr;

    if(stbi_is_hdr(hrdImage.c_str()) != 0)
    {
      pixels = stbi_loadf(hrdImage.c_str(), &width, &height, &component, STBI_rgb_alpha);
    }


    if(pixels != nullptr)
    {
      VkDeviceSize buffer_size = width * height * 4 * sizeof(float);
      VkExtent2D   img_size{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

      m_hdrImageSize = img_size;

      VkSamplerCreateInfo sampler_create_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
      sampler_create_info.minFilter  = VK_FILTER_LINEAR;
      sampler_create_info.magFilter  = VK_FILTER_LINEAR;
      sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      // The map is parameterized with the U axis corresponding to the azimuthal angle, and V to the polar angle
      // Therefore, in U the sampler will use VK_SAMPLER_ADDRESS_MODE_REPEAT (default), but V needs to use
      // CLAMP_TO_EDGE to avoid having light leaking from one pole to another.
      sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      VkFormat          format         = VK_FORMAT_R32G32B32A32_SFLOAT;
      VkImageCreateInfo ic_info =
          nvvk::makeImage2DCreateInfo(img_size, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

      // We can use a different family index (1 - transfer), to allow loading in a different queue/thread than the display (0)
      VkQueue queue = nullptr;
      vkGetDeviceQueue(m_device, m_familyIndex, 0, &queue);

      {
        nvh::ScopedTimer st("Generating Acceleration structure");
        {
          nvvk::ScopeCommandBuffer cmd_buf(m_device, m_familyIndex, queue);

          // Creating the importance sampling for the HDR and storing the info in the m_accelImpSmpl buffer
          auto env_accel = createEnvironmentAccel(pixels, img_size.width, img_size.height, m_average, m_integral);
          m_accelImpSmpl = m_alloc->createBuffer(cmd_buf, env_accel, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
          m_debug.setObjectName(m_accelImpSmpl.buffer, "HDR_accel");

          nvvk::Image           image   = m_alloc->createImage(cmd_buf, buffer_size, pixels, ic_info);
          VkImageViewCreateInfo iv_info = nvvk::makeImageViewCreateInfo(image.image, ic_info);
          m_texHdr                      = m_alloc->createTexture(image, iv_info, sampler_create_info);
          m_debug.setObjectName(m_texHdr.image, "HDR");
        }
        m_alloc->finalizeAndReleaseStaging();
      }

      stbi_image_free(pixels);

      m_valid = true;
    }
  }

  if(!m_valid)
  {  // Dummy
    VkQueue queue = nullptr;
    vkGetDeviceQueue(m_device, m_familyIndex, 0, &queue);
    {
      nvvk::ScopeCommandBuffer cmd_buf(m_device, m_familyIndex, queue);
      VkImageCreateInfo     image_create_info = nvvk::makeImage2DCreateInfo(VkExtent2D{1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
      std::vector<uint8_t>  color{255, 255, 255, 255};
      nvvk::Image           image   = m_alloc->createImage(cmd_buf, 4, color.data(), image_create_info);
      VkImageViewCreateInfo iv_info = nvvk::makeImageViewCreateInfo(image.image, image_create_info);
      VkSamplerCreateInfo   sampler_create_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
      m_texHdr       = m_alloc->createTexture(image, iv_info, sampler_create_info);
      m_accelImpSmpl = m_alloc->createBuffer(cmd_buf, color, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }
    m_alloc->finalizeAndReleaseStaging();
    m_valid = false;
  }

  createDescriptorSetLayout();
}


//--------------------------------------------------------------------------------------------------
// Descriptors of the HDR and the acceleration structure
//
void HdrEnv::createDescriptorSetLayout()
{
  nvvk::DescriptorSetBindings bind;
  VkShaderStageFlags          flags = VK_SHADER_STAGE_ALL;

  bind.addBinding(EnvBindings::eHdr, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, flags);  // HDR image
  bind.addBinding(EnvBindings::eImpSamples, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);   // importance sampling

  m_descPool = bind.createPool(m_device, 1);
  CREATE_NAMED_VK(m_descSetLayout, bind.createLayout(m_device));
  CREATE_NAMED_VK(m_descSet, nvvk::allocateDescriptorSet(m_device, m_descPool, m_descSetLayout));

  std::vector<VkWriteDescriptorSet> writes;
  VkDescriptorBufferInfo            accel_imp_smpl{m_accelImpSmpl.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(bind.makeWrite(m_descSet, EnvBindings::eHdr, &m_texHdr.descriptor));
  writes.emplace_back(bind.makeWrite(m_descSet, EnvBindings::eImpSamples, &accel_imp_smpl));

  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Build alias map for the importance sampling: Each texel is associated to another texel, or alias,
// so that their combined intensities are a close as possible to the average of the environment map.
// This will later allow the sampling shader to uniformly select a texel in the environment, and
// select either that texel or its alias depending on their relative intensities
//
inline float buildAliasmap(const std::vector<float>& data, std::vector<EnvAccel>& accel)
{
  auto size = static_cast<uint32_t>(data.size());

  // Compute the integral of the emitted radiance of the environment map
  // Since each element in data is already weighted by its solid angle
  // the integral is a simple sum
  float sum = std::accumulate(data.begin(), data.end(), 0.F);

  // For each texel, compute the ratio q between the emitted radiance of the texel and the average
  // emitted radiance over the entire sphere
  // We also initialize the aliases to identity, ie. each texel is its own alias
  auto  f_size          = static_cast<float>(size);
  float inverse_average = f_size / sum;
  for(uint32_t i = 0; i < size; ++i)
  {
    accel[i].q     = data[i] * inverse_average;
    accel[i].alias = i;
  }

  // Partition the texels according to their emitted radiance ratio wrt. average.
  // Texels with a value q < 1 (ie. below average) are stored incrementally from the beginning of the
  // array, while texels emitting higher-than-average radiance are stored from the end of the array
  std::vector<uint32_t> partition_table(size);
  uint32_t              s     = 0U;
  uint32_t              large = size;
  for(uint32_t i = 0; i < size; ++i)
  {
    if(accel[i].q < 1.F)
      partition_table[s++] = i;
    else
      partition_table[--large] = i;
  }

  // Associate the lower-energy texels to higher-energy ones. Since the emission of a high-energy texel may
  // be vastly superior to the average,
  for(s = 0; s < large && large < size; ++s)
  {
    // Index of the smaller energy texel
    const uint32_t small_energy_index = partition_table[s];

    // Index of the higher energy texel
    const uint32_t high_energy_index = partition_table[large];

    // Associate the texel to its higher-energy alias
    accel[small_energy_index].alias = high_energy_index;

    // Compute the difference between the lower-energy texel and the average
    const float difference_with_average = 1.F - accel[small_energy_index].q;

    // The goal is to obtain texel couples whose combined intensity is close to the average.
    // However, some texels may have low energies, while others may have very high intensity
    // (for example a sunset: the sky is quite dark, but the sun is still visible). In this case
    // it may not be possible to obtain a value close to average by combining only two texels.
    // Instead, we potentially associate a single high-energy texel to many smaller-energy ones until
    // the combined average is similar to the average of the environment map.
    // We keep track of the combined average by subtracting the difference between the lower-energy texel and the average
    // from the ratio stored in the high-energy texel.
    accel[high_energy_index].q -= difference_with_average;

    // If the combined ratio to average of the higher-energy texel reaches 1, a balance has been found
    // between a set of low-energy texels and the higher-energy one. In this case, we will use the next
    // higher-energy texel in the partition when processing the next texel.
    if(accel[high_energy_index].q < 1.0F)
      large++;
  }
  // Return the integral of the emitted radiance. This integral will be used to normalize the probability
  // distribution function (PDF) of each pixel
  return sum;
}

// CIE luminance
inline float luminance(const float* color)
{
  return color[0] * 0.2126F + color[1] * 0.7152F + color[2] * 0.0722F;
}

//--------------------------------------------------------------------------------------------------
// Create acceleration data for importance sampling
// See:  https://arxiv.org/pdf/1901.05423.pdf
// And store the PDF into the ALPHA channel of pixels
//
inline std::vector<EnvAccel> createEnvironmentAccel(float*& pixels, const uint32_t& width, const uint32_t& height, float& average, float& integral)
{
  const uint32_t rx = width;
  const uint32_t ry = height;

  // Create importance sampling data
  std::vector<EnvAccel> env_accel(rx * ry);
  std::vector<float>    importance_data(rx * ry);
  float                 cos_theta0 = 1.0F;
  const float           step_phi   = static_cast<float>(2.0F * M_PI) / static_cast<float>(rx);
  const float           step_theta = static_cast<float>(M_PI) / static_cast<float>(ry);
  double                total      = 0.0;

  // For each texel of the environment map, we compute the related solid angle
  // subtended by the texel, and store the weighted luminance in importance_data,
  // representing the amount of energy emitted through each texel.
  // Also compute the average CIE luminance to drive the tonemapping of the final image
  for(uint32_t y = 0; y < ry; ++y)
  {
    const float theta1     = static_cast<float>(y + 1) * step_theta;
    const float cos_theta1 = std::cos(theta1);
    const float area       = (cos_theta0 - cos_theta1) * step_phi;  // solid angle
    cos_theta0             = cos_theta1;

    for(uint32_t x = 0; x < rx; ++x)
    {
      const uint32_t idx           = y * rx + x;
      const uint32_t idx4          = idx * 4;
      float          cie_luminance = luminance(&pixels[idx4]);
      importance_data[idx]         = area * std::max(pixels[idx4], std::max(pixels[idx4 + 1], pixels[idx4 + 2]));
      total += cie_luminance;
    }
  }

  average = static_cast<float>(total) / static_cast<float>(rx * ry);

  // Build the alias map, which aims at creating a set of texel couples
  // so that all couples emit roughly the same amount of energy. To this aim,
  // each smaller radiance texel will be assigned an "alias" with higher emitted radiance
  // As a byproduct this function also returns the integral of the radiance emitted by the environment
  integral = buildAliasmap(importance_data, env_accel);

  // We deduce the PDF of each texel by normalizing its emitted radiance by the radiance integral
  const float inv_env_integral = 1.0F / integral;
  for(uint32_t i = 0; i < rx * ry; ++i)
  {
    const uint32_t idx4 = i * 4;
    pixels[idx4 + 3]    = std::max(pixels[idx4], std::max(pixels[idx4 + 1], pixels[idx4 + 2])) * inv_env_integral;
  }

  return env_accel;
}

}  // namespace nvvkhl
