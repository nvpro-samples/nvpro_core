#*****************************************************************************
# Copyright 2020-2023 NVIDIA Corporation. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#*****************************************************************************
include_guard(GLOBAL)

# Check for obsolete folders to remove
if(EXISTS ${BASE_DIRECTORY}/shared_sources)
  message(WARNING "shared_sources got renamed as nvpro_core. Please remove shared_sources")
endif()
if(EXISTS ${BASE_DIRECTORY}/shared_external)
  message(WARNING "Please remove shared_external folder : folder obsolete, now")
endif()


#The OLD behavior for this policy is to ignore <PackageName>_ROOT variables
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()


# Set the C/C++ specified in the projects as requirements
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# IDE Setup
set_property(GLOBAL PROPERTY USE_FOLDERS ON)  # Generate folders for IDE targets
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_cmake")

# https://cmake.org/cmake/help/latest/policy/CMP0072.html
set(OpenGL_GL_PREFERENCE GLVND)

set(SUPPORT_NVTOOLSEXT OFF CACHE BOOL "enable NVToolsExt for custom NSIGHT markers")
set(NSIGHT_AFTERMATH_SDK "" CACHE PATH "Point to top directory of nSight Aftermath SDK")

# We use the presence of NSIGHT_AFTERMATH_SDK as enable-switch for Aftermath
if (NOT NSIGHT_AFTERMATH_SDK)
	if (DEFINED ENV{NSIGHT_AFTERMATH_SDK})
	  set(NSIGHT_AFTERMATH_SDK  $ENV{NSIGHT_AFTERMATH_SDK})
	endif()
endif()
if (NSIGHT_AFTERMATH_SDK)
  message(STATUS "Enabling nSight Aftermath; NSIGHT_AFTERMATH_SDK provided as ${NSIGHT_AFTERMATH_SDK}")
  set(SUPPORT_AFTERMATH ON)
endif()

if(WIN32)
  set( MEMORY_LEAKS_CHECK OFF CACHE BOOL "Check for Memory leaks" )
endif(WIN32)

if(MSVC)
  # Enable parallel builds by default on MSVC
  string(APPEND CMAKE_C_FLAGS " /MP")
  string(APPEND CMAKE_CXX_FLAGS " /MP")
endif()

# USD specifc stuff
# When building a library containing nvpro-core code, it needs to be compiled position independent
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(UNIX)
    # Our copy of USD is not compiled with the C++11 ABI.
    # Disable it for us as, so we can use USD
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()

set(RESOURCE_DIRECTORY "${BASE_DIRECTORY}/nvpro_core/resources")
add_definitions(-DRESOURCE_DIRECTORY="${RESOURCE_DIRECTORY}/")

include_directories(${BASE_DIRECTORY}/nvpro_core)
include_directories(${BASE_DIRECTORY}/nvpro_core/nvp)

# Specify the list of directories to search for cmake modules.
list(APPEND CMAKE_MODULE_PATH ${BASE_DIRECTORY}/nvpro_core/cmake ${BASE_DIRECTORY}/nvpro_core/cmake/find)
set(CMAKE_FIND_ROOT_PATH "")

