/*
 * Copyright (c) 2020-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2020-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "gizmos_vk.hpp"

namespace nvvk {

//#include "E:\temp\glsl\axis.vert.h"
static const uint32_t s_vert_spv[] = {
    0x07230203, 0x00010500, 0x0008000a, 0x0000006e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001,
    0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000b000f, 0x00000000,
    0x00000004, 0x6e69616d, 0x00000000, 0x0000000c, 0x0000002e, 0x00000032, 0x0000003b, 0x00000041, 0x00000045,
    0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x0000000c,
    0x6f727261, 0x65765f77, 0x00007472, 0x00030005, 0x0000002c, 0x00736f70, 0x00060005, 0x0000002e, 0x565f6c67,
    0x65747265, 0x646e4978, 0x00007865, 0x00070005, 0x00000032, 0x495f6c67, 0x6174736e, 0x4965636e, 0x7865646e,
    0x00000000, 0x00050005, 0x00000039, 0x65746e69, 0x6c6f7072, 0x00746e61, 0x00050006, 0x00000039, 0x00000000,
    0x6f6c6f43, 0x00000072, 0x00030005, 0x0000003b, 0x0074754f, 0x00060005, 0x0000003f, 0x505f6c67, 0x65567265,
    0x78657472, 0x00000000, 0x00060006, 0x0000003f, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005,
    0x00000041, 0x00000000, 0x00060005, 0x00000043, 0x73755075, 0x6e6f4368, 0x6e617473, 0x00000074, 0x00060006,
    0x00000043, 0x00000000, 0x6e617274, 0x726f6673, 0x0000006d, 0x00030005, 0x00000045, 0x00006370, 0x00040047,
    0x0000002e, 0x0000000b, 0x0000002a, 0x00040047, 0x00000032, 0x0000000b, 0x0000002b, 0x00030047, 0x00000039,
    0x00000002, 0x00040047, 0x0000003b, 0x0000001e, 0x00000000, 0x00050048, 0x0000003f, 0x00000000, 0x0000000b,
    0x00000000, 0x00030047, 0x0000003f, 0x00000002, 0x00040048, 0x00000043, 0x00000000, 0x00000005, 0x00050048,
    0x00000043, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x00000043, 0x00000000, 0x00000007, 0x00000010,
    0x00030047, 0x00000043, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
    0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000003, 0x00040015, 0x00000008, 0x00000020,
    0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000008, 0x0004001c, 0x0000000a, 0x00000007, 0x00000009,
    0x00040020, 0x0000000b, 0x00000006, 0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000006, 0x00040015,
    0x0000000d, 0x00000020, 0x00000001, 0x0004002b, 0x0000000d, 0x0000000e, 0x00000000, 0x0004002b, 0x00000006,
    0x0000000f, 0x3f800000, 0x0004002b, 0x00000006, 0x00000010, 0x00000000, 0x0006002c, 0x00000007, 0x00000011,
    0x0000000f, 0x00000010, 0x00000010, 0x00040020, 0x00000012, 0x00000006, 0x00000007, 0x0004002b, 0x0000000d,
    0x00000014, 0x00000001, 0x0004002b, 0x00000006, 0x00000015, 0x3f400000, 0x0004002b, 0x00000006, 0x00000016,
    0x3dcccccd, 0x0006002c, 0x00000007, 0x00000017, 0x00000015, 0x00000016, 0x00000016, 0x0004002b, 0x0000000d,
    0x00000019, 0x00000002, 0x0004002b, 0x00000006, 0x0000001a, 0xbdcccccd, 0x0006002c, 0x00000007, 0x0000001b,
    0x00000015, 0x00000016, 0x0000001a, 0x0004002b, 0x0000000d, 0x0000001d, 0x00000003, 0x0006002c, 0x00000007,
    0x0000001e, 0x00000015, 0x0000001a, 0x0000001a, 0x0004002b, 0x0000000d, 0x00000020, 0x00000004, 0x0006002c,
    0x00000007, 0x00000021, 0x00000015, 0x0000001a, 0x00000016, 0x0004002b, 0x0000000d, 0x00000023, 0x00000005,
    0x0004002b, 0x0000000d, 0x00000025, 0x00000006, 0x0006002c, 0x00000007, 0x00000026, 0x00000010, 0x00000010,
    0x00000010, 0x0004002b, 0x0000000d, 0x00000028, 0x00000007, 0x0006002c, 0x00000007, 0x00000029, 0x00000015,
    0x00000010, 0x00000010, 0x00040020, 0x0000002b, 0x00000007, 0x00000007, 0x00040020, 0x0000002d, 0x00000001,
    0x0000000d, 0x0004003b, 0x0000002d, 0x0000002e, 0x00000001, 0x0004003b, 0x0000002d, 0x00000032, 0x00000001,
    0x00020014, 0x00000034, 0x00040017, 0x00000038, 0x00000006, 0x00000004, 0x0003001e, 0x00000039, 0x00000038,
    0x00040020, 0x0000003a, 0x00000003, 0x00000039, 0x0004003b, 0x0000003a, 0x0000003b, 0x00000003, 0x0007002c,
    0x00000038, 0x0000003c, 0x0000000f, 0x00000010, 0x00000010, 0x0000000f, 0x00040020, 0x0000003d, 0x00000003,
    0x00000038, 0x0003001e, 0x0000003f, 0x00000038, 0x00040020, 0x00000040, 0x00000003, 0x0000003f, 0x0004003b,
    0x00000040, 0x00000041, 0x00000003, 0x00040018, 0x00000042, 0x00000038, 0x00000004, 0x0003001e, 0x00000043,
    0x00000042, 0x00040020, 0x00000044, 0x00000009, 0x00000043, 0x0004003b, 0x00000044, 0x00000045, 0x00000009,
    0x00040020, 0x00000046, 0x00000009, 0x00000042, 0x0007002c, 0x00000038, 0x00000055, 0x00000010, 0x0000000f,
    0x00000010, 0x0000000f, 0x0007002c, 0x00000038, 0x00000062, 0x00000010, 0x00000010, 0x0000000f, 0x0000000f,
    0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x0000002b,
    0x0000002c, 0x00000007, 0x00050041, 0x00000012, 0x00000013, 0x0000000c, 0x0000000e, 0x0003003e, 0x00000013,
    0x00000011, 0x00050041, 0x00000012, 0x00000018, 0x0000000c, 0x00000014, 0x0003003e, 0x00000018, 0x00000017,
    0x00050041, 0x00000012, 0x0000001c, 0x0000000c, 0x00000019, 0x0003003e, 0x0000001c, 0x0000001b, 0x00050041,
    0x00000012, 0x0000001f, 0x0000000c, 0x0000001d, 0x0003003e, 0x0000001f, 0x0000001e, 0x00050041, 0x00000012,
    0x00000022, 0x0000000c, 0x00000020, 0x0003003e, 0x00000022, 0x00000021, 0x00050041, 0x00000012, 0x00000024,
    0x0000000c, 0x00000023, 0x0003003e, 0x00000024, 0x00000017, 0x00050041, 0x00000012, 0x00000027, 0x0000000c,
    0x00000025, 0x0003003e, 0x00000027, 0x00000026, 0x00050041, 0x00000012, 0x0000002a, 0x0000000c, 0x00000028,
    0x0003003e, 0x0000002a, 0x00000029, 0x0004003d, 0x0000000d, 0x0000002f, 0x0000002e, 0x00050041, 0x00000012,
    0x00000030, 0x0000000c, 0x0000002f, 0x0004003d, 0x00000007, 0x00000031, 0x00000030, 0x0003003e, 0x0000002c,
    0x00000031, 0x0004003d, 0x0000000d, 0x00000033, 0x00000032, 0x000500aa, 0x00000034, 0x00000035, 0x00000033,
    0x0000000e, 0x000300f7, 0x00000037, 0x00000000, 0x000400fa, 0x00000035, 0x00000036, 0x00000050, 0x000200f8,
    0x00000036, 0x00050041, 0x0000003d, 0x0000003e, 0x0000003b, 0x0000000e, 0x0003003e, 0x0000003e, 0x0000003c,
    0x00050041, 0x00000046, 0x00000047, 0x00000045, 0x0000000e, 0x0004003d, 0x00000042, 0x00000048, 0x00000047,
    0x0004003d, 0x00000007, 0x00000049, 0x0000002c, 0x00050051, 0x00000006, 0x0000004a, 0x00000049, 0x00000000,
    0x00050051, 0x00000006, 0x0000004b, 0x00000049, 0x00000001, 0x00050051, 0x00000006, 0x0000004c, 0x00000049,
    0x00000002, 0x00070050, 0x00000038, 0x0000004d, 0x0000004a, 0x0000004b, 0x0000004c, 0x0000000f, 0x00050091,
    0x00000038, 0x0000004e, 0x00000048, 0x0000004d, 0x00050041, 0x0000003d, 0x0000004f, 0x00000041, 0x0000000e,
    0x0003003e, 0x0000004f, 0x0000004e, 0x000200f9, 0x00000037, 0x000200f8, 0x00000050, 0x0004003d, 0x0000000d,
    0x00000051, 0x00000032, 0x000500aa, 0x00000034, 0x00000052, 0x00000051, 0x00000014, 0x000300f7, 0x00000054,
    0x00000000, 0x000400fa, 0x00000052, 0x00000053, 0x00000061, 0x000200f8, 0x00000053, 0x00050041, 0x0000003d,
    0x00000056, 0x0000003b, 0x0000000e, 0x0003003e, 0x00000056, 0x00000055, 0x00050041, 0x00000046, 0x00000057,
    0x00000045, 0x0000000e, 0x0004003d, 0x00000042, 0x00000058, 0x00000057, 0x0004003d, 0x00000007, 0x00000059,
    0x0000002c, 0x0008004f, 0x00000007, 0x0000005a, 0x00000059, 0x00000059, 0x00000001, 0x00000000, 0x00000002,
    0x00050051, 0x00000006, 0x0000005b, 0x0000005a, 0x00000000, 0x00050051, 0x00000006, 0x0000005c, 0x0000005a,
    0x00000001, 0x00050051, 0x00000006, 0x0000005d, 0x0000005a, 0x00000002, 0x00070050, 0x00000038, 0x0000005e,
    0x0000005b, 0x0000005c, 0x0000005d, 0x0000000f, 0x00050091, 0x00000038, 0x0000005f, 0x00000058, 0x0000005e,
    0x00050041, 0x0000003d, 0x00000060, 0x00000041, 0x0000000e, 0x0003003e, 0x00000060, 0x0000005f, 0x000200f9,
    0x00000054, 0x000200f8, 0x00000061, 0x00050041, 0x0000003d, 0x00000063, 0x0000003b, 0x0000000e, 0x0003003e,
    0x00000063, 0x00000062, 0x00050041, 0x00000046, 0x00000064, 0x00000045, 0x0000000e, 0x0004003d, 0x00000042,
    0x00000065, 0x00000064, 0x0004003d, 0x00000007, 0x00000066, 0x0000002c, 0x0008004f, 0x00000007, 0x00000067,
    0x00000066, 0x00000066, 0x00000001, 0x00000002, 0x00000000, 0x00050051, 0x00000006, 0x00000068, 0x00000067,
    0x00000000, 0x00050051, 0x00000006, 0x00000069, 0x00000067, 0x00000001, 0x00050051, 0x00000006, 0x0000006a,
    0x00000067, 0x00000002, 0x00070050, 0x00000038, 0x0000006b, 0x00000068, 0x00000069, 0x0000006a, 0x0000000f,
    0x00050091, 0x00000038, 0x0000006c, 0x00000065, 0x0000006b, 0x00050041, 0x0000003d, 0x0000006d, 0x00000041,
    0x0000000e, 0x0003003e, 0x0000006d, 0x0000006c, 0x000200f9, 0x00000054, 0x000200f8, 0x00000054, 0x000200f9,
    0x00000037, 0x000200f8, 0x00000037, 0x000100fd, 0x00010038};


//#include "E:\temp\glsl\axis.frag.h"
static const uint32_t s_frag_spv[] = {
    0x07230203, 0x00010500, 0x0008000a, 0x00000012, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001,
    0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004,
    0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x00030010, 0x00000004, 0x00000007, 0x00030003,
    0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000009, 0x6c6f4366,
    0x0000726f, 0x00050005, 0x0000000a, 0x65746e69, 0x6c6f7072, 0x00746e61, 0x00050006, 0x0000000a, 0x00000000,
    0x6f6c6f43, 0x00000072, 0x00030005, 0x0000000c, 0x00006e49, 0x00040047, 0x00000009, 0x0000001e, 0x00000000,
    0x00030047, 0x0000000a, 0x00000002, 0x00040047, 0x0000000c, 0x0000001e, 0x00000000, 0x00020013, 0x00000002,
    0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
    0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003,
    0x0003001e, 0x0000000a, 0x00000007, 0x00040020, 0x0000000b, 0x00000001, 0x0000000a, 0x0004003b, 0x0000000b,
    0x0000000c, 0x00000001, 0x00040015, 0x0000000d, 0x00000020, 0x00000001, 0x0004002b, 0x0000000d, 0x0000000e,
    0x00000000, 0x00040020, 0x0000000f, 0x00000001, 0x00000007, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x0000000f, 0x00000010, 0x0000000c, 0x0000000e, 0x0004003d,
    0x00000007, 0x00000011, 0x00000010, 0x0003003e, 0x00000009, 0x00000011, 0x000100fd, 0x00010038};


//--------------------------------------------------------------------------------------------------
//
//
void AxisVK::display(VkCommandBuffer cmdBuf, const nvmath::mat4f& transform, const VkExtent2D& screenSize)
{
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineTriangleFan);

  // Setup viewport:
  VkViewport viewport{};
  viewport.width    = float(screenSize.width);
  viewport.height   = float(screenSize.height);
  viewport.minDepth = 0;
  viewport.maxDepth = 1;
  VkRect2D rect;
  rect.offset = VkOffset2D{0, 0};
  rect.extent = VkExtent2D{screenSize.width, screenSize.height};
  vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
  vkCmdSetScissor(cmdBuf, 0, 1, &rect);


  // Set the orthographic matrix in the bottom left corner
  {
    const float         pixelW   = m_axisSize / screenSize.width;
    const float         pixelH   = m_axisSize / screenSize.height;
    const nvmath::mat4f matOrtho = {pixelW * .8f,  0.0f,          0.0f,  0.0f,  //
                                    0.0f,          -pixelH * .8f, 0.0f,  0.0f,  //
                                    0.0f,          0.0f,          -0.5f, 0.0f,  //
                                    -1.f + pixelW, 1.f - pixelH,  0.5f,  1.0f};

    nvmath::mat4f modelView = transform;
    modelView.set_translate({0, 0, 0});
    modelView = matOrtho * modelView;
    // Push the matrix to the shader
    vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(nvmath::mat4f), &modelView.a00);
  }

  // Draw 3 times the tip of the arrow, the shader is flipping the orientation and setting the color
  vkCmdDraw(cmdBuf, 6, 3, 0, 0);
  // Now draw the line of the arrow using the last 2 vertex of the buffer (offset 5)
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLines);
  vkCmdDraw(cmdBuf, 2, 3, 6, 0);
}

void AxisVK::createAxisObject(CreateAxisInfo& info)
{
  // The shader need Push Constants: the transformation matrix
  const VkPushConstantRange push_constants{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(nvmath::mat4f)};

  VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layout_info.pushConstantRangeCount = 1;
  layout_info.pPushConstantRanges    = &push_constants;
  vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipelineLayout);

  // Creation of the pipeline
  VkShaderModule smVertex;
  VkShaderModule smFrag;

  VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = sizeof(s_vert_spv);
  createInfo.pCode    = s_vert_spv;
  vkCreateShaderModule(m_device, &createInfo, nullptr, &smVertex);
  createInfo.codeSize = sizeof(s_frag_spv);
  createInfo.pCode    = s_frag_spv;
  vkCreateShaderModule(m_device, &createInfo, nullptr, &smFrag);

  // Pipeline state
  nvvk::GraphicsPipelineState gps;
  gps.inputAssemblyState.topology         = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
  gps.rasterizationState.cullMode         = VK_CULL_MODE_NONE;
  gps.depthStencilState.depthTestEnable   = VK_TRUE;
  gps.depthStencilState.stencilTestEnable = VK_FALSE;
  gps.depthStencilState.depthCompareOp    = VK_COMPARE_OP_LESS_OR_EQUAL;

  // Creating the tips
  nvvk::GraphicsPipelineGenerator gpg(m_device, m_pipelineLayout, info.renderPass, gps);
  gpg.addShader(smVertex, VK_SHADER_STAGE_VERTEX_BIT);
  gpg.addShader(smFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

  // Dynamic Rendering
  VkPipelineRenderingCreateInfoKHR rfInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
  if(info.renderPass == VK_NULL_HANDLE)
  {
    rfInfo.colorAttachmentCount    = static_cast<uint32_t>(info.colorFormat.size());
    rfInfo.pColorAttachmentFormats = info.colorFormat.data();
    rfInfo.depthAttachmentFormat   = info.depthFormat;
    rfInfo.stencilAttachmentFormat = info.stencilFormat;
    gpg.createInfo.pNext           = &rfInfo;
  }
  
  m_pipelineTriangleFan = gpg.createPipeline();

  // Creating the lines
  gps.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  m_pipelineLines                 = gpg.createPipeline();

  vkDestroyShaderModule(m_device, smVertex, nullptr);
  vkDestroyShaderModule(m_device, smFrag, nullptr);
}


// glsl_shader.vert, compiled with: (see comment)
/*************************************************

#version 450 core
// glslangValidator.exe --target-env vulkan1.2 --vn s_vert_spv -o axis.vert.h axis.vert

layout(push_constant) uniform uPushConstant
{
  mat4 transform;
}
pc;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out interpolant
{
  vec4 Color;
}
Out;

// Arrow along the x axis
const float asize = 1.0f;  // lenght of arrow
const float atip = 0.1f;   // width of arrow tip
const float abase = 0.66f; // 0.25 == tip lenght

vec3 arrow_vert[8];

void main()
{
  arrow_vert[0] = vec3(asize, 0, 0); // Tip
  arrow_vert[1] = vec3(abase, atip, atip);
  arrow_vert[2] = vec3(abase, atip, -atip);
  arrow_vert[3] = vec3(abase, -atip, -atip);
  arrow_vert[4] = vec3(abase, -atip, atip);
  arrow_vert[5] = vec3(abase, atip, atip);

  arrow_vert[6] = vec3(0, 0, 0);     // To draw the line
  arrow_vert[7] = vec3(abase, 0, 0); // To draw the line

  //  const float t = 0.04f;
  //  arrow_vert[6] = vec3(0, t, t);      // To draw the line
  //  arrow_vert[7] = vec3(abase, t, t);  // To draw the line
  //  arrow_vert[8] = vec3(0, -t, t);     // To draw the line
  //  arrow_vert[9] = vec3(abase, -t, t); // To draw the line
  //  //
  //  arrow_vert[10] = vec3(0, -t, -t);     // To draw the line
  //  arrow_vert[11] = vec3(abase, -t, -t); // To draw the line
  //                                        //
  //  arrow_vert[12] = vec3(0, t, -t);      // To draw the line
  //  arrow_vert[13] = vec3(abase, t, -t);  // To draw the line
  //
  //  arrow_vert[14] = vec3(0, t, t);     // To draw the line
  //  arrow_vert[15] = vec3(abase, t, t); // To draw the line

  vec3 pos = arrow_vert[gl_VertexIndex];
  // Out.Color = aColor;
  if (gl_InstanceIndex == 0)
  {
    Out.Color = vec4(1, 0, 0, 1);
    gl_Position = pc.transform * vec4(pos.xyz, 1);
  }
  else if (gl_InstanceIndex == 1)
  {
    Out.Color = vec4(0, 1, 0, 1);
    gl_Position = pc.transform * vec4(pos.yxz, 1);
  }
  else
  {
    Out.Color = vec4(0, 0, 1, 1);
    gl_Position = pc.transform * vec4(pos.yzx, 1);
  }
}


*********************/


// glsl_shader.frag
/*************************************************

#version 450 core
// glslangValidator.exe --target-env vulkan1.2 --vn s_frag_spv -o axis.frag.h axis.frag
layout(location = 0) out vec4 fColor;

layout(location = 0) in interpolant
{
  vec4 Color;
}
In;

void main()
{
  fColor = In.Color;
}


*********************/

}  // namespace nvvk
