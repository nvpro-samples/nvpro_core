# Try to find OptiX project dll/so and headers
#

# outputs
unset(OPTIX_DLL CACHE)
unset(OPTIX_LIB CACHE)
unset(OPTIX_FOUND CACHE)
unset(OPTIX_INCLUDE_DIR CACHE)

# search path, can be overridden by user
set ( OPTIX_LOCATION "${PROJECT_SOURCE_DIR}/../shared_optix" CACHE PATH "Path to OptiX install" )

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

macro(_find_version_path targetVersion targetPath rootName searchList platform )
  unset ( targetVersion )
  unset ( targetPath )
  SET ( bestver "0.0.0" )
  SET ( bestpath "" )
  foreach ( basedir ${searchList} )
    folder_list ( dirList ${basedir} ${platform} )	
	  foreach ( checkdir ${dirList} ) 	 
	    string ( REGEX MATCH "${rootName}.([0-9]+).([0-9]+).([0-9]+)(.*)$" result "${checkdir}" )
	    if ( "${result}" STREQUAL "${checkdir}" )
	       # found a path with versioning 
	       SET ( ver "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" )
	       if ( ver GREATER bestver )
	  	    SET ( bestver ${ver} )
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

 # Locate OptiX by version
set ( SEARCH_PATHS
  ${OPTIX_LOCATION}
  ${PROJECT_SOURCE_DIR}/shared_optix
  ${PROJECT_SOURCE_DIR}/../shared_optix
  ${PROJECT_SOURCE_DIR}/../../shared_optix
  $ENV{OPTIX_LOCATION}  
)
if (WIN32) 
  _find_version_path ( OPTIX_VERSION OPTIX_ROOT_DIR "Optix" "${SEARCH_PATHS}" "win64" )
endif()
if (UNIX)
  _find_version_path ( OPTIX_VERSION OPTIX_ROOT_DIR "Optix" "${SEARCH_PATHS}" "linux64" )
endif()
message ( STATUS "OptiX version: ${OPTIX_VERSION}")

if (NOT OPTIX_ROOT_DIR )
  # Locate by version failed. Handle user override for OPTIX_LOCATION.
  find_path( OPTIX_INCLUDE_DIR optix.h ${OPTIX_LOCATION}/include )
  if ( OPTIX_INCLUDE_DIR )
    set (OPTIX_ROOT_DIR ${OPTIX_INCLUDE_DIR}/../ )
  endif()
endif()


if (OPTIX_ROOT_DIR)

  if (WIN32) 
	  #-------- Locate DLLS
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "lib/optix.1.dll" "lib64/optix.1.dll" "")
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "lib/optixu.1.dll" "lib64/optixu.1.dll" "")
	  _find_files( OPTIX_DLL OPTIX_ROOT_DIR "lib/optix_prime.1.dll" "lib64/optix_prime.1.dll" "")
	  
	  #-------- Locate LIBS
    _find_files( OPTIX_LIB OPTIX_ROOT_DIR "lib/optix.1.lib" "lib64/optix.1.lib" "")
    _find_files( OPTIX_LIB OPTIX_ROOT_DIR "lib/optixu.1.lib" "lib64/optixu.1.lib" "")
	  _find_files( OPTIX_LIB OPTIX_ROOT_DIR "lib/optix_prime.1.lib" "lib64/optix_prime.1.lib" "")
    if(NOT OPTIX_LIB)
      message(STATUS "setting OPTIX_LIB to ${OPTIX_LIB}" )
    endif()
  endif(WIN32)

  if (UNIX)
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "lib/liboptix.so" "lib64/liboptix.so" "" )
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "lib/liboptixu.so" "lib64/liboptixu.so" "" )
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "lib/liboptix_prime.so" "lib64/liboptix_prime.so" "" )

    set(OPTIX_LIB ${OPTIX_DLL})

  endif(UNIX)

	#-------- Locate HEADERS
	_find_files( OPTIX_HEADERS OPTIX_ROOT_DIR "optix.h" "optix.h" "include/" )


  if(OPTIX_DLL)
	  set( OPTIX_FOUND "YES" )      
  else()
    message(STATUS "setting OPTIX_DLL to ${OPTIX_DLL}" )
  endif(OPTIX_DLL)
else(OPTIX_ROOT_DIR)

  message(WARNING "
      OPTIX not found. 
      The OPTIX folder you would specify with OPTIX_LOCATION should contain:
      - lib[64] folder: containing the Optix[64_]*.dll or *.so
      - include folder: containing the include files"
  )
endif(OPTIX_ROOT_DIR)

include(FindPackageHandleStandardArgs)

SET(OPTIX_DLL ${OPTIX_DLL} CACHE PATH "path")
SET(OPTIX_LIB ${OPTIX_LIB} CACHE PATH "path")
SET(OPTIX_INCLUDE_DIR "${OPTIX_ROOT_DIR}/include" CACHE PATH "path")

find_package_handle_standard_args(OPTIX DEFAULT_MSG
    OPTIX_INCLUDE_DIR
    OPTIX_DLL
)

mark_as_advanced( OPTIX_FOUND )