message(STATUS "BASE_DIRECTORY = ${BASE_DIRECTORY}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH "x64" CACHE STRING "CPU Architecture")
else ()
  set(ARCH "x86" CACHE STRING "CPU Architecture")
endif()

if(NOT OUTPUT_PATH)
  set(OUTPUT_PATH ${BASE_DIRECTORY}/bin_${ARCH} CACHE PATH "Directory where outputs will be stored")
endif()

# Set the default build to Release.  Note this doesn't do anything for the VS
# default build target.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if(NOT EXISTS ${BASE_DIRECTORY}/downloaded_resources)
  file(MAKE_DIRECTORY ${BASE_DIRECTORY}/downloaded_resources)
endif()

set(DOWNLOAD_TARGET_DIR "${BASE_DIRECTORY}/downloaded_resources")
set(DOWNLOAD_SITE http://developer.download.nvidia.com/ProGraphics/nvpro-samples)

#####################################################################################
function(_make_relative FROM TO OUT)
  #message(STATUS "FROM = ${FROM}")
  #message(STATUS "TO = ${TO}")
  
  file(RELATIVE_PATH _TMP_STR "${FROM}" "${TO}")
  
  #message(STATUS "_TMP_STR = ${_TMP_STR}")
  
  set (${OUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

macro(_add_project_definitions name)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${BASE_DIRECTORY}/_install" CACHE PATH "folder in which INSTALL will put everything needed to run the binaries" FORCE)
  endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(CUDA_TOOLKIT_ROOT_DIR)
    string(REPLACE "\\" "/" CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR})
  endif()
  if(CMAKE_INSTALL_PREFIX)
    string(REPLACE "\\" "/" CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  endif()
  
  # the "config" directory doesn't really exist but serves as place holder
  # for the actual CONFIG based directories (Release, RelWithDebInfo etc.)
  _make_relative("${OUTPUT_PATH}/config" "${CMAKE_CURRENT_SOURCE_DIR}" TO_CURRENT_SOURCE_DIR)
  _make_relative("${OUTPUT_PATH}/config" "${DOWNLOAD_TARGET_DIR}" TO_DOWNLOAD_TARGET_DIR)
  
  add_definitions(-DPROJECT_NAME="${name}")
  add_definitions(-DPROJECT_RELDIRECTORY="${TO_CURRENT_SOURCE_DIR}/")
  add_definitions(-DPROJECT_DOWNLOAD_RELDIRECTORY="${TO_DOWNLOAD_TARGET_DIR}/")
endmacro(_add_project_definitions)

#####################################################################################

macro(_set_subsystem_console exename)
  if(WIN32)
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
     target_compile_definitions(${exename} PRIVATE "_CONSOLE")
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
  endif(WIN32)
endmacro(_set_subsystem_console)

macro(_set_subsystem_windows exename)
  if(WIN32)
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
     set_target_properties(${exename} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
  endif(WIN32)
endmacro(_set_subsystem_windows)

#####################################################################################
if(UNIX) 
  set(OS "linux")
  add_definitions(-DLINUX)
else(UNIX)
  if(APPLE)
  else(APPLE)
    if(WIN32)
      set(OS "win")
      add_definitions(-DNOMINMAX)
      if(MEMORY_LEAKS_CHECK)
        add_definitions(-DMEMORY_LEAKS_CHECK)
        message(STATUS "MEMORY_LEAKS_CHECK is enabled; any memory leaks in apps will be reported when they close.")
      endif()
    endif(WIN32)
  endif(APPLE)
endif(UNIX)


# Macro for adding files close to the executable
macro(_copy_files_to_target target thefiles)
  foreach (FFF ${thefiles} )
    if(EXISTS "${FFF}")
      add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${FFF}
          $<TARGET_FILE_DIR:${target}>
          VERBATIM
      )
    endif()
  endforeach()
endmacro()

# Macro for adding files close to the executable
macro(_copy_file_to_target target thefile folder)
  if(EXISTS "${thefile}")
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${target}>/${folder}"
        VERBATIM
    )
    add_custom_command(
      TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${thefile}
        "$<TARGET_FILE_DIR:${target}>/${folder}"
        VERBATIM
    )
  endif()
endmacro()

#####################################################################################
# Optional OpenGL package
#
macro(_add_package_OpenGL)
  find_package(OpenGL)  
  if(OPENGL_FOUND)
      Message(STATUS "--> using package OpenGL")
      get_directory_property(hasParent PARENT_DIRECTORY)
      if(hasParent)
        set( USING_OPENGL "YES" PARENT_SCOPE) # PARENT_SCOPE important to have this variable passed to parent. Here we want to notify that something used the OpenGL package
      endif()
      set( USING_OPENGL "YES")
      add_definitions(-DNVP_SUPPORTS_OPENGL)
 else(OPENGL_FOUND)
     Message(STATUS "--> NOT using package OpenGL")
 endif(OPENGL_FOUND)
endmacro(_add_package_OpenGL)
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_OpenGL)
  if(USING_OPENGL AND NOT OPENGL_FOUND)
    _add_package_OpenGL()
  endif()
endmacro(_optional_package_OpenGL)

#####################################################################################
# Optional ZLIB
#
macro(_add_package_ZLIB)
  add_definitions(-DNVP_SUPPORTS_GZLIB=1)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set(USING_ZLIB ON PARENT_SCOPE)
  else()
    set(USING_ZLIB ON)
  endif()
  LIST(APPEND LIBRARIES_OPTIMIZED zlibstatic)
  LIST(APPEND LIBRARIES_DEBUG zlibstatic)
endmacro()

#####################################################################################
# Optional VMA
#
macro(_add_package_VMA)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set(USING_VMA ON PARENT_SCOPE)
  else()
    set(USING_VMA ON)
  endif()
endmacro()

#####################################################################################
# ImGUI
#
macro(_add_package_ImGUI)
  Message(STATUS "--> using package ImGUI")
  include_directories(${BASE_DIRECTORY}/nvpro_core/imgui)

  set(USING_IMGUI ON)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set(USING_IMGUI ON PARENT_SCOPE) # PARENT_SCOPE important to have this variable passed to parent. Here we want to notify that something used the OpenGL package
  endif()

endmacro()


#####################################################################################
# FreeImage
#
macro(_add_package_FreeImage)
  Message(STATUS "--> using package FreeImage")
  find_package(FreeImage)
  if(FREEIMAGE_FOUND)
    add_definitions(-DNVP_SUPPORTS_FREEIMAGE)
    include_directories(${FREEIMAGE_INCLUDE_DIR})
    LIST(APPEND PACKAGE_SOURCE_FILES 
      ${FREEIMAGE_HEADERS}
    )
    LIST(APPEND LIBRARIES_OPTIMIZED ${FREEIMAGE_LIB})
    LIST(APPEND LIBRARIES_DEBUG ${FREEIMAGE_LIB})
    # source_group(AntTweakBar FILES 
    #   ${ANTTWEAKBAR_HEADERS}
    # )
  endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_FreeImage)
  if(FREEIMAGE_FOUND)
    _add_package_FreeImage()
  endif(FREEIMAGE_FOUND)
endmacro(_optional_package_FreeImage)

#####################################################################################
# Optional OptiX package
#
macro(_add_package_Optix)
  find_package(Optix)  
  if(OPTIX_FOUND)
      Message(STATUS "--> using package OptiX")
      add_definitions(-DNVP_SUPPORTS_OPTIX)
      include_directories(${OPTIX_INCLUDE_DIR})
      LIST(APPEND LIBRARIES_OPTIMIZED ${OPTIX_LIB} )
      LIST(APPEND LIBRARIES_DEBUG ${OPTIX_LIB} )
      LIST(APPEND PACKAGE_SOURCE_FILES ${OPTIX_HEADERS} )
      source_group(OPTIX FILES  ${OPTIX_HEADERS} )
      set( USING_OPTIX "YES")
 else()
     Message(STATUS "--> NOT using package OptiX")
 endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_Optix)
  if(OPTIX_FOUND)
    _add_package_Optix()
  endif(OPTIX_FOUND)
endmacro(_optional_package_Optix)

#####################################################################################
# Optional OptiX7 package
#
macro(_add_package_Optix7)
  find_package(Optix7)  
  if(OPTIX7_FOUND)
      Message(STATUS "--> using package OptiX7")
      add_definitions(-DNVP_SUPPORTS_OPTIX7)
      include_directories(${OPTIX7_INCLUDE_DIR})
      LIST(APPEND PACKAGE_SOURCE_FILES ${OPTIX7_HEADERS} )
      source_group(OPTIX FILES  ${OPTIX7_HEADERS} )
      set( USING_OPTIX7 "YES")
 else()
     Message(STATUS "--> NOT using package OptiX7")
 endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_Optix7)
  if(OPTIX7_FOUND)
    _add_package_Optix7()
  endif(OPTIX7_FOUND)
