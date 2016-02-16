# Try to find VulkanSDK project dll/so and headers
#

# outputs
unset(VULKAN_LIB CACHE)
unset(VULKANSTATIC_LIB CACHE)
unset(VULKANSDK_FOUND CACHE)
unset(VULKANSDK_INCLUDE_DIR CACHE)

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


macro(_find_version_path targetVersion targetPath searchList )
  unset ( targetVersion )
  unset ( targetPath )
  SET ( bestver "0.0.0.0" )
  SET ( bestpath "" )
  SET ( bestvernumeric "0000" )
  
   Message(STATUS "searchList: ${dirList}" )

  foreach ( basedir ${searchList} )
    folder_list ( dirList ${basedir} )	
    Message(STATUS "dirList: ${dirList}" )
	  foreach ( checkdir ${dirList} )
      Message(STATUS "checkdir: ${checkdir}" )
	    string ( REGEX MATCH "([0-9]+).([0-9]+).([0-9]+).([0-9]+)" result "${checkdir}" )
	    if ( "${result}" STREQUAL "${checkdir}" )
	       # found a path with versioning 
         SET ( ver "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4}" )
         SET ( vernumeric "${CMAKE_MATCH_1}${CMAKE_MATCH_2}${CMAKE_MATCH_3}${CMAKE_MATCH_4}" )
	       if ( vernumeric GREATER bestvernumeric )
          SET ( bestver ${ver} )
          SET ( bestvernumeric ${vernumeric} )
	  		SET ( bestpath "${basedir}/${checkdir}" )
	  	 endif ()
	    endif()	  
	  endforeach ()		
  endforeach ()  
  SET ( ${targetVersion} "${bestver}" )
  SET ( ${targetPath} "${bestpath}" )
endmacro()

macro(_find_files targetVar incDir dllName dllName64 folder)
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

  # message ( "File list: ${${targetVar}}" )		#-- debugging
endmacro()

 # Locate VULKANSDK by version
set ( SEARCH_PATHS
  ${VULKANSDK_LOCATION} # this could be set to C:\VulkanSDK Best version will be taken
  # in the case where we'd like to include Vulkan minimal stuff to our sample framework:
  #${PROJECT_SOURCE_DIR}/shared_external/vulkan
  #${PROJECT_SOURCE_DIR}/../shared_external/vulkan
  #${PROJECT_SOURCE_DIR}/../../shared_external/vulkan
)
if (WIN32) 
  _find_version_path ( VULKANSDK_VERSION VULKANSDK_ROOT_DIR "${SEARCH_PATHS}" )
endif()
if (UNIX)
    message ( STATUS "VulkanSDK search paths: ${SEARCH_PATHS}")
  #_find_version_path ( VULKANSDK_VERSION VULKANSDK_ROOT_DIR "${SEARCH_PATHS}" )
  
  find_path(VULKANSDK_ROOT_DIR NAMES vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/include")
  find_library(VULKAN_LIB NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib")
  
  Message(STATUS "Vulkan Include : ${VULKANSDK_ROOT_DIR}")
  Message(STATUS "Vulkan Library : ${VULKAN_LIB}")
  

  
endif()
message ( STATUS "VulkanSDK version: ${VULKANSDK_VERSION}")
#
#------- no overridden place to look at so let's use VK_SDK_PATH
#        VK_SDK_PATH directly points to the dedicated version
#        put after the search if one wanted to override this default VK_SDK_PATH
if (NOT VULKANSDK_ROOT_DIR )
  STRING(REGEX REPLACE "\\\\" "/" VK_SDK_PATH "$ENV{VK_SDK_PATH}") 
  find_path( VULKANSDK_INCLUDE_DIR vulkan/vulkan.h ${VK_SDK_PATH}/include )
  if ( VULKANSDK_INCLUDE_DIR )
    set (VULKANSDK_ROOT_DIR ${VK_SDK_PATH} )
  endif()
endif()


if (VULKANSDK_ROOT_DIR)

  if (WIN32) 
	  #-------- Locate LIBS
    _find_files( VULKAN_LIB VULKANSDK_ROOT_DIR "bin/vulkan-1.lib" "bin/vulkan-1.lib" "")
    _find_files( VULKANSTATIC_LIB VULKANSDK_ROOT_DIR "bin/VKstatic.1.lib" "bin/VKstatic.1.lib" "")
    _find_files( GLSLANGVALIDATOR VULKANSDK_ROOT_DIR "bin/glslangValidator.exe" "bin/glslangValidator.exe" "")
  endif(WIN32)

  if (UNIX)
  
  if (VULKANSDK_ROOT_DIR)
  Message("Using system for vulkan sdk. ")
  endif()
  
  endif(UNIX)

	#-------- Locate HEADERS
	_find_files( VULKANSDK_HEADERS VULKANSDK_ROOT_DIR "vulkan.h" "vulkan.h" "include/vulkan/" )

  if(VULKAN_LIB)
	  set( VULKANSDK_FOUND "YES" )      
  endif(VULKAN_LIB)
else(VULKANSDK_ROOT_DIR)

  message(WARNING "
      VULKANSDK not found. 
      either env. VK_SDK_PATH should be set directly to the right version to use (C:\\VulkanSDK\\1.0.1.1)
      or you can specify in cmake VULKANSDK_LOCATION to the folder where VulkanSDK versions are put (C:\\VulkanSDK)"
  )
endif(VULKANSDK_ROOT_DIR)

include(FindPackageHandleStandardArgs)

SET(VULKAN_LIB ${VULKAN_LIB} CACHE PATH "path")
SET(VULKANSTATIC_LIB ${VULKANSTATIC_LIB} CACHE PATH "path")
SET(VULKANSDK_INCLUDE_DIR "${VULKANSDK_ROOT_DIR}/include" CACHE PATH "path")

find_package_handle_standard_args(VULKANSDK DEFAULT_MSG
    VULKANSDK_INCLUDE_DIR
    VULKAN_LIB
)

mark_as_advanced( VULKANSDK_FOUND )

