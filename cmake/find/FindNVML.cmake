# Copyright (c) 2020-2023, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  set(_CUDA_LIB_DIR ${CUDA_TOOLKIT_ROOT_DIR}/lib/${_ARCH})
  set(_CUDA_INCLUDE_DIRS ${CUDA_TOOLKIT_ROOT_DIR}/include)
else() # CMake >= 3.17.0
  find_package(CUDAToolkit)
  set(_CUDA_LIB_DIR ${CUDAToolkit_LIBRARY_DIR})
  set(_CUDA_INCLUDE_DIRS ${CUDAToolkit_INCLUDE_DIRS})
endif()

# Finding CUDA doesn't guarantee that NVML was installed with CUDA, since
# one can turn that off during installation. Search for NVML in a number of
# locations:

find_library(NVML_LIBRARIES NAMES nvml nvidia-ml PATHS ${_CUDA_LIB_DIR})

if(NOT NVML_LIBRARIES)
  if(WIN32)
    message(WARNING "CMake couldn't locate the NVML library, so compilation \
will likely fail. You may need to install the CUDA toolkit.")
  else()
    message(WARNING "CMake couldn't locate the nvidia-ml library, so \
compilation will likely fail. You may need to install NVML using your OS' \
package manager; for instance, by running `sudo apt install libnvidia-ml-dev`.")
  endif()
endif()

find_path(NVML_INCLUDE_DIRS nvml.h
  ${NVML_LOCATION}
  $ENV{NVML_LOCATION}
  ${_CUDA_INCLUDE_DIRS}
  # if no CUDA, let's try to find nvml locally in our third_party supplement.
  # FindNVML.cmake is located in nvpro_core/cmake/find.
  ${CMAKE_CURRENT_LIST_DIR}/../../third_party/binaries/nvml
)

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