endmacro(_optional_package_Optix7)

#####################################################################################
# Optional VulkanSDK package
#
macro(_add_package_VulkanSDK)
  find_package(Vulkan REQUIRED
    COMPONENTS glslc glslangValidator shaderc_combined)
  if(Vulkan_FOUND)
      Message(STATUS "--> using package VulkanSDK (linking with ${Vulkan_LIBRARY})")
      get_directory_property(hasParent PARENT_DIRECTORY)
      if(hasParent)
        set( USING_VULKANSDK "YES" PARENT_SCOPE) # PARENT_SCOPE important to have this variable passed to parent. Here we want to notify that something used the Vulkan package
      endif()
      set( USING_VULKANSDK "YES")
      option(VK_ENABLE_BETA_EXTENSIONS "Enable beta extensions provided by the Vulkan SDK" ON)
      add_definitions(-DNVP_SUPPORTS_VULKANSDK)
      if(VK_ENABLE_BETA_EXTENSIONS)
        add_definitions(-DVK_ENABLE_BETA_EXTENSIONS)
      endif()
      add_definitions(-DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

      if(NOT VULKAN_BUILD_DEPENDENCIES)
        set(VULKAN_BUILD_DEPENDENCIES ON CACHE BOOL "Create dependencies on GLSL files")
      endif()

      set(VULKAN_HEADERS_OVERRIDE_INCLUDE_DIR CACHE PATH "Override for Vulkan headers, leave empty to use SDK")

      if (VULKAN_HEADERS_OVERRIDE_INCLUDE_DIR)
        set(vulkanHeaderDir ${VULKAN_HEADERS_OVERRIDE_INCLUDE_DIR})
      else()
        set(vulkanHeaderDir ${Vulkan_INCLUDE_DIR})
      endif()

      Message(STATUS "--> using Vulkan Headers from: ${vulkanHeaderDir}")
      include_directories(${vulkanHeaderDir})
      set( vulkanHeaderFiles 
        "${vulkanHeaderDir}/vulkan/vulkan_core.h")
      LIST(APPEND PACKAGE_SOURCE_FILES ${vulkanHeaderFiles})
      source_group(Vulkan FILES ${vulkanHeaderFiles})

      LIST(APPEND LIBRARIES_OPTIMIZED ${Vulkan_LIBRARY})
      LIST(APPEND LIBRARIES_DEBUG ${Vulkan_LIBRARY})

      # CMake 3.24+ finds glslangValidator and glslc for us.
      # On < 3.24, find it manually:
      if(${CMAKE_VERSION} VERSION_LESS "3.24.0")
        get_filename_component(_VULKAN_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
        find_file(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
          NAMES glslangValidator.exe glslangValidator
          PATHS ${_VULKAN_LIB_DIR}/../Bin)
        find_file(Vulkan_GLSLC_EXECUTABLE
          NAMES glslc.exe glslc
          PATHS ${_VULKAN_LIB_DIR}/../Bin)
      endif()
 else()
     Message(STATUS "--> NOT using package VulkanSDK")
 endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_VulkanSDK)
  if(USING_VULKANSDK AND NOT VULKAN_FOUND)
    _add_package_VulkanSDK()
  endif()
endmacro(_optional_package_VulkanSDK)

#####################################################################################
# Optional Glm package (Part of Vulkan SDK)
#
macro(_add_package_GLM)
  find_package(GLM)  
  if(GLM_FOUND)
    add_definitions(-DNVMATH_SUPPORTS_GLM)
    include_directories(${GLM_INCLUDE_DIRS})
  else()
    message("GLM not found. Check `GLM` with the installation of Vulkan SDK") 
  endif()
endmacro()

#####################################################################################
# Optional ShaderC package
#
macro(_add_package_ShaderC)
  _add_package_VulkanSDK()
  # Find the release shaderc libraries.
  # CMake 3.24 finds shaderc_combined for us, but we target CMake 3.10+.
  # Debug ShaderC isn't always installed on Windows or Linux.
  # On Windows, linking release shaderc with a debug app produces linker errors,
  # because they use different C runtime libraries (one uses /MT and the other
  # uses /MTd), and MSVC's linker prohibits mixing these in the same .exe.
  # So we use release shaderc_shared on Windows and release shaderc_combined
  # on Linux.
  if(NOT NVSHADERC_LIB)
    get_filename_component(_VULKAN_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
    if(WIN32)
      find_file(VULKANSDK_SHADERC_LIB
        NAMES shaderc_shared.lib
        PATHS ${_VULKAN_LIB_DIR})
      find_file(VULKANSDK_SHADERC_DLL
        NAMES shaderc_shared.dll
        PATHS ${_VULKAN_LIB_DIR}/../Bin)
      add_definitions(-DSHADERC_SHAREDLIB)
      if(NOT VULKANSDK_SHADERC_DLL)
        message(FATAL_ERROR "Windows platform requires VulkanSDK with shaderc_shared.lib/dll (since SDK 1.2.135.0)")
      endif()
    else()
      if(NOT Vulkan_shaderc_combined_LIBRARY)
        find_file(Vulkan_shaderc_combined_LIBRARY
          NAMES libshaderc_combined.a
          PATHS ${_VULKAN_LIB_DIR})
      endif()
      set(VULKANSDK_SHADERC_LIB ${Vulkan_shaderc_combined_LIBRARY})
    endif()
  endif()

  if(Vulkan_FOUND AND (VULKANSDK_SHADERC_LIB OR NVSHADERC_LIB))
      Message(STATUS "--> using package ShaderC")
      
      add_definitions(-DNVP_SUPPORTS_SHADERC)
      if (NVSHADERC_LIB)
        Message(STATUS "--> using NVShaderC LIB")
        add_definitions(-DNVP_SUPPORTS_NVSHADERC)
      endif()
      
      if (NVSHADERC_LIB)
        LIST(APPEND LIBRARIES_OPTIMIZED ${NVSHADERC_LIB})
        LIST(APPEND LIBRARIES_DEBUG ${NVSHADERC_LIB})
      else()
        LIST(APPEND LIBRARIES_OPTIMIZED ${VULKANSDK_SHADERC_LIB})
        LIST(APPEND LIBRARIES_DEBUG ${VULKANSDK_SHADERC_LIB})
      endif()
      if(hasParent)
        set( USING_SHADERC "YES" PARENT_SCOPE)
      else()
        set( USING_SHADERC "YES")
      endif()
  else()
      Message(STATUS "--> NOT using package ShaderC")
  endif() 
endmacro(_add_package_ShaderC)
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_ShaderC)
  if(USING_SHADERC)
    _add_package_ShaderC()
  endif(USING_SHADERC)
endmacro(_optional_package_ShaderC)

#####################################################################################
# Optional DirectX11 package
#
macro(_add_package_DirectX11)
  find_package(DirectX11)  
  if(DX11SDK_FOUND)
      Message(STATUS "--> using package DirectX 11")
      get_directory_property(hasParent PARENT_DIRECTORY)
      if(hasParent)
        set( USING_DIRECTX11 "YES" PARENT_SCOPE) # PARENT_SCOPE important to have this variable passed to parent. Here we want to notify that something used the DX11 package
      else()
        set( USING_DIRECTX11 "YES")
      endif()
      add_definitions(-DNVP_SUPPORTS_DIRECTX11)
      include_directories(${DX11SDK_INCLUDE_DIR})
      LIST(APPEND LIBRARIES_OPTIMIZED ${DX11SDK_D3D_LIBRARIES})
      LIST(APPEND LIBRARIES_DEBUG ${DX11SDK_D3D_LIBRARIES})
 else()
     Message(STATUS "--> NOT using package DirectX11")
 endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_DirectX11)
  if(USING_DIRECTX11)
    _add_package_DirectX11()
  endif(USING_DIRECTX11)
