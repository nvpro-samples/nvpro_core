cmake_minimum_required(VERSION 3.10)

project(third_party LANGUAGES C CXX)

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

# tinygltf
if (NOT TARGET tinygltf)
  add_library(tinygltf INTERFACE)
  set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
  target_sources(tinygltf INTERFACE ${TINYGLTF_DIR}/tiny_gltf.h ${TINYGLTF_DIR}/json.hpp)
  target_include_directories(tinygltf INTERFACE ${TINYGLTF_DIR})
endif()

# tinyobjloader
if (NOT TARGET tinyobjloader)
  add_library(tinyobjloader INTERFACE)
  set(TINYOBJLOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader)
  target_sources(tinyobjloader INTERFACE ${TINYOBJLOADER_DIR}/tiny_obj_loader.h)
  target_include_directories(tinyobjloader INTERFACE ${TINYOBJLOADER_DIR})
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


# stb
if (NOT TARGET stb)
  add_library(stb INTERFACE)
  target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")
endif()

# GLFW
if (NOT TARGET glfw)
  set(GLFW_USE_HYBRID_HPG OFF)
  set(GLFW_VULKAN_STATIC OFF)
  set(GLFW_BUILD_DOCS OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_INSTALL OFF)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(glfw)
  set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
endif()

# cgltf
if (NOT TARGET cgltf)
  add_library(cgltf INTERFACE)
  target_include_directories(cgltf INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/cgltf")
endif()

#zlib
if (USING_ZLIB)
  if (NOT TARGET zlibstatic)
    include(zlibstatic.cmake)
  endif()
endif()

# vma
if (NOT TARGET vma)
  add_library(vma INTERFACE)
  set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
  target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
  target_include_directories(vma INTERFACE ${VMA_DIR})
endif()

# NvToolsExt
if (NOT TARGET nvtx)
  add_library(nvtx INTERFACE)
  set(NVTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NVTX/c/include)
  target_sources(nvtx INTERFACE ${NVTX_DIR}/nvtx3/nvToolsExt.h)
  target_include_directories(nvtx INTERFACE ${NVTX_DIR})
endif()
