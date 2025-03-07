# Copyright (c) 2020-2025, NVIDIA CORPORATION. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-FileCopyrightText: Copyright (c) 2020-2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------
# Try to find NVML.
# Once done this will define
#
# NVML_FOUND - whether all of the components of NVML were found
# NVML_INCLUDE_DIRS - same as the CUDA include dir
# NVML_LIBRARIES - the linker library for NVML
#
# The NVML headers and linker library are distributed as part of the CUDA SDK.
# We use the variables set by finding CUDA
# (see https://cmake.org/cmake/help/latest/module/FindCUDAToolkit.html and
# https://cmake.org/cmake/help/v3.3/module/FindCUDA.html)
# However, the shared library (DLL on Windows) should be found at runtime.
#
# Please note that on Windows, NVML is only available for 64-bit systems.

# If we've already found NVML, avoid re-running this script. This helps because
# find_package(CUDAToolkit) often spends time re-finding packages.
if((NOT NVML_LIBRARIES) OR (NOT NVML_INCLUDE_DIRS))
  set(NVML_FOUND OFF)

  # FindCUDAToolkit is the new module to use here, but it's only available in
  # CMake 3.17+.
  # FindCUDA is available until 3.27. So we switch between the two
  # depending on the CMake version:
  if(${CMAKE_VERSION} VERSION_LESS 3.17.0)
    find_package(CUDA)
    if(WIN32) # Windows
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_ARCH "x64")
      else()
        message(FATAL_ERROR "FindNVML.cmake was called with a 32-bit platform on \
  Windows, but NVML and nvpro_core are not available for 32-bit systems on \
  Windows. Did you mean to use a 64-bit platform?")
      endif()
    endif()
    set(_TRY_LIB_DIRS ${CUDA_TOOLKIT_ROOT_DIR}/lib/${_ARCH})
    set(_TRY_INCLUDE_DIRS ${CUDA_TOOLKIT_ROOT_DIR}/include)
  else() # CMake >= 3.17.0
    find_package(CUDAToolkit)
    if(TARGET CUDA::nvml)
      # We get both the library (if it exists) and the stub library in case we want
      # to build an NVML app without an installed driver on Linux:
      get_target_property(_IMPORTED_NVML_LIB CUDA::nvml IMPORTED_LOCATION)
      get_target_property(_IMPORTED_NVML_IMPLIB CUDA::nvml IMPORTED_IMPLIB)
      unset(_TRY_LIB_DIRS)
      if(_IMPORTED_NVML_LIB)
        get_filename_component(_IMPORTED_NVML_LIB_DIR ${_IMPORTED_NVML_LIB} DIRECTORY)
        list(APPEND _TRY_LIB_DIRS ${_IMPORTED_NVML_LIB_DIR})
      endif()
      if(_IMPORTED_NVML_IMPLIB)
        get_filename_component(_IMPORTED_NVML_IMPLIB_DIR ${_IMPORTED_NVML_IMPLIB} DIRECTORY)
        list(APPEND _TRY_LIB_DIRS ${_IMPORTED_NVML_IMPLIB_DIR})
      endif()
      get_target_property(_TRY_INCLUDE_DIRS CUDA::nvml INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  # Finding CUDA doesn't guarantee that NVML was installed with CUDA, since
  # one can turn that off during installation. Search for NVML in a number of
  # locations:

  find_library(NVML_LIBRARIES NAMES nvml nvidia-ml HINTS ${_TRY_LIB_DIRS})

  find_path(NVML_INCLUDE_DIRS nvml.h HINTS
    ${NVML_LOCATION}
    $ENV{NVML_LOCATION}
    ${_TRY_INCLUDE_DIRS}
    # if no CUDA, let's try to find nvml locally in our third_party supplement.
    # FindNVML.cmake is located in nvpro_core/cmake/find.
    ${CMAKE_CURRENT_LIST_DIR}/../../third_party/nvml
  )
endif()

if(NOT NVML_LIBRARIES)
  if(WIN32)
    message(WARNING "CMake couldn't locate the NVML library, so compilation \
will likely fail. You may need to install the CUDA toolkit.")
  else()
    message(WARNING "CMake couldn't locate the nvidia-ml library, so \
compilation will likely fail. You may need to install NVML using your OS' \
package manager; for instance, by running `sudo apt install libnvidia-ml-dev`. \
If you're using NixOS, you'll need CMake 3.17.0 or newer, so that this script \
can use CMake's FindCUDAToolkit.")
  endif()
endif()

if(NOT NVML_INCLUDE_DIRS)
  message(WARNING "
      NVML headers not found. To explicitly locate it, set NVML_LOCATION,
      which should be a folder containing nvml.h."
  )
endif()

if(NVML_LIBRARIES AND NVML_INCLUDE_DIRS)
  set(NVML_FOUND ON)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(NVML DEFAULT_MSG
  NVML_INCLUDE_DIRS
  NVML_LIBRARIES
)

mark_as_advanced(
  NVML_INCLUDE_DIRS
  NVML_LIBRARIES
)