endmacro(_optional_package_DirectX11)

#####################################################################################
# Optional DirectX12 package
#
macro(_add_package_DirectX12)
  if(WIN32)
    Message(STATUS "--> using package DirectX 12")
    get_directory_property(hasParent PARENT_DIRECTORY)
    if(hasParent)
      set( USING_DIRECTX12 "YES" PARENT_SCOPE) # PARENT_SCOPE important to have this variable passed to parent. Here we want to notify that something used the DX12 package
    else()
      set( USING_DIRECTX12 "YES")
    endif()
    add_definitions(-DNVP_SUPPORTS_DIRECTX12)
    include_directories(${BASE_DIRECTORY}/nvpro_core/third_party/dxc/Include)
    include_directories(${BASE_DIRECTORY}/nvpro_core/third_party/dxh/include/directx)
    LIST(APPEND LIBRARIES_OPTIMIZED dxgi.lib d3d12.lib)
    LIST(APPEND LIBRARIES_DEBUG dxgi.lib d3d12.lib)
  endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_DirectX12)
  if(USING_DIRECTX12)
    _add_package_DirectX12()
  endif(USING_DIRECTX12)
endmacro(_optional_package_DirectX12)


#####################################################################################
# Optional CUDA package
# see https://cmake.org/cmake/help/v3.3/module/FindCUDA.html
#
macro(_add_package_Cuda)
  if(CUDA_TOOLKIT_ROOT_DIR)
    string(REPLACE "\\" "/" CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR})
  endif()
  find_package(CUDA QUIET)
  if(CUDA_FOUND)
      add_definitions("-DCUDA_PATH=R\"(${CUDA_TOOLKIT_ROOT_DIR})\"")
      Message(STATUS "--> using package CUDA (${CUDA_VERSION})")
      add_definitions(-DNVP_SUPPORTS_CUDA)
      include_directories(${CUDA_INCLUDE_DIRS})
      LIST(APPEND LIBRARIES_OPTIMIZED ${CUDA_LIBRARIES} )
      LIST(APPEND LIBRARIES_DEBUG ${CUDA_LIBRARIES} )
      # STRANGE: default CUDA package finder from cmake doesn't give anything to find cuda.lib
      if(WIN32)
        if((ARCH STREQUAL "x86"))
          LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32/cuda.lib" )
          LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32/cudart.lib" )
          LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32/cuda.lib" )
          LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32/cudart.lib" )
        else()
          LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cuda.lib" )
          LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cudart.lib" )
          LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/nvrtc.lib" )
          LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cuda.lib" )
          LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cudart.lib" )
          LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/nvrtc.lib" )
        endif()
      else()
        LIST(APPEND LIBRARIES_DEBUG "libcuda.so" )
        LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib64/libcudart.so" )
        LIST(APPEND LIBRARIES_DEBUG "${CUDA_TOOLKIT_ROOT_DIR}/lib64/libnvrtc.so" )
        LIST(APPEND LIBRARIES_OPTIMIZED "libcuda.so" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib64/libcudart.so" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDA_TOOLKIT_ROOT_DIR}/lib64/libnvrtc.so" )
      endif()
      #LIST(APPEND PACKAGE_SOURCE_FILES ${CUDA_HEADERS} ) Not available anymore with cmake 3.3... we might have to list them by hand
      # source_group(CUDA FILES ${CUDA_HEADERS} )  Not available anymore with cmake 3.3
 else()
     Message(STATUS "--> NOT using package CUDA") 
 endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_Cuda)
  if(CUDA_FOUND)
    _add_package_Cuda()
  endif(CUDA_FOUND)
endmacro(_optional_package_Cuda)

#####################################################################################
# NVToolsExt
#
macro(_add_package_NVToolsExt)
  if(SUPPORT_NVTOOLSEXT)
    Message(STATUS "--> using package NVToolsExt")
    get_directory_property(hasParent PARENT_DIRECTORY)
    if(hasParent)
      set( USING_NVTOOLSEXT "YES" PARENT_SCOPE)
    else()
      set( USING_NVTOOLSEXT "YES")
    endif()

    add_definitions(-DNVP_SUPPORTS_NVTOOLSEXT)
  endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_NVToolsExt)
  if(USING_NVTOOLSEXT)
    _add_package_NVToolsExt()
  endif()
