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
  set(GLFW_BUILD_TESTS OFF)
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

# stb
if (NOT TARGET stb AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stb)
  add_library(stb INTERFACE)
  target_include_directories(stb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb")
endif()
if (TARGET stb)
  LIST(APPEND THIRDPARTY_LIBRARIES stb)
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

# Zstandard (~450 KB)
# Check to make sure 
if (NOT TARGET libzstd_static AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zstd)
  if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zstd/lib/zstd.h)
    message(WARNING "It looks like the zstd submodule hasn't been downloaded; try running git submodule update --init --recursive in the nvpro_core directory.")
  else()
    set(ZSTD_BUILD_PROGRAMS OFF)
    set(ZSTD_BUILD_SHARED OFF)
    set(ZSTD_BUILD_STATIC ON)
    set(ZSTD_USE_STATIC_RUNTIME ON)
    set(_ZSTD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zstd)
    add_subdirectory(${_ZSTD_DIR}/build/cmake)
    target_sources(libzstd_static INTERFACE $<BUILD_INTERFACE:${_ZSTD_DIR}/lib/zstd.h>)
    target_include_directories(libzstd_static INTERFACE $<BUILD_INTERFACE:${_ZSTD_DIR}/lib>)
  endif()
endif()
if (TARGET libzstd_static)
  # Make targets linking with libzstd_static compile with NVP_SUPPORTS_ZSTD
  target_compile_definitions(libzstd_static INTERFACE NVP_SUPPORTS_ZSTD)
  LIST(APPEND THIRDPARTY_LIBRARIES libzstd_static)
  set_property(TARGET libzstd_static PROPERTY FOLDER "ThirdParty")
endif()

# Basis Universal (~800 KB)
if (NOT TARGET basisu AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/basis_universal)
  if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/basis_universal/transcoder)
    message(WARNING "It looks like the basis_universal submodule hasn't been downloaded; try running git submodule update --init --recursive in the nvpro_core directory.")
  else()
    set(_BASISU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/basis_universal)
    file(GLOB _BASISU_FILES "${_BASISU_DIR}/transcoder/*.*" "${_BASISU_DIR}/encoder/*.*")
    add_library(basisu STATIC "${_BASISU_FILES}")
    target_include_directories(basisu INTERFACE "${_BASISU_DIR}/transcoder" "${_BASISU_DIR}/encoder")
  endif()
endif()
if (TARGET basisu)
  # basisu.h wants to set the iterator debug level to a different value than the
  # default for debug performance. However, this can cause it to fail linking.
  target_compile_definitions(basisu PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL=1)
  # Make targets linking with basisu compile with NVP_SUPPORTS_BASISU
  target_compile_definitions(basisu INTERFACE NVP_SUPPORTS_BASISU)
  # Turn off some transcoding formats we don't use to reduce code size by about
  # 500 KB.
  target_compile_definitions(basisu PRIVATE
    BASISD_SUPPORT_ATC=0
    BASISD_SUPPORT_DXT1=0
    BASISD_SUPPORT_DXT5A=0
    BASISD_SUPPORT_ETC2_EAC_A8=0
    BASISD_SUPPORT_ETC2_EAC_RG11=0
    BASISD_SUPPORT_FXT1=0
    BASISD_SUPPORT_PVRTC1=0
    BASISD_SUPPORT_PVRTC2=0
  )
  LIST(APPEND THIRDPARTY_LIBRARIES basisu)
  set_property(TARGET basisu PROPERTY FOLDER "ThirdParty")
  # If Zstandard isn't included, also turn off Zstd support in Basis:
  if(NOT TARGET libzstd_static)
    target_compile_definitions(basisu PRIVATE BASISD_SUPPORT_KTX2_ZSTD=0)
  endif()
endif()

set(THIRDPARTY_LIBRARIES "${THIRDPARTY_LIBRARIES}" PARENT_SCOPE)
