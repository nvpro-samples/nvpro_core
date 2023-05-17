/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "pipeline_vk.hpp"
#include <inttypes.h>
#include <nvh/nvprint.hpp>

namespace nvvk {

void nvprintPipelineStats(VkDevice device, VkPipeline pipeline, const char* name, bool verbose /*= false*/)
{
  VkPipelineInfoKHR pipeInfo = {VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR};
  pipeInfo.pipeline          = pipeline;
  if(!pipeline)
    return;

  std::vector<VkPipelineExecutablePropertiesKHR> props;
  uint32_t                                       executableCount = 0;
  vkGetPipelineExecutablePropertiesKHR(device, &pipeInfo, &executableCount, nullptr);
  props.resize(executableCount, {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR});
  vkGetPipelineExecutablePropertiesKHR(device, &pipeInfo, &executableCount, props.data());

  nvprintfLevel(LOGLEVEL_STATS, "VkPipeline stats for %p, %s\n", pipeline, name);
  nvprintfLevel(LOGLEVEL_STATS, "---------------------------\n");
  for(uint32_t i = 0; i < executableCount; i++)
  {
    const VkPipelineExecutablePropertiesKHR& prop = props[i];
    nvprintfLevel(LOGLEVEL_STATS, "- Executable: %s\n", prop.name);
    if(verbose)
      nvprintfLevel(LOGLEVEL_STATS, "  (%s)\n", prop.description);
    nvprintfLevel(LOGLEVEL_STATS, "  - stages: 0x%08X\n", prop.stages);
    nvprintfLevel(LOGLEVEL_STATS, "  - subgroupSize: %2d\n", prop.subgroupSize);
    VkPipelineExecutableInfoKHR execInfo = {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR};
    execInfo.pipeline                    = pipeline;
    execInfo.executableIndex             = i;

    uint32_t                                      statsCount = 0;
    std::vector<VkPipelineExecutableStatisticKHR> stats;
    vkGetPipelineExecutableStatisticsKHR(device, &execInfo, &statsCount, nullptr);
    stats.resize(statsCount, {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR});
    vkGetPipelineExecutableStatisticsKHR(device, &execInfo, &statsCount, stats.data());

    for(uint32_t s = 0; s < statsCount; s++)
    {
      const VkPipelineExecutableStatisticKHR& stat = stats[s];
      switch(stat.format)
      {
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
          nvprintfLevel(LOGLEVEL_STATS, "  - %s: %d\n", stat.name, stat.value.b32);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
          nvprintfLevel(LOGLEVEL_STATS, "  - %s: %" PRIi64 "\n", stat.name, stat.value.i64);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
          nvprintfLevel(LOGLEVEL_STATS, "  - %s: %" PRIu64 "\n", stat.name, stat.value.u64);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
          nvprintfLevel(LOGLEVEL_STATS, "  - %s: %f\n", stat.name, stat.value.f64);
          break;
      }
      if(verbose)
        nvprintfLevel(LOGLEVEL_STATS, "    (%s)\n", stat.description);
    }
  }
  nvprintfLevel(LOGLEVEL_STATS, "\n");
}

void dumpPipelineStats(VkDevice device, VkPipeline pipeline, const char* fileName)
{
  VkPipelineInfoKHR pipeInfo = {VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR};
  pipeInfo.pipeline          = pipeline;
  if(!pipeline)
    return;

  FILE* fdump = fopen(fileName, "wt");
  if(!fdump)
    return;


  std::vector<VkPipelineExecutablePropertiesKHR> props;
  uint32_t                                       executableCount = 0;
  vkGetPipelineExecutablePropertiesKHR(device, &pipeInfo, &executableCount, nullptr);
  props.resize(executableCount, {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR});
  vkGetPipelineExecutablePropertiesKHR(device, &pipeInfo, &executableCount, props.data());

  fprintf(fdump, "VkPipeline stats for %p\n", pipeline);
  fprintf(fdump, "-----------------------\n");
  for(uint32_t i = 0; i < executableCount; i++)
  {
    const VkPipelineExecutablePropertiesKHR& prop = props[i];
    fprintf(fdump, "- Executable: %s\n", prop.name);
    fprintf(fdump, "  (%s)\n", prop.description);
    fprintf(fdump, "  - stages: 0x%08X\n", prop.stages);
    fprintf(fdump, "  - subgroupSize: %2d\n", prop.subgroupSize);
    VkPipelineExecutableInfoKHR execInfo = {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR};
    execInfo.pipeline                    = pipeline;
    execInfo.executableIndex             = i;

    uint32_t                                      statsCount = 0;
    std::vector<VkPipelineExecutableStatisticKHR> stats;
    vkGetPipelineExecutableStatisticsKHR(device, &execInfo, &statsCount, nullptr);
    stats.resize(statsCount, {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR});
    vkGetPipelineExecutableStatisticsKHR(device, &execInfo, &statsCount, stats.data());

    for(uint32_t s = 0; s < statsCount; s++)
    {
      const VkPipelineExecutableStatisticKHR& stat = stats[s];
      switch(stat.format)
      {
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
          fprintf(fdump, "  - %s: %d\n", stat.name, stat.value.b32);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
          fprintf(fdump, "  - %s: %" PRIi64 "\n", stat.name, stat.value.i64);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
          fprintf(fdump, "  - %s: %" PRIu64 "\n", stat.name, stat.value.u64);
          break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
          fprintf(fdump, "  - %s: %f\n", stat.name, stat.value.f64);
          break;
      }
      fprintf(fdump, "    (%s)\n", stat.description);
    }
  }
  fprintf(fdump, "\n");
  fclose(fdump);
}

static inline std::string stringFormat(const char* msg, ...)
{
  char    text[1024];
  va_list list;

  if(msg == 0)
    return std::string();

  va_start(list, msg);
  vsnprintf(text, sizeof(text), msg, list);
  va_end(list);

  return std::string(text);
}

void dumpPipelineInternals(VkDevice device, VkPipeline pipeline, const char* baseFileName)
{
  VkPipelineInfoKHR pipeInfo = {VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR};
  pipeInfo.pipeline          = pipeline;
  if(!pipeline)
    return;

  std::vector<VkPipelineExecutablePropertiesKHR> props;
  uint32_t                                       executableCount = 0;
  vkGetPipelineExecutablePropertiesKHR(device, &pipeInfo, &executableCount, nullptr);
  props.resize(executableCount, {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR});
  vkGetPipelineExecutablePropertiesKHR(device, &pipeInfo, &executableCount, props.data());

  for(uint32_t e = 0; e < executableCount; e++)
  {
    const VkPipelineExecutablePropertiesKHR& prop     = props[e];
    VkPipelineExecutableInfoKHR              execInfo = {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR};
    execInfo.pipeline                                 = pipeline;
    execInfo.executableIndex                          = e;

    uint32_t internalCount = 0;
    vkGetPipelineExecutableInternalRepresentationsKHR(device, &execInfo, &internalCount, nullptr);
    if(internalCount)
    {
      std::vector<VkPipelineExecutableInternalRepresentationKHR> internals(
          internalCount, {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR});
      vkGetPipelineExecutableInternalRepresentationsKHR(device, &execInfo, &internalCount, internals.data());

      size_t offset = 0;
      for(uint32_t i = 0; i < internalCount; i++)
      {
        offset += internals[i].dataSize;
      }

      std::vector<uint8_t> rawBytes(offset);

      offset = 0;
      for(uint32_t i = 0; i < internalCount; i++)
      {
        internals[i].pData = &rawBytes[offset];
        offset += internals[i].dataSize;
      }

      vkGetPipelineExecutableInternalRepresentationsKHR(device, &execInfo, &internalCount, internals.data());
      for(uint32_t i = 0; i < internalCount; i++)
      {
        bool isText = strstr(internals[i].name, "text") != nullptr;

        std::string fileName = std::string(baseFileName) + "." + std::string(prop.name) + stringFormat(".%d.", e)
                               + internals[i].name + stringFormat(".%d.%s", i, isText ? "txt" : "bin");
        FILE* f = fopen(fileName.c_str(), "wb");
        if(f)
        {
          fwrite(internals[i].pData, internals[i].dataSize, 1, f);
          fclose(f);
        }
      }
    }
  }
}
}  // namespace nvvk