endmacro(_optional_package_NVToolsExt)


#####################################################################################
# NVML
# Note that NVML often needs to be delay-loaded on Windows using
# set_target_properties(${PROJNAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:nvml.dll")!
#
macro(_add_package_NVML)
  message(STATUS "--> using package NVML")
  find_package(NVML)
  if(NVML_FOUND)
    add_definitions(-DNVP_SUPPORTS_NVML)
    include_directories(${NVML_INCLUDE_DIRS})
    LIST(APPEND LIBRARIES_OPTIMIZED ${NVML_LIBRARIES})
    LIST(APPEND LIBRARIES_DEBUG ${NVML_LIBRARIES})
  endif()
endmacro()

#####################################################################################
# nSight Aftermath
macro(_add_package_NsightAftermath)
  if(SUPPORT_AFTERMATH)
     message(STATUS "--> using package Aftermmath")
	if (NOT DEFINED NSIGHT_AFTERMATH_SDK)
		if (DEFINED ENV{NSIGHT_AFTERMATH_SDK})
		  set(NSIGHT_AFTERMATH_SDK  $ENV{NSIGHT_AFTERMATH_SDK} CACHE STRING "Path to the Aftermath SDK")
		else()
		  message(WARNING "--> Download nSight Aftermath from from https://developer.nvidia.com/nsight-aftermath")
		  message(WARNING "--> Unzip it to a folder. Then set NSIGHT_AFTERMATH_SDK env. variable to the top of the unpacked aftermath directory before running CMake.")
		endif()
	endif()

	message(STATUS "--> Looking for nSight Aftermath at: ${NSIGHT_AFTERMATH_SDK}")

    find_package(NsightAftermath)

    if(NsightAftermath_FOUND)
        add_definitions(-DNVVK_SUPPORTS_AFTERMATH)

        include_directories(${NsightAftermath_INCLUDE_DIRS})
        LIST(APPEND LIBRARIES_OPTIMIZED ${NsightAftermath_LIBRARIES})
        LIST(APPEND LIBRARIES_DEBUG ${NsightAftermath_LIBRARIES})
    endif(NsightAftermath_FOUND)
  endif(SUPPORT_AFTERMATH)

  list(APPEND COMMON_SOURCE_FILES "${BASE_DIRECTORY}/nvpro_core/nvvk/nsight_aftermath_vk.cpp")

endmacro(_add_package_NsightAftermath)

#####################################################################################
# KTX dependencies (Zstandard and Basis Universal; also adds Zlib)
# By default, the nv_ktx KTX reader/writer can only handle files
# without supercompression.
# Add _add_package_KTX() to your project to also build, include, and enable
# compression libraries for the Zstandard, Basis Universal, and Zlib
# supercompression modes.
# This is not included by default so that projects that don't use these formats
# don't have to spend time compiling these dependencies.
macro(_add_package_KTX)
  # Zlib
  _add_package_ZLIB()
  
  # Zstandard
  set(_ZSTD_DIR ${BASE_DIRECTORY}/nvpro_core/third_party/zstd)
  if(NOT TARGET libzstd_static AND EXISTS ${_ZSTD_DIR})
    if(NOT EXISTS ${_ZSTD_DIR}/lib/zstd.h)
      message(WARNING "It looks like the zstd submodule hasn't been downloaded; try running git submodule update --init --recursive in the nvpro_core directory.")
    else()
      set(ZSTD_BUILD_PROGRAMS OFF)
      set(ZSTD_BUILD_SHARED OFF)
      set(ZSTD_BUILD_STATIC ON)
      set(ZSTD_USE_STATIC_RUNTIME ON)
      add_subdirectory(${_ZSTD_DIR}/build/cmake ${CMAKE_BINARY_DIR}/zstd)
      target_sources(libzstd_static INTERFACE $<BUILD_INTERFACE:${_ZSTD_DIR}/lib/zstd.h>)
      target_include_directories(libzstd_static INTERFACE $<BUILD_INTERFACE:${_ZSTD_DIR}/lib>)
    endif()
  endif()
  if(TARGET libzstd_static)
    message(STATUS "--> using package Zstd (from KTX)")
    # Make targets linking with libzstd_static compile with NVP_SUPPORTS_ZSTD
    target_compile_definitions(libzstd_static INTERFACE NVP_SUPPORTS_ZSTD)
    LIST(APPEND LIBRARIES_OPTIMIZED libzstd_static)
    LIST(APPEND LIBRARIES_DEBUG libzstd_static)
    set_target_properties(libzstd_static clean-all uninstall PROPERTIES FOLDER "ThirdParty")
    # Exclude Zstd's clean-all and uninstall targets from ALL_BUILD and INSTALL;
    # otherwise, it'll fail when building everything.
    set_target_properties(clean-all uninstall PROPERTIES
      EXCLUDE_FROM_ALL 1
      EXCLUDE_FROM_DEFAULT_BUILD 1
    )
  else()
    message(STATUS "--> NOT using package Zstd (from KTX)") 
  endif()
  
  # Basis Universal
  set(_BASISU_DIR ${BASE_DIRECTORY}/nvpro_core/third_party/basis_universal)
  if(NOT TARGET basisu AND EXISTS ${_BASISU_DIR})
    if(NOT EXISTS ${_BASISU_DIR}/transcoder)
      message(WARNING "It looks like the basis_universal submodule hasn't been downloaded; try running git submodule update --init --recursive in the nvpro_core directory.")
    else()
      file(GLOB _BASISU_FILES "${_BASISU_DIR}/transcoder/*.*" "${_BASISU_DIR}/encoder/*.*")
      add_library(basisu STATIC "${_BASISU_FILES}")
      target_include_directories(basisu INTERFACE "${_BASISU_DIR}/transcoder" "${_BASISU_DIR}/encoder")
    endif()
  endif()
  if(TARGET basisu)
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
    LIST(APPEND LIBRARIES_OPTIMIZED basisu)
    LIST(APPEND LIBRARIES_DEBUG basisu)
    set_property(TARGET basisu PROPERTY FOLDER "ThirdParty")
    
    # Set up linking between basisu and its dependencies, so that we always get
    # a correct linking order on Linux:
    if(TARGET libzstd_static)
      target_link_libraries(basisu PUBLIC libzstd_static)
    else()
      # If Zstandard isn't included, also turn off Zstd support in Basis:
      target_compile_definitions(basisu PRIVATE BASISD_SUPPORT_KTX2_ZSTD=0)
    endif()
    if(TARGET zlibstatic)
      target_link_libraries(basisu PUBLIC zlibstatic)
    endif()
  endif()
