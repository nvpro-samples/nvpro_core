# Try to find VulkanSDK project dll/so and headers
#

# outputs
unset(VULKAN_LIB CACHE)
unset(VULKANSDK_FOUND CACHE)
unset(VULKANSDK_INCLUDE_DIR CACHE)
unset(VULKANSDK_SHADERC_LIB CACHE)
unset(VULKANSDK_SHADERC_DLL CACHE)
unset(GLSLANGVALIDATOR)
unset(GLSLC)

set(VULKAN_BUILD_DEPENDENCIES ON CACHE BOOL "Create dependies on GLSL files")

# -------------------------------------------------------------------
macro ( folder_list result curdir )
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  foreach ( child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()
# -------------------------------------------------------------------
macro(_check_version_on_folder checkdir bestver bestvernumeric bestpath)
  # Find a 4-digit version number somewhere in the directory. We allow trailing
  # non-digit characters at the end to make tarball paths like
  # path/to/x.y.z.w/x86_64 work.
  string ( REGEX MATCH ".*([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)[^0-9]*" result "${checkdir}" )
  if ( "${result}" STREQUAL "${checkdir}" )
     # found a path with versioning 
     SET ( ver "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4}" )
     SET ( vernumeric "${CMAKE_MATCH_1}${CMAKE_MATCH_2}${CMAKE_MATCH_3}${CMAKE_MATCH_4}" )
     if ( ver VERSION_GREATER bestver )
      SET ( bestver ${ver} )
      SET ( bestvernumeric ${vernumeric} )
    SET ( bestpath "${basedir}/${checkdir}" )
   endif ()
  endif()
endmacro()
# -------------------------------------------------------------------
macro(_find_version_path targetVersion targetPath searchList )
  unset ( targetVersion )
  unset ( targetPath )
  SET ( bestver "0.0.0.0" )
  SET ( bestpath "" )
  SET ( bestvernumeric "0000" )

  foreach ( basedir ${searchList} )
    folder_list ( dirList ${basedir} )
	  foreach ( checkdir ${dirList} )
      _check_version_on_folder(${checkdir} bestver bestvernumeric bestpath)
	  endforeach ()
  endforeach ()
  SET ( ${targetVersion} "${bestver}" )
  SET ( ${targetPath} "${bestpath}" )
endmacro()
# -------------------------------------------------------------------
macro(_find_files targetVar incDir dllName dllName64 folder)
  unset ( targetVar )
  unset ( fileList )
  if(ARCH STREQUAL "x86")
      file(GLOB fileList "${${incDir}}/../${folder}${dllName}")
       list(LENGTH fileList NUMLIST)
      if(NUMLIST EQUAL 0)
        file(GLOB fileList "${${incDir}}/${folder}${dllName}")
      endif()
  else()
      file(GLOB fileList "${${incDir}}/../${folder}${dllName64}")
       list(LENGTH fileList NUMLIST)
      if(NUMLIST EQUAL 0)
        file(GLOB fileList "${${incDir}}/${folder}${dllName64}")
      endif()
  endif()  
  list(LENGTH fileList NUMLIST)
  if(NUMLIST EQUAL 0)
    message(STATUS "MISSING: unable to find ${targetVar} files (${folder}${dllName}, ${folder}${dllName64})" )
    set (${targetVar} "NOTFOUND")
  endif()
  list(APPEND ${targetVar} ${fileList} )
endmacro()
# -------------------------------------------------------------------
# Locate VULKANSDK by version
STRING(REGEX REPLACE "\\\\" "/" VULKANSDK_LOCATION "${VULKANSDK_LOCATION}")

set ( SEARCH_PATHS
  "${VULKANSDK_LOCATION}" # this could be set to C:\VulkanSDK Best version will be taken
)

_find_version_path ( VULKANSDK_VERSION VULKANSDK_ROOT_DIR "${SEARCH_PATHS}" )

#
#------- no overridden place to look at so let's use VULKAN_SDK
#        VULKAN_SDK directly points to the dedicated version
#        put after the search if one wanted to override this default VULKAN_SDK
if (NOT VULKANSDK_ROOT_DIR )
  STRING(REGEX REPLACE "\\\\" "/" VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
  # This may set VULKANSDK_INCLUDE_DIR to:
  # - C:\VulkanSDK\x.y.z.w\Include (Windows)
  # - path/to/x.y.z.w/x86_64/include (Linux tarball)
  # - /usr/include (Ubuntu package, or if installed in another way and $ENV{VULKAN_SDK} is null)
  # The and bin lib folders are always next to the include folder, so setting
  # VULKANSDK_ROOT_DIR to ${VULKAN_SDK_INCLUDE_DIR}/.. should work.
  find_path( VULKANSDK_INCLUDE_DIR vulkan/vulkan.h HINTS ${VULKAN_SDK_PATH}/include ${VULKAN_SDK_PATH}/Include)
  if ( VULKANSDK_INCLUDE_DIR )
    get_filename_component(VULKANSDK_ROOT_DIR ${VULKANSDK_INCLUDE_DIR} DIRECTORY)
  endif()
endif()

if (VULKANSDK_ROOT_DIR)
  set ( bestver "0.0.0.0" )
  set ( bestpath "" )
  set ( bestvernumeric "0000" )
  _check_version_on_folder(${VULKANSDK_ROOT_DIR} bestver bestvernumeric bestpath)
  set ( VULKANSDK_VERSION "${bestver}" )

  #-------- Locate Vulkan and ShaderC libraries, and the glslangValidator executable.

  if (WIN32)
    _find_files( VULKAN_LIB VULKANSDK_ROOT_DIR "Lib/vulkan-1.lib" "Lib/vulkan-1.lib" "")
    _find_files( VULKANSDK_SHADERC_LIB VULKANSDK_ROOT_DIR "Lib/shaderc_shared.lib" "Lib/shaderc_shared.lib" "")
    _find_files( VULKANSDK_SHADERC_DLL VULKANSDK_ROOT_DIR "Bin/shaderc_shared.dll" "Bin/shaderc_shared.dll" "")
    _find_files( GLSLANGVALIDATOR VULKANSDK_ROOT_DIR "bin/glslangValidator.exe" "bin/glslangValidator.exe" "")
    _find_files( GLSLC VULKANSDK_ROOT_DIR "" "bin/glslc.exe" "")
  endif(WIN32)

  if (UNIX)
    unset(VULKAN_LIB)
    find_library(VULKAN_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib")
    get_filename_component(VULKAN_LIB_DIR ${VULKAN_LIB} DIRECTORY)
    find_library(VULKANSDK_SHADERC_LIB "libshaderc_combined.a" HINTS ${VULKAN_LIB_DIR})
    find_file(GLSLANGVALIDATOR VULKANSDK_ROOT_DIR "glslangValidator" HINTS ${VULKANSDK_ROOT_DIR}"../bin/glslangValidator")
    find_file(GLSLC VULKANSDK_ROOT_DIR "glslc" HINTS ${VULKANSDK_ROOT_DIR}"../bin/glslc")

    # Uncomment these 2 lines to print additional diagnostics:
    # message(STATUS "Vulkan Library Dir    : ${VULKAN_LIB_DIR}")
    # message(STATUS "Vulkan Root Dir       : ${VULKANSDK_ROOT_DIR}")
    message(STATUS "Vulkan Library        : ${VULKAN_LIB}")
    message(STATUS "Vulkan ShaderC Library: ${VULKANSDK_SHADERC_LIB}")
  endif(UNIX)

  if(VULKAN_LIB)
	  set( VULKANSDK_FOUND "YES" )
  endif(VULKAN_LIB)
else(VULKANSDK_ROOT_DIR)

  message(WARNING "
      Vulkan SDK not found.
      Most likely, this means that the environment variable VULKAN_SDK should be set directly to the
      right version to use (e.g. C:\\VulkanSDK\\1.0.1.1; this contains the Vulkan SDK's Bin and Lib folders).
      Another option is that you can set the CMake VULKANSDK_LOCATION variable to the folder where this script should
      search for Vulkan SDK versions (e.g. C:\\VulkanSDK)."
  )

endif(VULKANSDK_ROOT_DIR)

include(FindPackageHandleStandardArgs)

SET(VULKAN_LIB ${VULKAN_LIB} CACHE PATH "path")
message(STATUS "VULKANSDK_ROOT_DIR = ${VULKANSDK_ROOT_DIR}")
# Relative to VULKANSDK_ROOT_DIR, the Vulkan SDK include directory uses a
# capital I on Windows, a lowercase i when installed on Linux using a tarball,
# and is the same as VULKANSDK_ROOT_DIR when installed from a Debian package.
if(EXISTS "${VULKANSDK_ROOT_DIR}/Include/vulkan/vulkan.h")
  SET(VULKANSDK_INCLUDE_DIR "${VULKANSDK_ROOT_DIR}/Include" CACHE PATH "Path to the directory including vulkan/vulkan.h")
elseif(EXISTS "${VULKANSDK_ROOT_DIR}/include/vulkan/vulkan.h")
  set(VULKANSDK_INCLUDE_DIR "${VULKANSDK_ROOT_DIR}/include" CACHE PATH "Path to the directory including vulkan/vulkan.h")
else()
  SET(VULKANSDK_INCLUDE_DIR "${VULKANSDK_ROOT_DIR}"         CACHE PATH "Path to the directory including vulkan/vulkan.h")
endif()
SET(VULKANSDK_SHADERC_LIB ${VULKANSDK_SHADERC_LIB}          CACHE PATH "Path to the directory including the shaderc libraries.")

find_package_handle_standard_args(VulkanSDK DEFAULT_MSG
    VULKANSDK_INCLUDE_DIR
    VULKAN_LIB
    VULKANSDK_SHADERC_LIB
    GLSLANGVALIDATOR
)

mark_as_advanced( VULKANSDK_FOUND )
