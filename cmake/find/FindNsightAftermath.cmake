#
# Copyright (c) 2021 NVIDIA Corporation
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

#.rst:
# FindNsightAftermath
# ----------
#
# Try to find the NVIDIA Nsight Aftermath SDK based on the NSIGHT_AFTERMATH_SDK environment variable
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``NsightAftermath::NsightAftermath``, if
# the NVIDIA Nsight Aftermath SDK has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   NsightAftermath_FOUND          - True if the NVIDIA Nsight Aftermath SDK was found
#   NsightAftermath_INCLUDE_DIRS   - include directories for the NVIDIA Nsight Aftermath SDK
#   NsightAftermath_LIBRARIES      - link against this library to use the NVIDIA Nsight Aftermath SDK
#   NsightAftermath_DLLS		   - .dll or .so files needed for distribution
#
# The module will also define two cache variables::
#
#   NsightAftermath_INCLUDE_DIR    - the NVIDIA Nsight Aftermath SDK include directory
#   NsightAftermath_LIBRARY        - the path to the NVIDIA Nsight Aftermath SDK library
#

if(WIN32 OR UNIX)
  find_path(NsightAftermath_INCLUDE_DIR
    NAMES GFSDK_Aftermath.h
    PATHS
      "${NSIGHT_AFTERMATH_SDK}/include"
    )

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(NsightAftermath_LIBRARY
      NAMES GFSDK_Aftermath_Lib.x64
      PATHS
        "${NSIGHT_AFTERMATH_SDK}/lib/x64"
        NO_SYSTEM_ENVIRONMENT_PATH
        )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_library(NsightAftermath_LIBRARY
      NAMES GFSDK_Aftermath_Lib.x86
      PATHS
        "${NSIGHT_AFTERMATH_SDK}/lib/x86"
        NO_SYSTEM_ENVIRONMENT_PATH
        )
  endif()
else()
    find_path(NsightAftermath_INCLUDE_DIR
      NAMES GFSDK_Aftermath.h
      PATHS
        "${NSIGHT_AFTERMATH_SDK}/include")
    find_library(NsightAftermath_LIBRARY
      NAMES GFSDK_Aftermath_Lib
      PATHS
        "${NSIGHT_AFTERMATH_SDK}/lib")
endif()

string(REPLACE ".lib" ".dll" NsightAftermath_DLLS ${NsightAftermath_LIBRARY})

set(NsightAftermath_LIBRARIES ${NsightAftermath_LIBRARY})
set(NsightAftermath_INCLUDE_DIRS ${NsightAftermath_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NsightAftermath
  DEFAULT_MSG
  NsightAftermath_LIBRARY NsightAftermath_INCLUDE_DIR)

mark_as_advanced(NsightAftermath_INCLUDE_DIR NsightAftermath_LIBRARY)

if(NsightAftermath_FOUND AND NOT TARGET NsightAftermath::NsightAftermath)
  add_library(NsightAftermath::NsightAftermath UNKNOWN IMPORTED)
  set_target_properties(NsightAftermath::NsightAftermath PROPERTIES
    IMPORTED_LOCATION "${NsightAftermath_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${NsightAftermath_INCLUDE_DIRS}")
endif()

if(NOT NsightAftermath_FOUND)
    message("NSIGHT_AFTERMATH_SDK environment variable not set to a valid location (value: ${NSIGHT_AFTERMATH_SDK})")
endif()

