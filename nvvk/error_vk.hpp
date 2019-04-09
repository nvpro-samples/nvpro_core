#pragma once

#include <vulkan/vulkan.h>

namespace nvvk
{
  void  check_vk_result(VkResult err);
  void  checkVkResult(const char* file, int32_t line, VkResult result);

#ifndef CHECK_VK_RESULT
#define CHECK_VK_RESULT(result) nvvk::checkVkResult(__FILE__, __LINE__, result)
#endif

#define VK_CHECK CHECK_VK_RESULT
}

