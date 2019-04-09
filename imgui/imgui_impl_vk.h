#pragma once

struct ImDrawData;

#include "../nvvk/physical_vk.hpp"
#include "../nvvk/deviceutils_vk.hpp"
#include "../nvvk/context_vk.hpp"

namespace ImGui
{
  void InitVK(const nvvk::DeviceUtils &utils, const nvvk::PhysicalInfo &physical, VkRenderPass pass, VkQueue queue, uint32_t queueFamilyIndex, int subPassIndex = 0);
  void ReInitPipelinesVK(const nvvk::DeviceUtils &utils, VkRenderPass pass, int subPassIndex = 0);

  void ShutdownVK();
  void RenderDrawDataVK(VkCommandBuffer cmd, const ImDrawData* drawData);
}
