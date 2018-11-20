#pragma once

struct ImDrawData;

#include "../nv_helpers_vk/base_vk.hpp"

namespace ImGui
{
  void InitVK(const nv_helpers_vk::DeviceUtils &utils, const nv_helpers_vk::PhysicalInfo &physical, VkRenderPass pass, VkQueue queue, uint32_t queueFamilyIndex);
  void ReInitPipelinesVK(const nv_helpers_vk::DeviceUtils &utils, VkRenderPass pass);

  void ShutdownVK();
  void RenderDrawDataVK(VkCommandBuffer cmd, const ImDrawData* drawData);
}
