# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
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
# (see https://cmake.org/cmake/help/v3.3/module/FindCUDA.html)
# However, the shared library (DLL on Windows) should be found at runtime.
#
# Please note that on Windows, NVML is only available for 64-bit systems.
set(NVML_FOUND OFF)

# Fix directory separators in a TeamCity continuous integration context.
if(CUDA_TOOLKIT_ROOT_DIR)
  string(REPLACE "\\" "/" CUDA_TOOLKIT_ROOT_DIR ${CUDA_TOOLKIT_ROOT_DIR})
endif()
find_package(CUDA)

# Finding CUDA doesn't guarantee that NVML was installed with CUDA, since
# one can turn that off during installation. Verify that NVML in fact
# exists under CUDA_TOOLKIT_ROOT_DIR:
if(WIN32) # Windows
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ARCH "x64")
  else()
    message(WARNING "FindNVML.cmake was called with a 32-bit platform on Windows, but NVML is not available for 32-bit systems on Windows. Did you mean to use a 64-bit platform?")
    # Attempt to set the right path for 32-bit Windows, though,
    # although this is unlikely to change.
    set(_ARCH "Win32")
  endif()
  set(_NVML_LIBRARY_PATH "${CUDA_TOOLKIT_ROOT_DIR}/lib/${_ARCH}/nvml.lib")
else() # Linux
  set(_NVML_LIBRARY_PATH "nvidia-ml")
endif()

set(_NVML_IN_CUDA_DIR ON)
if((NOT EXISTS "${CUDA_TOOLKIT_ROOT_DIR}/include/nvml.h") OR (WIN32 AND (NOT EXISTS "${_NVML_LIBRARY_PATH}")))
  set(_NVML_IN_CUDA_DIR OFF)
endif()

if(_NVML_IN_CUDA_DIR)
  # Add .lib files
  set(NVML_LIBRARIES ${_NVML_LIBRARY_PATH})
  # Include directory
  set(NVML_INCLUDE_DIRS ${CUDA_TOOLKIT_ROOT_DIR}/include)
  set(NVML_FOUND ON)
else()
  Message(STATUS "No CUDA SDK detected with NVML installed. Looking for NVML locally and in NVML_LOCATION (${NVML_LOCATION})")
  # if no CUDA, let's try to find nvml locally in our nvpro_core. This file is
  # located in nvpro_core/cmake/find.
  find_path( NVML_INCLUDE_DIRS nvml.h
    ${NVML_LOCATION}
    $ENV{NVML_LOCATION}
    ${CMAKE_CURRENT_LIST_DIR}/../../third_party/binaries/nvml
  )

  if(NVML_INCLUDE_DIRS)
    Message(STATUS "Found it here : ${NVML_INCLUDE_DIRS}")
    set( NVML_FOUND ON )
    # Add .lib files
    if(WIN32) # Windows
      set(NVML_LIBRARIES "${NVML_INCLUDE_DIRS}/lib/${_ARCH}/nvml.lib")
    else() # Linux
      set(NVML_LIBRARIES "nvidia-ml")
    endif()
  else()
    message(WARNING "
        NVML not found. To explicitly locate it, set NVML_LOCATION,
        which should be a folder containing nvml.h."
    )
  endif()
endif()

# In the non-Windows case, we use the system nvidia-ml library. Check if it
# exists; if it doesn't, we point the user to where to install it.
if(NOT WIN32)
  find_library(_NVML_LIBRARY_PATH ${NVML_LIBRARIES})
  if(NOT _NVML_LIBRARY_PATH)
    message(WARNING "CMake couldn't locate the ${NVML_LIBRARIES} library, so compilation will likely fail. You may need to install NVML using your OS' package manager; for instance, by running `sudo apt install libnvidia-ml-dev`.")
  endif()
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
