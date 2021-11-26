# Try to find OptiX project dll/so and headers
#

# outputs
unset(OPTIX7_DLL CACHE)
unset(OPTIX7_LIB CACHE)
unset(OPTIX7_FOUND CACHE)
unset(OPTIX7_INCLUDE_DIR CACHE)

# OPTIX7_LOCATION can be setup to search versions somewhere else

macro ( folder_list result curdir substring )
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*${substring}*)
  SET(dirlist "")
  foreach ( child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

macro(_find_version_path targetVersion targetPath rootName searchList )
  unset ( targetVersion )
  unset ( targetPath )
  SET ( bestver "0.0.0" )
  SET ( bestpath "" )
  foreach ( basedir ${searchList} )
    folder_list ( dirList ${basedir} ${rootName} )
	  foreach ( checkdir ${dirList} ) 	 
	    string ( REGEX MATCH "${rootName}(.*)([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)$" result "${checkdir}" )
	    if ( "${result}" STREQUAL "${checkdir}" )
	       # found a path with versioning 
	       SET ( ver "${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4}" )
	       if ( ver VERSION_GREATER bestver )
	  	    SET ( bestver ${ver} )
          SET ( bestmajorver ${CMAKE_MATCH_2})
          SET ( bestminorver ${CMAKE_MATCH_3})
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

if (DEFINED OPTIX7_LOCATION OR DEFINED ENV{OPTIX7_LOCATION} )
  Message(STATUS "using OPTIX7_LOCATION (${OPTIX7_LOCATION})...")
  if(NOT DEFINED OPTIX7_LOCATION)
    if(DEFINED ENV{OPTIX7_LOCATION})
      set(OPTIX7_LOCATION $ENV{OPTIX7_LOCATION})
    endif()
  endif()
  # Locate by version failed. Handle user override for OPTIX7_LOCATION.
  string ( REGEX MATCH ".*([7]+).([0-9]+).([0-9]+)(.*)$" result "${OPTIX7_LOCATION}" )
  if ( "${result}" STREQUAL "${OPTIX7_LOCATION}" )
    SET ( bestver "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" )
    SET ( bestmajorver ${CMAKE_MATCH_1})
    SET ( bestminorver ${CMAKE_MATCH_2})
    Message(STATUS "found version ${bestver}")
  else()
    Message(WARNING "Could NOT extract the version from OptiX7 folder : ${result}")
  endif()
  find_path( OPTIX7_INCLUDE_DIR optix.h ${OPTIX7_LOCATION}/include )
  if ( OPTIX7_INCLUDE_DIR )
    set (OPTIX7_ROOT_DIR ${OPTIX7_INCLUDE_DIR}/../ )
  endif()
endif()
if(NOT DEFINED OPTIX7_ROOT_DIR)
 # Locate OptiX by version
 set ( SEARCH_PATHS
  $ENV{OPTIX7_LOCATION}
  ${OPTIX7_LOCATION}
  ${PROJECT_SOURCE_DIR}/../LocalPackages/Optix
  ${PROJECT_SOURCE_DIR}/../../LocalPackages/Optix
  ${PROJECT_SOURCE_DIR}/../../../LocalPackages/Optix
  C:/ProgramData/NVIDIA\ Corporation

 )
 
 _find_version_path ( OPTIX7_VERSION OPTIX7_ROOT_DIR "OptiX" "${SEARCH_PATHS}" )
 
 message ( STATUS "OptiX version: ${OPTIX7_VERSION}")
endif()

if (OPTIX7_ROOT_DIR)
	#-------- Locate HEADERS
	_find_files( OPTIX7_HEADERS OPTIX7_ROOT_DIR "optix.h" "optix.h" "include/" )

    include(FindPackageHandleStandardArgs)

    SET(OPTIX7_INCLUDE_DIR "${OPTIX7_ROOT_DIR}/include" CACHE PATH "path")
    add_definitions("-DOPTIX7_PATH=R\"(${OPTIX7_ROOT_DIR})\"")
    add_definitions("-DOPTIX7_VERSION_STR=\"${OPTIX7_VERSION}\"")
    
else(OPTIX7_ROOT_DIR)

  message(WARNING "
      OPTIX not found. 
      The OPTIX folder you would specify with OPTIX7_LOCATION should contain:
      - lib[64] folder: containing the Optix[64_]*.dll or *.so
      - include folder: containing the include files"
  )
endif(OPTIX7_ROOT_DIR)

find_package_handle_standard_args(Optix7 DEFAULT_MSG OPTIX7_ROOT_DIR)
mark_as_advanced( OPTIX7_FOUND )

