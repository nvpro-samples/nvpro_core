cmake_minimum_required(VERSION 3.10)

project(third_party LANGUAGES C CXX)

# List of found and added third party libraries
set(THIRDPARTY_LIBRARIES)

### mandatory dependencies
# GLFW
if (NOT TARGET glfw)
  set(GLFW_USE_HYBRID_HPG OFF)
  set(GLFW_VULKAN_STATIC OFF)
  set(GLFW_BUILD_DOCS OFF)
  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_INSTALL OFF)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(glfw)
  set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
endif()

# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui" )
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")

if (NOT TARGET imgui)
  add_library(imgui STATIC ${IMGUI_FILES})
  target_include_directories(imgui PUBLIC ${IMGUI_DIR})
  set_property(TARGET imgui PROPERTY FOLDER "ThirdParty")
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui" PARENT_SCOPE)

# fmt
if (NOT TARGET fmt)
  add_library(fmt STATIC fmt/src/format.cc)
  target_include_directories(fmt PUBLIC fmt/include)
  set_property(TARGET fmt PROPERTY FOLDER "ThirdParty")
endif()


### optional dependencies

# glm
# add_library(glm INTERFACE)
# set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
# target_sources(glm INTERFACE ${GLM_DIR}/glm/glm.hpp)
# target_include_directories(glm INTERFACE ${GLM_DIR})
# 
# target_compile_definitions(glm INTERFACE
#     GLM_FORCE_SWIZZLE
#     GLM_FORCE_RADIANS
#     GLM_FORCE_CTOR_INIT
#     GLM_ENABLE_EXPERIMENTAL
# )
# if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#     target_compile_definitions(glm INTERFACE GLM_FORCE_CXX14)
# endif()

# stb
if (NOT TARGET stb AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stb)
  add_library(stb INTERFACE)
  target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")
  # Use secure C Runtime calls when compiling with MSVC
  if(MSVC)
    target_compile_definitions(stb INTERFACE STBI_MSC_SECURE_CRT)
  endif()
endif()
if (TARGET stb)
  LIST(APPEND THIRDPARTY_LIBRARIES stb)
endif()

# tinygltf
if (NOT TARGET tinygltf AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
  add_library(tinygltf INTERFACE)
  set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
  target_sources(tinygltf INTERFACE ${TINYGLTF_DIR}/tiny_gltf.h ${TINYGLTF_DIR}/json.hpp)
  target_include_directories(tinygltf INTERFACE ${TINYGLTF_DIR})
endif()
if (TARGET tinygltf)
  LIST(APPEND THIRDPARTY_LIBRARIES tinygltf)
endif()

# tinyobjloader
if (NOT TARGET tinyobjloader AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader)
  add_library(tinyobjloader INTERFACE)
  set(TINYOBJLOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader)
  target_sources(tinyobjloader INTERFACE ${TINYOBJLOADER_DIR}/tiny_obj_loader.h)
  target_include_directories(tinyobjloader INTERFACE ${TINYOBJLOADER_DIR})
endif()
if (TARGET tinyobjloader)
  LIST(APPEND THIRDPARTY_LIBRARIES tinyobjloader)
endif()

# cgltf
if (NOT TARGET cgltf AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
  add_library(cgltf INTERFACE)
  target_include_directories(cgltf INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/cgltf")
endif()
if (TARGET cgltf)
  LIST(APPEND THIRDPARTY_LIBRARIES cgltf)
endif()

#zlib
if (USING_ZLIB)
  if (NOT TARGET zlibstatic AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
    include(zlibstatic.cmake)
  endif()
endif()

# vma
if (NOT TARGET vma AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vma)
  add_library(vma INTERFACE)
  set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
  target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
  target_include_directories(vma INTERFACE ${VMA_DIR})
  # Although nvpro_core requires at least Vulkan 1.3.x, we tell VMA to only
  # link with core extension symbols from Vulkan 1.2.x. This avoids
  # "undefined symbol" errors at load-time on Linux when running projects
  # using nvpro_core on systems with earlier versions of libvulkan.so.
  target_compile_definitions(vma INTERFACE VMA_VULKAN_VERSION=1002000)
endif()
if (TARGET vma)
  LIST(APPEND THIRDPARTY_LIBRARIES vma)
endif()

# NvToolsExt
if (NOT TARGET nvtx AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/NVTX)
  add_library(nvtx INTERFACE)
  set(NVTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NVTX/c/include)
  target_sources(nvtx INTERFACE ${NVTX_DIR}/nvtx3/nvToolsExt.h)
  target_include_directories(nvtx INTERFACE ${NVTX_DIR})
endif()
if (TARGET nvtx)
  LIST(APPEND THIRDPARTY_LIBRARIES nvtx)
endif()

# DirectX12-Headers
if (NOT TARGET dxh AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dxh)
  add_library(dxh INTERFACE)
  target_include_directories(dxh INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/dxh/include/directx")
endif()
if (TARGET dxh)
  LIST(APPEND THIRDPARTY_LIBRARIES dxh)
endif()

set(THIRDPARTY_LIBRARIES "${THIRDPARTY_LIBRARIES}" PARENT_SCOPE)
