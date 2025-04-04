#*****************************************************************************
# Copyright 2020-2025 NVIDIA Corporation. All rights reserved.
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
# Allow target_link_libraries() to link with targets in other directories;
# this fixes cases where two samples use KTX.
if(POLICY CMP0079)
  cmake_policy(SET CMP0079 NEW)
endif()


# Set the C/C++ specified in the projects as requirements
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Include symbols in executables for better stack trace information on Linux
set(CMAKE_ENABLE_EXPORTS ON)

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

  # Enable correct C++ version macros
  string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
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

set(ARCH "x64" CACHE STRING "CPU Architecture")

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
  file(RELATIVE_PATH TO_CURRENT_SOURCE_DIR "${OUTPUT_PATH}/config" "${CMAKE_CURRENT_SOURCE_DIR}")
  file(RELATIVE_PATH TO_DOWNLOAD_TARGET_DIR "${OUTPUT_PATH}/config" "${DOWNLOAD_TARGET_DIR}")
  file(RELATIVE_PATH TO_NVPRO_CORE_DIR "${OUTPUT_PATH}/config" "${BASE_DIRECTORY}/nvpro_core")
  
  add_definitions(-DPROJECT_NAME="${name}")
  add_definitions(-DPROJECT_RELDIRECTORY="${TO_CURRENT_SOURCE_DIR}/")
  add_definitions(-DPROJECT_DOWNLOAD_RELDIRECTORY="${TO_DOWNLOAD_TARGET_DIR}/")
  add_definitions(-DPROJECT_NVPRO_CORE_RELDIRECTORY="${TO_NVPRO_CORE_DIR}/")
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
# Downloads the URL to FILENAME and extracts its content if EXTRACT option is present
# ZIP files should have a folder of the name of the archive
# - ex. foo.zip -> foo/<data>
# Arguments:
#  FILENAMES   : all filenames to download
#  URLS        : if present, a custom download URL for each FILENAME.
#                If only one FILENAME is provided, a list of alternate download locations.
#                Defaults to ${DOWNLOAD_SITE}${SOURCE_DIR}/${FILENAME}.
#  EXTRACT     : if present, will extract the content of the file
#  NOINSTALL   : if present, will not make files part of install
#  INSTALL_DIR : folder for the 'install' build, default is 'media' next to the executable
#  TARGET_DIR  : folder where to download to, default is {DOWNLOAD_TARGET_DIR}
#  SOURCE_DIR  : folder on server, if not present 'scenes'
#
# Examples:
# download_files(FILENAMES sample1.zip EXTRACT) 
# download_files(FILENAMES env.hdr)
# download_files(FILENAMES zlib.zip EXTRACT TARGET_DIR ${BASE_DIRECTORY}/blah SOURCE_DIR /libraries NOINSTALL) 
# 
function(download_files)
  set(options EXTRACT NOINSTALL)
  set(oneValueArgs INSTALL_DIR SOURCE_DIR TARGET_DIR)
  set(multiValueArgs FILENAMES URLS)
  cmake_parse_arguments(DOWNLOAD_FILES  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if(NOT DEFINED DOWNLOAD_FILES_INSTALL_DIR)
    set(DOWNLOAD_FILES_INSTALL_DIR "media")
  endif()
  if(NOT DEFINED DOWNLOAD_FILES_SOURCE_DIR)
    set(DOWNLOAD_FILES_SOURCE_DIR "")
  endif()
  if(NOT DEFINED DOWNLOAD_FILES_TARGET_DIR)
    set(DOWNLOAD_FILES_TARGET_DIR ${DOWNLOAD_TARGET_DIR})
  endif()

  # Check each file to download
  # In CMake 3.17+, we can change this to _FILENAME_URL IN ZIP_LISTS DOWNLOAD_FILES_FILENAMES DOWNLOAD_FILES_URLS
  list(LENGTH DOWNLOAD_FILES_FILENAMES _NUM_DOWNLOADS)
  math(EXPR _DOWNLOAD_IDX_MAX "${_NUM_DOWNLOADS}-1")
  foreach(_DOWNLOAD_IDX RANGE ${_DOWNLOAD_IDX_MAX})
    list(GET DOWNLOAD_FILES_FILENAMES ${_DOWNLOAD_IDX} FILENAME)
    set(_TARGET_FILENAME ${DOWNLOAD_FILES_TARGET_DIR}/${FILENAME})
    
    set(_DO_DOWNLOAD ON)
    if(EXISTS ${_TARGET_FILENAME})
      file(SIZE ${_TARGET_FILENAME} _FILE_SIZE)
      if(${_FILE_SIZE} GREATER 0)
        set(_DO_DOWNLOAD OFF)
      endif()
    endif()
    
    if(_DO_DOWNLOAD)
      if(DOWNLOAD_FILES_URLS AND (_NUM_DOWNLOADS GREATER 1)) # One URL per file
        list(GET DOWNLOAD_FILES_URLS ${_DOWNLOAD_IDX} _DOWNLOAD_URLS)
      elseif(DOWNLOAD_FILES_URLS) # One file, multiple URLs
        set(_DOWNLOAD_URLS ${DOWNLOAD_FILES_URLS})
      else()
        set(_DOWNLOAD_URLS ${DOWNLOAD_SITE}${DOWNLOAD_FILES_SOURCE_DIR}/${FILENAME})
      endif()
      foreach(_DOWNLOAD_URL ${_DOWNLOAD_URLS})
        message(STATUS "Downloading ${_DOWNLOAD_URL} to ${_TARGET_FILENAME}")

        file(DOWNLOAD ${_DOWNLOAD_URL} ${_TARGET_FILENAME}
          SHOW_PROGRESS
          STATUS _DOWNLOAD_STATUS)

        # Check whether the download succeeded. _DOWNLOAD_STATUS is a list of
        # length 2; element 0 is the return value (0 == no error), element 1 is
        # a string value for the error.
        list(GET _DOWNLOAD_STATUS 0 _DOWNLOAD_STATUS_CODE)
        if(${_DOWNLOAD_STATUS_CODE} EQUAL 0)
          break() # Successful download!
        else()
          list(GET _DOWNLOAD_STATUS 1 _DOWNLOAD_STATUS_MESSAGE)
          # CMake usually creates a 0-byte file in this case. Remove it:
          file(REMOVE ${_TARGET_FILENAME})
          message(WARNING "Download of ${_DOWNLOAD_URL} to ${_TARGET_FILENAME} failed with code ${_DOWNLOAD_STATUS_CODE}: ${_DOWNLOAD_STATUS_MESSAGE}")
        endif()
      endforeach()

      if(NOT EXISTS ${_TARGET_FILENAME})
        message(FATAL_ERROR "All possible downloads to ${_TARGET_FILENAME} failed. See above warnings for more info.")
      endif()

      # Extracting the ZIP file
      if(DOWNLOAD_FILES_EXTRACT)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${_TARGET_FILENAME}
                      WORKING_DIRECTORY ${DOWNLOAD_FILES_TARGET_DIR})
        # We could use ARCHIVE_EXTRACT instead, but it needs CMake 3.18+:
        # file(ARCHIVE_EXTRACT INPUT ${_TARGET_FILENAME}
        #      DESTINATION ${DOWNLOAD_FILES_TARGET_DIR})
      endif()
    endif()

    # Installing the files or directory
    if (NOT DOWNLOAD_FILES_NOINSTALL)
      if(DOWNLOAD_FILES_EXTRACT)
       get_filename_component(FILE_DIR ${FILENAME} NAME_WE)
       install(DIRECTORY ${DOWNLOAD_FILES_TARGET_DIR}/${FILE_DIR} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION "bin_${ARCH}/${DOWNLOAD_FILES_INSTALL_DIR}")
       install(DIRECTORY ${DOWNLOAD_FILES_TARGET_DIR}/${FILE_DIR} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${DOWNLOAD_FILES_INSTALL_DIR}")
      else()
       install(FILES ${TARGET_FILENAME} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION "bin_${ARCH}/${DOWNLOAD_FILES_INSTALL_DIR}")
       install(FILES ${TARGET_FILENAME} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${DOWNLOAD_FILES_INSTALL_DIR}")
      endif()
    endif()
  endforeach()
endfunction()

#####################################################################################
# Downloads and extracts a package of source code and places it in
# downloaded_resources, if it doesn't already exist there. By default, downloads
# from the nvpro-samples server.
# Sets the variable in the DIR argument to its location.
# If it doesn't exist and couldn't be found, produces a fatal error.
# This is intended as a sort of lightweight package manager, for packages that
# are used by 2 or fewer samples, can be downloaded without authentication, and
# are not fundamental graphics APIs (like DirectX 12 or OptiX).
#
# Arguments:
#  NAME     : The name of the package to find. E.g. NVAPI looks for
#             a folder named NVAPI or downloads NVAPI.zip.
#  URLS     : Optional path to an archive to download. By default, this
#             downloads from ${DOWNLOAD_SITE}/libraries/${NAME}-${VERSION}.zip.
#             If more than one URL is specified, tries them in turn until one works.
#  VERSION  : The package's version number, like "555.0.0" or "1.1.0".
#  LOCATION : Will be set to the package's directory.
#
function(download_package)
  set(oneValueArgs NAME VERSION LOCATION)
  set(multiValueArgs URLS)
  cmake_parse_arguments(DOWNLOAD_PACKAGE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  
  set(_TARGET_DIR ${CMAKE_BINARY_DIR}/_deps/${DOWNLOAD_PACKAGE_NAME}-${DOWNLOAD_PACKAGE_VERSION})
  if(EXISTS ${_TARGET_DIR})
    # An empty directory is not a valid cache entry; that usually indicates
    # a failed download.
    file(GLOB _TARGET_DIR_FILES "${_TARGET_DIR}/*")
    list(LENGTH _TARGET_DIR_FILES _TARGET_DIR_NUM_FILES)
    if(_TARGET_DIR_NUM_FILES GREATER 0)
      set(${DOWNLOAD_PACKAGE_LOCATION} ${_TARGET_DIR} PARENT_SCOPE)
      return()
    endif()
  endif()
  
  # Cache couldn't be used. Download the package:
  if(DOWNLOAD_PACKAGE_URLS)
    set(_URLS ${DOWNLOAD_PACKAGE_URLS})
  else()
    set(_URLS ${DOWNLOAD_SITE}/libraries/${DOWNLOAD_PACKAGE_NAME}-${DOWNLOAD_PACKAGE_VERSION}.zip)
  endif()
  download_files(FILENAMES "${DOWNLOAD_PACKAGE_NAME}.zip"
                 URLS ${_URLS}
                 EXTRACT
                 TARGET_DIR ${_TARGET_DIR}
                 NOINSTALL
  )
  # Save some space by cleaning up the archive we extracted from.
  file(REMOVE ${_TARGET_DIR}/${DOWNLOAD_PACKAGE_NAME}.zip)
  
  set(${DOWNLOAD_PACKAGE_LOCATION} ${_TARGET_DIR} PARENT_SCOPE)
endfunction()

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
  message(STATUS "--> using package Zlib")
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set(USING_ZLIB ON PARENT_SCOPE)
  else()
    set(USING_ZLIB ON)
  endif()
endmacro()

#####################################################################################
# ImGUI
#
macro(_add_package_ImGUI)
  Message(STATUS "--> using package ImGUI")

  set(USING_IMGUI ON)
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set(USING_IMGUI ON PARENT_SCOPE) # PARENT_SCOPE important to have this variable passed to parent. Here we want to notify that something used the OpenGL package
  endif()

endmacro()

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

      if(NOT Vulkan_BUILD_DEPENDENCIES)
        set(Vulkan_BUILD_DEPENDENCIES ON CACHE BOOL "Create dependencies on GLSL files")
      endif()

      set(Vulkan_HEADERS_OVERRIDE_INCLUDE_DIR CACHE PATH "Override for Vulkan headers, leave empty to use SDK")

      if (Vulkan_HEADERS_OVERRIDE_INCLUDE_DIR)
        set(vulkanHeaderDir ${Vulkan_HEADERS_OVERRIDE_INCLUDE_DIR})
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

      # CMake 3.21+ finds glslangValidator and glslc for us.
      # On < 3.21, find it manually:
      if((NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE) OR (NOT Vulkan_GLSLC_EXECUTABLE))
        get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
        find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
          NAMES glslangValidator
          HINTS ${_Vulkan_LIB_DIR}/../Bin)
        find_program(Vulkan_GLSLC_EXECUTABLE
          NAMES glslc
          HINTS ${_Vulkan_LIB_DIR}/../Bin)
      endif()
 else()
     Message(STATUS "--> NOT using package VulkanSDK")
 endif()
endmacro()
# this macro is needed for the samples to add this package, although not needed
# this happens when the nvpro_core library was built with these stuff in it
# so many samples can share the same library for many purposes
macro(_optional_package_VulkanSDK)
  if(USING_VULKANSDK AND NOT Vulkan_FOUND)
    _add_package_VulkanSDK()
  endif()
endmacro(_optional_package_VulkanSDK)

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
    get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
    if(WIN32)
      find_file(Vulkan_shaderc_shared_LIBRARY
        NAMES shaderc_shared.lib
        HINTS ${_Vulkan_LIB_DIR})
      mark_as_advanced(Vulkan_shaderc_shared_LIBRARY)
      find_file(Vulkan_shaderc_shared_DLL
        NAMES shaderc_shared.dll
        HINTS ${_Vulkan_LIB_DIR}/../Bin)
      mark_as_advanced(Vulkan_shaderc_shared_DLL)
      add_definitions(-DSHADERC_SHAREDLIB)
      if(NOT Vulkan_shaderc_shared_DLL)
        message(FATAL_ERROR "Windows platform requires VulkanSDK with shaderc_shared.lib/dll (since SDK 1.2.135.0)")
      endif()
    else()
      if(NOT Vulkan_shaderc_combined_LIBRARY)
        find_file(Vulkan_shaderc_combined_LIBRARY
          NAMES libshaderc_combined.a
          HINTS ${_Vulkan_LIB_DIR})
      endif()
      set(Vulkan_shaderc_shared_LIBRARY ${Vulkan_shaderc_combined_LIBRARY})
      mark_as_advanced(Vulkan_shaderc_shared_LIBRARY)
    endif()
  endif()

  if(Vulkan_FOUND AND (Vulkan_shaderc_shared_LIBRARY OR NVSHADERC_LIB))
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
        LIST(APPEND LIBRARIES_OPTIMIZED ${Vulkan_shaderc_shared_LIBRARY})
        LIST(APPEND LIBRARIES_DEBUG ${Vulkan_shaderc_shared_LIBRARY})
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
  # Avoid calling CUDAToolkit if we already have its results cached
  if(CUDAToolkit_LIBRARY_DIR AND CUDAToolkit_VERSION AND CUDAToolkit_INCLUDE_DIRS AND CUDAToolkit_NVCC_EXECUTABLE)
    message(STATUS "Using cached CUDAToolkit values")
    set(CUDAToolkit_FOUND ON)
  else()
    message(STATUS "Finding CUDAToolkit")
    find_package(CUDAToolkit)
  endif()
  if(CUDAToolkit_FOUND)
      add_definitions("-DCUDA_PATH=R\"(${CUDA_TOOLKIT_ROOT_DIR})\"")
      Message(STATUS "--> using package CUDA (${CUDAToolkit_VERSION})")
      add_definitions(-DNVP_SUPPORTS_CUDA)
      include_directories(${CUDAToolkit_INCLUDE_DIRS})
      if(WIN32)
        LIST(APPEND LIBRARIES_DEBUG "${CUDAToolkit_LIBRARY_DIR}/cuda.lib" )
        LIST(APPEND LIBRARIES_DEBUG "${CUDAToolkit_LIBRARY_DIR}/cudart.lib" )
        LIST(APPEND LIBRARIES_DEBUG "${CUDAToolkit_LIBRARY_DIR}/nvrtc.lib" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDAToolkit_LIBRARY_DIR}/cuda.lib" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDAToolkit_LIBRARY_DIR}/cudart.lib" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDAToolkit_LIBRARY_DIR}/nvrtc.lib" )
      else()
        LIST(APPEND LIBRARIES_DEBUG "libcuda.so" )
        LIST(APPEND LIBRARIES_DEBUG "${CUDAToolkit_LIBRARY_DIR}/libcudart.so" )
        LIST(APPEND LIBRARIES_DEBUG "${CUDAToolkit_LIBRARY_DIR}/libnvrtc.so" )
        LIST(APPEND LIBRARIES_OPTIMIZED "libcuda.so" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDAToolkit_LIBRARY_DIR}/libcudart.so" )
        LIST(APPEND LIBRARIES_OPTIMIZED "${CUDAToolkit_LIBRARY_DIR}/libnvrtc.so" )
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
# NVAPI
macro(_add_package_NVAPI)
  if(WIN32)
    if(NOT NVAPI_LOCATION) # Allow using NVAPI_LOCATION to override the NVAPI path
      download_package(NAME NVAPI VERSION R560 LOCATION NVAPI_LOCATION)
    endif()
    message(STATUS "--> using package NVAPI from ${NVAPI_LOCATION}")
    include_directories(${NVAPI_LOCATION})
    add_definitions(-DNVP_SUPPORTS_NVAPI)
    set(_NVAPI_LIB "${NVAPI_LOCATION}/amd64/nvapi64.lib")
    list(APPEND LIBRARIES_OPTIMIZED ${_NVAPI_LIB})
    list(APPEND LIBRARIES_DEBUG ${_NVAPI_LIB})
  
    get_directory_property(hasParent PARENT_DIRECTORY)
    if(hasParent)
      set( USING_NVAPI "YES" PARENT_SCOPE)
    else()
      set( USING_NVAPI "YES")
    endif()
  else()
    message(WARNING "--> NOT using NVAPI: NVAPI is only supported on Windows.")
  endif()
endmacro()

macro(_optional_package_NVAPI)
  if(USING_NVAPI)
    _add_package_NVAPI()
  endif()
endmacro()

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
    
    get_directory_property(hasParent PARENT_DIRECTORY)
    if(hasParent)
      set( USING_NVML "YES" PARENT_SCOPE)
    else()
      set( USING_NVML "YES")
    endif()
  endif()
endmacro()

macro(_optional_package_NVML)
  if(USING_NVML)
    _add_package_NVML()
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
  
  message(STATUS "--> using package Zstd (from KTX)")
  message(STATUS "--> using package basisu (from KTX)")
  
  get_directory_property(hasParent PARENT_DIRECTORY)
  if(hasParent)
    set( USING_ZSTD "YES" PARENT_SCOPE)
    set( USING_BASISU "YES" PARENT_SCOPE)
  else()
    set( USING_ZSTD "YES")
    set( USING_BASISU "YES")
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
  
  #Message(STATUS "${NVCUDA_COMPILE_PTX} ${options} ${oneValueArgs} ${multiValueArgs} ")

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
    
    add_custom_command(
      OUTPUT  ${output}
      DEPENDS ${input}
      COMMAND ${CMAKE_COMMAND} -E echo ${CUDAToolkit_NVCC_EXECUTABLE} ${MACHINE} --ptx ${NVCUDA_COMPILE_PTX_NVCC_OPTIONS} ${input} -o ${output}
      COMMAND ${CUDAToolkit_NVCC_EXECUTABLE} ${MACHINE} --ptx ${NVCUDA_COMPILE_PTX_NVCC_OPTIONS} ${input} -o ${output}
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
  
  #Message(STATUS "${NVCUDA_COMPILE_CUBIN} ${options} ${oneValueArgs} ${multiValueArgs} ")

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
    
    add_custom_command(
      OUTPUT  ${output}
      DEPENDS ${input}
      COMMAND ${CMAKE_COMMAND} -E echo ${CUDAToolkit_NVCC_EXECUTABLE} ${MACHINE} --cubin ${NVCUDA_COMPILE_CUBIN_NVCC_OPTIONS} ${input} -o ${output} 
      COMMAND ${CUDAToolkit_NVCC_EXECUTABLE} ${MACHINE} --cubin ${NVCUDA_COMPILE_CUBIN_NVCC_OPTIONS} ${input} -o ${output} 
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
    # Will use default output for .lib files not to "polute" 
    # the root smaple binary directory with .lib or .exp files
    #ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_PATH}/$<CONFIG>/"
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
    if(USING_SHADERC AND Vulkan_shaderc_shared_DLL)
      _copy_files_to_target( ${_PROJNAME} "${Vulkan_shaderc_shared_DLL}")
      install(FILES ${Vulkan_shaderc_shared_DLL} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION bin_${ARCH})
      install(FILES ${Vulkan_shaderc_shared_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
    endif()
  endif()
  if(OPTIX_FOUND)
    _copy_files_to_target( ${_PROJNAME} "${OPTIX_DLL}")
    install(FILES ${OPTIX_DLL} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION bin_${ARCH})
    install(FILES ${OPTIX_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  if(PERFWORKS_DLL)
    _copy_files_to_target( ${_PROJNAME} "${PERFWORKS_DLL}")
    install(FILES ${PERFWORKS_DLL} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION bin_${ARCH})
    install(FILES ${PERFWORKS_DLL} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  if(NsightAftermath_FOUND)
    _copy_files_to_target( ${_PROJNAME} "${NsightAftermath_DLLS}")
    install(FILES ${NsightAftermath_DLLS} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION bin_${ARCH})
    install(FILES ${NsightAftermath_DLLS} CONFIGURATIONS Debug DESTINATION bin_${ARCH}_debug)
  endif()
  install(TARGETS ${_PROJNAME} CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION bin_${ARCH})
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
