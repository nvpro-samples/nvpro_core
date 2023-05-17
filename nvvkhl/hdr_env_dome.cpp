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


#define _USE_MATH_DEFINES
#include <chrono>
#include <iostream>
#include <cmath>
#include <numeric>

#include "nvmath/nvmath.h"
#include "nvh/nvprint.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/context_vk.hpp"

#include "hdr_env_dome.hpp"
#include "stb_image.h"
#include "nvvkhl/shaders/dh_comp.h"
#include "nvvkhl/shaders/dh_hdr.h"

#include "_autogen/hdr_dome.comp.h"
#include "_autogen/hdr_integrate_brdf.comp.h"
#include "_autogen/hdr_prefilter_diffuse.comp.h"
#include "_autogen/hdr_prefilter_glossy.comp.h"
#include "nvh/timesampler.hpp"


namespace nvvkhl {

HdrEnvDome::HdrEnvDome(nvvk::Context* ctx, nvvk::ResourceAllocator* allocator, uint32_t queueFamilyIndex)
{
  setup(ctx->m_device, ctx->m_physicalDevice, queueFamilyIndex, allocator);
}

//--------------------------------------------------------------------------------------------------
//
//
void HdrEnvDome::setup(const VkDevice& device, const VkPhysicalDevice& /*physicalDevice*/, uint32_t familyIndex, nvvk::ResourceAllocator* allocator)
{
  m_device      = device;
  m_alloc       = allocator;
  m_familyIndex = familyIndex;
  m_debug.setup(device);
}


//--------------------------------------------------------------------------------------------------
// The descriptor set and layout are from HDR_ENV
// - it consists of the HDR image and the acceleration structure
// - those will be used to create the diffuse and glossy image
// - Also use to 'clear' the image with the background image
//
void HdrEnvDome::create(VkDescriptorSet dstSet, VkDescriptorSetLayout dstSetLayout)
{
  destroy();
  m_hdrEnvSet    = dstSet;
  m_hdrEnvLayout = dstSetLayout;

  VkShaderModule diff_module = nvvk::createShaderModule(m_device, hdr_prefilter_diffuse_comp, sizeof(hdr_prefilter_diffuse_comp));
  VkShaderModule gloss_module = nvvk::createShaderModule(m_device, hdr_prefilter_glossy_comp, sizeof(hdr_prefilter_glossy_comp));

  createDrawPipeline();
  integrateBrdf(512, m_textures.lutBrdf);
  prefilterHdr(128, m_textures.diffuse, diff_module, false);
  prefilterHdr(512, m_textures.glossy, gloss_module, true);
  createDescriptorSetLayout();

  m_debug.setObjectName(m_textures.lutBrdf.image, "HDR_BRDF");
  m_debug.setObjectName(m_textures.diffuse.image, "HDR_Diffuse");
  m_debug.setObjectName(m_textures.glossy.image, "HDR_Glossy");

  vkDestroyShaderModule(m_device, diff_module, nullptr);
  vkDestroyShaderModule(m_device, gloss_module, nullptr);
}

//--------------------------------------------------------------------------------------------------
// This is the image the HDR will be write to, a framebuffer image or an offsceen image
//
void HdrEnvDome::setOutImage(const VkDescriptorImageInfo& outimage)
{
  VkWriteDescriptorSet wds{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  wds.dstSet          = m_domeSet;
  wds.dstBinding      = EnvDomeDraw::eHdrImage;
  wds.descriptorCount = 1;
  wds.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  wds.pImageInfo      = &outimage;
  vkUpdateDescriptorSets(m_device, 1, &wds, 0, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Compute Pipeline to "Clear" the image with the HDR as background
//
void HdrEnvDome::createDrawPipeline()
{
  // Descriptor: the output image
  nvvk::DescriptorSetBindings bind;
  bind.addBinding(EnvDomeDraw::eHdrImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);
  m_domeLayout = bind.createLayout(m_device);
  m_domePool   = bind.createPool(m_device);
  m_domeSet    = nvvk::allocateDescriptorSet(m_device, m_domePool, m_domeLayout);

  // Creating the pipeline layout
  VkPushConstantRange push_constant_ranges = {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HdrDomePushConstant)};
  std::vector<VkDescriptorSetLayout> layouts{m_domeLayout, m_hdrEnvLayout};
  VkPipelineLayoutCreateInfo         create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  create_info.setLayoutCount         = static_cast<uint32_t>(layouts.size());
  create_info.pSetLayouts            = layouts.data();
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges    = &push_constant_ranges;
  vkCreatePipelineLayout(m_device, &create_info, nullptr, &m_domePipelineLayout);

  // HDR Dome compute shader
  VkPipelineShaderStageCreateInfo stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stage_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  stage_info.module = nvvk::createShaderModule(m_device, hdr_dome_comp, sizeof(hdr_dome_comp));
  stage_info.pName  = "main";

  VkComputePipelineCreateInfo comp_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  comp_info.layout = m_domePipelineLayout;
  comp_info.stage  = stage_info;

  vkCreateComputePipelines(m_device, {}, 1, &comp_info, nullptr, &m_domePipeline);
  NAME_VK(m_domePipeline);

  // Clean up
  vkDestroyShaderModule(m_device, comp_info.stage.module, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Draw the HDR to the image (setOutImage)
// - view and projection matrix should come from the camera
// - size is the image output size (framebuffer size)
// - color is the color multiplier of the HDR (intensity)
//
void HdrEnvDome::draw(const VkCommandBuffer& cmdBuf,
                      const nvmath::mat4f&   view,
                      const nvmath::mat4f&   proj,
                      const VkExtent2D&      size,
                      const float*           color,
                      float                  rotation /*=0.f*/)
{
  LABEL_SCOPE_VK(cmdBuf);

  // This will be to have a world direction vector pointing to the pixel
  nvmath::mat4f m = nvmath::invert(proj);
  m.a30 = m.a31 = m.a32 = m.a33 = 0.0F;
  m                             = nvmath::invert(view) * m;

  // Information to the compute shader
  HdrDomePushConstant pc{};
  pc.mvp       = m;
  pc.multColor = vec4(*color);
  pc.rotation  = rotation;

  // Execution
  std::vector<VkDescriptorSet> dst_sets{m_domeSet, m_hdrEnvSet};
  vkCmdPushConstants(cmdBuf, m_domePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HdrDomePushConstant), &pc);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_domePipelineLayout, 0,
                          static_cast<uint32_t>(dst_sets.size()), dst_sets.data(), 0, nullptr);
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_domePipeline);
  VkExtent2D group_counts = getGroupCounts(size);
  vkCmdDispatch(cmdBuf, group_counts.width, group_counts.height, 1);
}


//--------------------------------------------------------------------------------------------------
//
//
void HdrEnvDome::destroy()
{
  m_alloc->destroy(m_textures.diffuse);
  m_alloc->destroy(m_textures.lutBrdf);
  m_alloc->destroy(m_textures.glossy);

  vkDestroyPipeline(m_device, m_domePipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_domePipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_domeLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_domePool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_hdrLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_hdrPool, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Descriptors of the HDR and the acceleration structure
//
void HdrEnvDome::createDescriptorSetLayout()
{
  nvvk::DescriptorSetBindings bind;
  VkShaderStageFlags          flags = VK_SHADER_STAGE_ALL;

  bind.addBinding(EnvDomeBindings::eHdrBrdf, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, flags);      // HDR image
  bind.addBinding(EnvDomeBindings::eHdrDiffuse, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, flags);   // HDR image
  bind.addBinding(EnvDomeBindings::eHdrSpecular, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, flags);  // HDR image

  m_hdrPool = bind.createPool(m_device, 1);
  CREATE_NAMED_VK(m_hdrLayout, bind.createLayout(m_device));
  CREATE_NAMED_VK(m_hdrSet, nvvk::allocateDescriptorSet(m_device, m_hdrPool, m_hdrLayout));

  std::vector<VkWriteDescriptorSet> writes;
  writes.emplace_back(bind.makeWrite(m_hdrSet, EnvDomeBindings::eHdrBrdf, &m_textures.lutBrdf.descriptor));
  writes.emplace_back(bind.makeWrite(m_hdrSet, EnvDomeBindings::eHdrDiffuse, &m_textures.diffuse.descriptor));
  writes.emplace_back(bind.makeWrite(m_hdrSet, EnvDomeBindings::eHdrSpecular, &m_textures.glossy.descriptor));

  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}


//--------------------------------------------------------------------------------------------------
// Pre-integrate glossy BRDF, see
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
void HdrEnvDome::integrateBrdf(uint32_t dimension, nvvk::Texture& target)
{
  nvh::ScopedTimer st(__FUNCTION__);

  // Create an image RG16 to store the BRDF
  VkImageCreateInfo     image_ci        = nvvk::makeImage2DCreateInfo({dimension, dimension}, VK_FORMAT_R16G16_SFLOAT,
                                                                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  nvvk::Image           image           = m_alloc->createImage(image_ci);
  VkImageViewCreateInfo image_view_info = nvvk::makeImageViewCreateInfo(image.image, image_ci);
  VkSamplerCreateInfo   sampler_ci      = nvvk::makeSamplerCreateInfo();
  target                                = m_alloc->createTexture(image, image_view_info, sampler_ci);
  target.descriptor.imageLayout         = VK_IMAGE_LAYOUT_GENERAL;

  // Compute shader
  VkDescriptorSet       dst_set{VK_NULL_HANDLE};
  VkDescriptorSetLayout dst_layout{VK_NULL_HANDLE};
  VkDescriptorPool      dst_pool{VK_NULL_HANDLE};
  VkPipeline            pipeline{VK_NULL_HANDLE};
  VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};

  {
    nvvk::ScopeCommandBuffer cmd_buf(m_device, m_familyIndex);
    LABEL_SCOPE_VK(cmd_buf);

    // Change image layout to general
    nvvk::cmdBarrierImageLayout(cmd_buf, target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // The output image is the one we have just created
    nvvk::DescriptorSetBindings bind;
    bind.addBinding(EnvDomeDraw::eHdrImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    dst_layout = bind.createLayout(m_device);
    dst_pool   = bind.createPool(m_device);
    dst_set    = nvvk::allocateDescriptorSet(m_device, dst_pool, dst_layout);

    std::vector<VkWriteDescriptorSet> writes;
    writes.emplace_back(bind.makeWrite(dst_set, 0, &target.descriptor));
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Creating the pipeline
    VkPipelineLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    create_info.setLayoutCount = 1;
    create_info.pSetLayouts    = &dst_layout;
    vkCreatePipelineLayout(m_device, &create_info, nullptr, &pipeline_layout);
    NAME_VK(pipeline_layout);

    VkPipelineShaderStageCreateInfo stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = nvvk::createShaderModule(m_device, hdr_integrate_brdf_comp, sizeof(hdr_integrate_brdf_comp));
    stage_info.pName  = "main";

    VkComputePipelineCreateInfo comp_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    comp_info.layout = pipeline_layout;
    comp_info.stage  = stage_info;

    vkCreateComputePipelines(m_device, {}, 1, &comp_info, nullptr, &pipeline);
    vkDestroyShaderModule(m_device, comp_info.stage.module, nullptr);

    // Run shader
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &dst_set, 0, nullptr);
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    VkExtent2D group_counts = getGroupCounts({dimension, dimension});
    vkCmdDispatch(cmd_buf, group_counts.width, group_counts.height, 1);
  }

  // Clean up
  vkDestroyDescriptorSetLayout(m_device, dst_layout, nullptr);
  vkDestroyDescriptorPool(m_device, dst_pool, nullptr);
  vkDestroyPipeline(m_device, pipeline, nullptr);
  vkDestroyPipelineLayout(m_device, pipeline_layout, nullptr);
}


//--------------------------------------------------------------------------------------------------
//
//
void HdrEnvDome::prefilterHdr(uint32_t dim, nvvk::Texture& target, const VkShaderModule& module, bool doMipmap)
{
  const VkExtent2D size{dim, dim};
  VkFormat         format   = VK_FORMAT_R16G16B16A16_SFLOAT;
  const uint32_t   num_mips = doMipmap ? static_cast<uint32_t>(floor(log2(dim))) + 1 : 1;

  nvh::ScopedTimer st("%s: %u", __FUNCTION__, num_mips);

  VkSamplerCreateInfo sampler_create_info = nvvk::makeSamplerCreateInfo();
  sampler_create_info.maxLod              = static_cast<float>(num_mips);

  {  // Target - cube
    VkImageCreateInfo image_create_info = nvvk::makeImageCubeCreateInfo(
        size, format, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, doMipmap);
    nvvk::Image           image           = m_alloc->createImage(image_create_info);
    VkImageViewCreateInfo image_view_info = nvvk::makeImageViewCreateInfo(image.image, image_create_info, true);
    target                                = m_alloc->createTexture(image, image_view_info, sampler_create_info);
    target.descriptor.imageLayout         = VK_IMAGE_LAYOUT_GENERAL;
  }

  nvvk::Texture scratch_texture;
  {  // Scratch texture
    VkImageCreateInfo     image_ci         = nvvk::makeImage2DCreateInfo(size, format, VK_IMAGE_USAGE_STORAGE_BIT);
    nvvk::Image           image            = m_alloc->createImage(image_ci);
    VkImageViewCreateInfo image_view_info  = nvvk::makeImageViewCreateInfo(image.image, image_ci);
    VkSamplerCreateInfo   sampler_ci       = nvvk::makeSamplerCreateInfo();
    scratch_texture                        = m_alloc->createTexture(image, image_view_info, sampler_ci);
    scratch_texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  }


  // Compute shader
  VkDescriptorSet       dst_set{VK_NULL_HANDLE};
  VkDescriptorSetLayout dst_layout{VK_NULL_HANDLE};
  VkDescriptorPool      dst_pool{VK_NULL_HANDLE};
  VkPipeline            pipeline{VK_NULL_HANDLE};
  VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};

  // Descriptors
  nvvk::DescriptorSetBindings bind;
  bind.addBinding(EnvDomeDraw::eHdrImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);
  dst_layout = bind.createLayout(m_device);
  dst_pool   = bind.createPool(m_device);
  dst_set    = nvvk::allocateDescriptorSet(m_device, dst_pool, dst_layout);

  std::vector<VkWriteDescriptorSet> writes;
  writes.emplace_back(bind.makeWrite(dst_set, 0, &scratch_texture.descriptor));  // Writing to scratch
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

  // Creating the pipeline
  VkPushConstantRange                push_constant_range{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HdrPushBlock)};
  std::vector<VkDescriptorSetLayout> layouts{dst_layout, m_hdrEnvLayout};
  VkPipelineLayoutCreateInfo         create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  create_info.setLayoutCount         = static_cast<uint32_t>(layouts.size());
  create_info.pSetLayouts            = layouts.data();
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges    = &push_constant_range;
  vkCreatePipelineLayout(m_device, &create_info, nullptr, &pipeline_layout);

  VkPipelineShaderStageCreateInfo stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stage_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
  stage_info.module = module;
  stage_info.pName  = "main";

  VkComputePipelineCreateInfo comp_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  comp_info.layout = pipeline_layout;
  comp_info.stage  = stage_info;

  vkCreateComputePipelines(m_device, {}, 1, &comp_info, nullptr, &pipeline);

  {
    nvvk::ScopeCommandBuffer cmd_buf(m_device, m_familyIndex);
    // Change scratch to general
    nvvk::cmdBarrierImageLayout(cmd_buf, scratch_texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    // Change target to destination
    VkImageSubresourceRange subresource_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, num_mips, 0, 6};
    nvvk::cmdBarrierImageLayout(cmd_buf, target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

    std::vector<VkDescriptorSet> dst_sets{dst_set, m_hdrEnvSet};
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0,
                            static_cast<uint32_t>(dst_sets.size()), dst_sets.data(), 0, nullptr);
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    renderToCube(cmd_buf, target, scratch_texture, pipeline_layout, dim, num_mips);
  }

  // Clean up
  vkDestroyDescriptorSetLayout(m_device, dst_layout, nullptr);
  vkDestroyDescriptorPool(m_device, dst_pool, nullptr);
  vkDestroyPipeline(m_device, pipeline, nullptr);
  vkDestroyPipelineLayout(m_device, pipeline_layout, nullptr);

  m_alloc->destroy(scratch_texture);
}


//--------------------------------------------------------------------------------------------------
//
//
void HdrEnvDome::renderToCube(const VkCommandBuffer& cmdBuf,
                              nvvk::Texture&         target,
                              nvvk::Texture&         scratch,
                              VkPipelineLayout       pipelineLayout,
                              uint32_t               dim,
                              const uint32_t         numMips)
{
  LABEL_SCOPE_VK(cmdBuf);

  nvmath::mat4f mat_pers = nvmath::perspectiveVK(90.0F, 1.0F, 0.1F, 10.0F);
  mat_pers               = nvmath::invert(mat_pers);
  mat_pers.a30 = mat_pers.a31 = mat_pers.a32 = mat_pers.a33 = 0.0F;

  std::array<nvmath::mat4f, 6> mv;
  const nvmath::vec3f          pos(0.0F, 0.0F, 0.0F);
  mv[0] = nvmath::look_at(pos, nvmath::vec3f(1.0F, 0.0F, 0.0F), nvmath::vec3f(0.0F, -1.0F, 0.0F));   // Positive X
  mv[1] = nvmath::look_at(pos, nvmath::vec3f(-1.0F, 0.0F, 0.0F), nvmath::vec3f(0.0F, -1.0F, 0.0F));  // Negative X
  mv[2] = nvmath::look_at(pos, nvmath::vec3f(0.0F, -1.0F, 0.0F), nvmath::vec3f(0.0F, 0.0F, -1.0F));  // Positive Y
  mv[3] = nvmath::look_at(pos, nvmath::vec3f(0.0F, 1.0F, 0.0F), nvmath::vec3f(0.0F, 0.0F, 1.0F));    // Negative Y
  mv[4] = nvmath::look_at(pos, nvmath::vec3f(0.0F, 0.0F, 1.0F), nvmath::vec3f(0.0F, -1.0F, 0.0F));   // Positive Z
  mv[5] = nvmath::look_at(pos, nvmath::vec3f(0.0F, 0.0F, -1.0F), nvmath::vec3f(0.0F, -1.0F, 0.0F));  // Negative Z
  for(auto& m : mv)
    m = nvmath::invert(m);

  // Change image layout for all cubemap faces to transfer destination
  VkImageSubresourceRange subresource_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, numMips, 0, 6};
  nvvk::cmdBarrierImageLayout(cmdBuf, target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);


  // Image barrier for compute stage
  auto barrier = [&](VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkImageMemoryBarrier    image_memory_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    image_memory_barrier.oldLayout           = oldLayout;
    image_memory_barrier.newLayout           = newLayout;
    image_memory_barrier.image               = scratch.image;
    image_memory_barrier.subresourceRange    = range;
    image_memory_barrier.srcAccessMask       = VK_ACCESS_MEMORY_WRITE_BIT;
    image_memory_barrier.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkPipelineStageFlags src_stage_mask      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkPipelineStageFlags dest_stage_mask     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(cmdBuf, src_stage_mask, dest_stage_mask, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
  };


  VkExtent3D   extent{dim, dim, 1};
  HdrPushBlock push_block{};

  for(uint32_t mip = 0; mip < numMips; mip++)
  {
    for(uint32_t f = 0; f < 6; f++)
    {
      // Update shader push constant block
      float roughness       = static_cast<float>(mip) / static_cast<float>(numMips - 1);
      push_block.roughness  = roughness;
      push_block.mvp        = mv[f] * mat_pers;
      push_block.size       = vec2f(vec2ui(extent.width, extent.height));
      push_block.numSamples = 1024 / (mip + 1);
      vkCmdPushConstants(cmdBuf, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(HdrPushBlock), &push_block);

      // Execute compute shader
      VkExtent2D group_counts = getGroupCounts({extent.width, extent.height});
      vkCmdDispatch(cmdBuf, group_counts.width, group_counts.height, 1);

      // Wait for compute to finish before copying
      barrier(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

      // Copy region for transfer from framebuffer to cube face
      VkImageCopy copy_region{};
      copy_region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      copy_region.srcSubresource.baseArrayLayer = 0;
      copy_region.srcSubresource.mipLevel       = 0;
      copy_region.srcSubresource.layerCount     = 1;
      copy_region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      copy_region.dstSubresource.baseArrayLayer = f;
      copy_region.dstSubresource.mipLevel       = mip;
      copy_region.dstSubresource.layerCount     = 1;
      copy_region.extent                        = extent;

      vkCmdCopyImage(cmdBuf, scratch.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target.image,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

      // Transform scratch texture back to general
      barrier(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    }

    // Next mipmap level
    if(extent.width > 1)
      extent.width /= 2;
    if(extent.height > 1)
      extent.height /= 2;
  }


  nvvk::cmdBarrierImageLayout(cmdBuf, target.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresource_range);
}

}  // namespace nvvkhl