endmacro()

#####################################################################################
# Omniverse
#
macro(_add_package_Omniverse)
  Message(STATUS "--> using package Omniverse")
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set( USING_OMNIVERSE "YES" PARENT_SCOPE)
  else()
    set( USING_OMNIVERSE "YES")
  endif()
  find_package(OmniClient REQUIRED)
  find_package(USD)
  find_package(KitSDK)
  
  set(Python3_ROOT_DIR "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/python")
  # Stop looking fore newer versions in other locations
  set(Python3_FIND_STRATEGY LOCATION)
  # On Windows, ignore a potentially preinstalled newer version of Python
  set(Python3_FIND_REGISTRY NEVER)

  find_package(Python3 3.7 EXACT REQUIRED COMPONENTS Development Interpreter)
  
  #message("Python3_ROOT_DIR " ${Python3_ROOT_DIR})
  
  add_compile_definitions(TBB_USE_DEBUG=0)
  
endmacro()

# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_Omniverse)
  if(USING_OMNIVERSE)
    _add_package_Omniverse()
  endif()
endmacro(_optional_package_Omniverse)

#####################################################################################
# Generate PTX files
# NVCUDA_COMPILE_PTX( SOURCES file1.cu file2.cu TARGET_PATH <path where ptxs should be stored> GENERATED_FILES ptx_sources NVCC_OPTIONS -arch=sm_20)
# Generates ptx files for the given source files. ptx_sources will contain the list of generated files.
function(nvcuda_compile_ptx)
  set(options "")
  set(oneValueArgs TARGET_PATH GENERATED_FILES)
  set(multiValueArgs NVCC_OPTIONS SOURCES)
  CMAKE_PARSE_ARGUMENTS(NVCUDA_COMPILE_PTX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
Message(STATUS "NVCUDA_COMPILE_PTX ${options} ${oneValueArgs} ${multiValueArgs} ")

  # Match the bitness of the ptx to the bitness of the application
  set( MACHINE "--machine=32" )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( MACHINE "--machine=64" )
  endif()
  
  # Custom build rule to generate ptx files from cuda files
  FOREACH( input ${NVCUDA_COMPILE_PTX_SOURCES} )
    get_filename_component( input_we ${input} NAME_WE )
    
    # generate the *.ptx files inside "ptx" folder inside the executable's output directory.
    set( output "${NVCUDA_COMPILE_PTX_TARGET_PATH}/${input_we}.ptx" )
    LIST( APPEND PTX_FILES  ${output} )
    
    message(STATUS "${CUDA_NVCC_EXECUTABLE} ${MACHINE} --ptx ${NVCUDA_COMPILE_PTX_NVCC_OPTIONS} ${input} -o ${output} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}")
    
    add_custom_command(
      OUTPUT  ${output}
      DEPENDS ${input}
      COMMAND ${CUDA_NVCC_EXECUTABLE} ${MACHINE} --ptx ${NVCUDA_COMPILE_PTX_NVCC_OPTIONS} ${input} -o ${output} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMAND ${CMAKE_COMMAND} -E echo ${NVCUDA_COMPILE_PTX_TARGET_PATH}
    )
  ENDFOREACH( )
  
  set(${NVCUDA_COMPILE_PTX_GENERATED_FILES} ${PTX_FILES} PARENT_SCOPE)
endfunction()

#####################################################################################
# Generate CUBIN files
# NVCUDA_COMPILE_CUBIN( SOURCES file1.cu file2.cu TARGET_PATH <path where cubin's should be stored> GENERATED_FILES cubin_sources NVCC_OPTIONS -arch=sm_20)
# Generates cubin files for the given source files. cubin_sources will contain the list of generated files.
function(nvcuda_compile_cubin)
  set(options "")
  set(oneValueArgs TARGET_PATH GENERATED_FILES)
  set(multiValueArgs NVCC_OPTIONS SOURCES)
  CMAKE_PARSE_ARGUMENTS(NVCUDA_COMPILE_CUBIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
Message(STATUS "NVCUDA_COMPILE_CUBIN ${options} ${oneValueArgs} ${multiValueArgs} ")

  # Match the bitness of the cubin to the bitness of the application
  set( MACHINE "--machine=32" )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( MACHINE "--machine=64" )
  endif()
  
  # Custom build rule to generate cubin files from cuda files
  FOREACH( input ${NVCUDA_COMPILE_CUBIN_SOURCES} )
    get_filename_component( input_we ${input} NAME_WE )
    
    # generate the *.cubin files inside "cubin" folder inside the executable's output directory.
    set( output "${NVCUDA_COMPILE_CUBIN_TARGET_PATH}/${input_we}.cubin" )
    LIST( APPEND CUBIN_FILES  ${output} )
    
    message(STATUS "${CUDA_NVCC_EXECUTABLE} ${MACHINE} --cubin ${NVCUDA_COMPILE_CUBIN_NVCC_OPTIONS} ${input} -o ${CMAKE_CURRENT_SOURCE_DIR}/${output}")
    message(STATUS "WORKING_DIRECTORY: ${CMAKE_CURRENT_SOURCE_DIR}")
    
    add_custom_command(
      OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/${output}
      DEPENDS ${input}
      COMMAND ${CUDA_NVCC_EXECUTABLE} ${MACHINE} --cubin ${NVCUDA_COMPILE_CUBIN_NVCC_OPTIONS} ${input} -o ${CMAKE_CURRENT_SOURCE_DIR}/${output} 
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  ENDFOREACH( )
  
  set(${NVCUDA_COMPILE_CUBIN_GENERATED_FILES} ${CUBIN_FILES} PARENT_SCOPE)
endfunction()

#####################################################################################
# Macro to setup output directories 
macro(_set_target_output _PROJNAME)
  set_target_properties(${_PROJNAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_PATH}/$<CONFIG>/"
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_PATH}/$<CONFIG>/"
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_PATH}/$<CONFIG>/"
  )
  # Most installed projects place their files under a folder with the same name as
  # the project. On Linux, this conflicts with the name of the executable. We add
  # the following suffix to avoid this.
  # Note that setting CMAKE_EXECUTABLE_SUFFIX is not the solution: it causes
  # CUDA try_compile to break inside build_all, because CMake thinks the CUDA
  # executables end with CMAKE_EXECUTABLE_SUFFIX.
  if(UNIX)
    set_target_properties(${_PROJNAME} PROPERTIES SUFFIX "_app")
  endif()
endmacro()

#####################################################################################
# Macro that copies various binaries that need to be close to the exe files
#
macro(_finalize_target _PROJNAME)

  _set_target_output(${_PROJNAME})

  if(NOT UNIX)
    if(USING_SHADERC AND VULKANSDK_SHADERC_DLL)
      _copy_files_to_target( ${_PROJNAME} "${VULKANSDK_SHADERC_DLL}")
      install(FILES ${VULKANSDK_SHADERC_DLL} CONFIGURATIONS Release DESTINATION bin_${ARCH})
      install(FILES ${VULKANSDK_SHADERC_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
    endif()
  endif()
  if(FREEIMAGE_FOUND)
    _copy_files_to_target( ${_PROJNAME} "${FREEIMAGE_DLL}")
    install(FILES ${FREEIMAGE_DLL} CONFIGURATIONS Release DESTINATION bin_${ARCH})
    install(FILES ${FREEIMAGE_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  if(OPTIX_FOUND)
    _copy_files_to_target( ${_PROJNAME} "${OPTIX_DLL}")
    install(FILES ${OPTIX_DLL} CONFIGURATIONS Release DESTINATION bin_${ARCH})
    install(FILES ${OPTIX_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  if(PERFWORKS_DLL)
    _copy_files_to_target( ${_PROJNAME} "${PERFWORKS_DLL}")
    install(FILES ${PERFWORKS_DLL} CONFIGURATIONS Release DESTINATION bin_${ARCH})
    install(FILES ${PERFWORKS_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  if(NsightAftermath_FOUND)
    _copy_files_to_target( ${_PROJNAME} "${NsightAftermath_DLLS}")
    install(FILES ${NsightAftermath_DLLS} CONFIGURATIONS Release DESTINATION bin_${ARCH})
    install(FILES ${NsightAftermath_DLLS} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  install(TARGETS ${_PROJNAME} CONFIGURATIONS Release DESTINATION bin_${ARCH})
  install(TARGETS ${_PROJNAME} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
endmacro()

#####################################################################################
# Macro to add custom build for SPIR-V, with additional arbitrary glslangvalidator
# flags (run glslangvalidator --help for a list of possible flags).
# Inputs:
# _SOURCE can be more than one file (.vert + .frag)
# _OUTPUT is the .spv file, resulting from the linkage
# _FLAGS are the flags to add to the command line
# Outputs:
# SOURCE_LIST has _SOURCE appended to it
# OUTPUT_LIST has _OUTPUT appended to it
#
macro(_compile_GLSL_flags _SOURCE _OUTPUT _FLAGS SOURCE_LIST OUTPUT_LIST)
  if(NOT DEFINED VULKAN_TARGET_ENV)
    set(VULKAN_TARGET_ENV vulkan1.1)
  endif()
  LIST(APPEND ${SOURCE_LIST} ${_SOURCE})
  LIST(APPEND ${OUTPUT_LIST} ${_OUTPUT})
  if(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    set(_COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} --target-env ${VULKAN_TARGET_ENV} -o ${_OUTPUT} ${_FLAGS} ${_SOURCE})
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${_OUTPUT}
      COMMAND echo ${_COMMAND}
      COMMAND ${_COMMAND}
      MAIN_DEPENDENCY ${_SOURCE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      )
  else(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    MESSAGE(WARNING "could not find Vulkan_GLSLANG_VALIDATOR_EXECUTABLE to compile shaders")
  endif(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
endmacro()

#####################################################################################
# Macro to add custom build for SPIR-V, with debug information (glslangvalidator's -g flag)
# Inputs:
# _SOURCE can be more than one file (.vert + .frag)
# _OUTPUT is the .spv file, resulting from the linkage
# Outputs:
# SOURCE_LIST has _SOURCE appended to it
# OUTPUT_LIST has _OUTPUT appended to it
#
macro(_compile_GLSL _SOURCE _OUTPUT SOURCE_LIST OUTPUT_LIST)
  _compile_GLSL_flags(${_SOURCE} ${_OUTPUT} "-g" ${SOURCE_LIST} ${OUTPUT_LIST})
endmacro()

#####################################################################################
# Macro to add custom build for SPIR-V, without debug information
# Inputs:
# _SOURCE can be more than one file (.vert + .frag)
# _OUTPUT is the .spv file, resulting from the linkage
# Outputs:
# SOURCE_LIST has _SOURCE appended to it
# OUTPUT_LIST has _OUTPUT appended to it
#
macro(_compile_GLSL_no_debug_info _SOURCE _OUTPUT SOURCE_LIST OUTPUT_LIST)
  _compile_GLSL_flags(${_SOURCE} ${_OUTPUT} "" ${SOURCE_LIST} ${OUTPUT_LIST})
endmacro()

#####################################################################################
# This is the rest of the cmake code that the project needs to call
# used by the samples via _add_nvpro_core_lib and by nvpro_core
#
macro(_process_shared_cmake_code)
  
  set(PLATFORM_LIBRARIES)
  
  if (USING_DIRECTX11)
    LIST(APPEND PLATFORM_LIBRARIES ${DX11SDK_D3D_LIBRARIES})
  endif()
  
  if (USING_DIRECTX12)
    LIST(APPEND PLATFORM_LIBRARIES ${DX12SDK_D3D_LIBRARIES})
  endif()
  
  if (USING_OPENGL)
    LIST(APPEND PLATFORM_LIBRARIES ${OPENGL_LIBRARY})
  endif()
  
  if (USING_VULKANSDK)
    LIST(APPEND PLATFORM_LIBRARIES ${Vulkan_LIBRARIES})
  endif()
  
  set(COMMON_SOURCE_FILES)
  LIST(APPEND COMMON_SOURCE_FILES
      ${BASE_DIRECTORY}/nvpro_core/nvp/resources.h
      ${BASE_DIRECTORY}/nvpro_core/nvp/resources.rc
      # Add this cpp file into the individual projects such that each one gets a unique g_ProjectName definition
      # This will allow to move more nvpro_core code relying on PROJECT_NAME into .cpp files.
      ${BASE_DIRECTORY}/nvpro_core/nvp/perproject_globals.cpp
  )
   
  if(UNIX)
    LIST(APPEND PLATFORM_LIBRARIES "Xxf86vm")

	#Work around some obscure bug where samples would crash in std::filesystem::~path() when
	#multiple GCC versions are installed. Details under:
	#https://stackoverflow.com/questions/63902528/program-crashes-when-filesystempath-is-destroyed
	#
	#When the GCC version is less than 9, explicitly link against libstdc++fs to
	#prevent accidentally picking up GCC9's implementation and thus create an ABI
	#incompatibility that results in crashes
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
		 LIST(APPEND PLATFORM_LIBRARIES stdc++fs)
	endif()
  endif()
endmacro(_process_shared_cmake_code)

#####################################################################################
# This is the rest of the cmake code that the samples needs to call in order
# - to add the nvpro_core library (needed by any sample)
# - this part will also setup a directory where to find some downloaded resources
# - add optional packages
# - and will then call another shared cmake macro : _process_shared_cmake_code
#
macro(_add_nvpro_core_lib)
  #-----------------------------------------------------------------------------------
  # now we have added some packages, we can guess more
  # on what is needed overall for the shared library

  # build_all adds individual samples, and then at the end 
  # the nvpro_core itself, otherwise we build a single 
  # sample which does need nvpro_core added here

  if(NOT HAS_NVPRO_CORE)
    add_subdirectory(${BASE_DIRECTORY}/nvpro_core ${CMAKE_BINARY_DIR}/nvpro_core)
  endif()

  #-----------------------------------------------------------------------------------
  # optional packages we don't need, but might be needed by other samples
  Message(STATUS " Packages needed for nvpro_core lib compat:")
  _optional_package_OpenGL()
  _optional_package_VulkanSDK()

  # finish with another part (also used by cname for the nvpro_core)
  _process_shared_cmake_code()

  # added uncondtionally here, since it will also do something in case
  # SUPPORT_AFTERMATH is OFF
  _add_package_NsightAftermath()

  # putting this into one of the other branches didn't work
  if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
  else()
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
  endif(WIN32)
  add_definitions(-DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
endmacro()

#####################################################################################
# Finds OpenMP. This is a backwards-compatible alias for a function that was
# previously more complex.
macro(_find_package_OpenMP)
  find_package(OpenMP)
endmacro()



# -------------------------------------------------------------------------------------------------
# function that copies a list of files into the target directory
#
#   target_copy_to_output_dir(TARGET foo
#       [RELATIVE <path_prefix>]                                # allows to keep the folder structure starting from this level
#       FILES <absolute_file_path> [<absolute_file_path>]
#       )
#
function(target_copy_to_output_dir)
    set(options)
    set(oneValueArgs TARGET RELATIVE DEST_SUBFOLDER)
    set(multiValueArgs FILES)
    cmake_parse_arguments(TARGET_COPY_TO_OUTPUT_DIR "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    foreach(_ELEMENT ${TARGET_COPY_TO_OUTPUT_DIR_FILES} )

        # handle absolute and relative paths
        if(TARGET_COPY_TO_OUTPUT_DIR_RELATIVE)
            set(_SOURCE_FILE ${TARGET_COPY_TO_OUTPUT_DIR_RELATIVE}/${_ELEMENT})
            set(_FOLDER_PATH ${_ELEMENT})
        else()
            set(_SOURCE_FILE ${_ELEMENT})
            get_filename_component(_FOLDER_PATH ${_ELEMENT} NAME)
            set (_ELEMENT "")
        endif()

        # handle directories and files slightly different
        if(IS_DIRECTORY ${_SOURCE_FILE})
            if(MDL_LOG_FILE_DEPENDENCIES)
                MESSAGE(STATUS "- folder to copy: ${_SOURCE_FILE}")
            endif()
            add_custom_command(
                TARGET ${TARGET_COPY_TO_OUTPUT_DIR_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${_SOURCE_FILE} $<TARGET_FILE_DIR:${TARGET_COPY_TO_OUTPUT_DIR_TARGET}>/${TARGET_COPY_TO_OUTPUT_DIR_DEST_SUBFOLDER}${_FOLDER_PATH}
            )
        else()   
            if(MDL_LOG_FILE_DEPENDENCIES)
                MESSAGE(STATUS "- file to copy:   ${_SOURCE_FILE}")
            endif()
            add_custom_command(
                TARGET ${TARGET_COPY_TO_OUTPUT_DIR_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_SOURCE_FILE} $<TARGET_FILE_DIR:${TARGET_COPY_TO_OUTPUT_DIR_TARGET}>/${TARGET_COPY_TO_OUTPUT_DIR_DEST_SUBFOLDER}${_ELEMENT}
            )
        endif()
    endforeach()
endfunction()
