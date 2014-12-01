# Try to find OptiX project dll and include file
#
unset(OPTIX_DLL CACHE)
unset(OPTIX_ROOT_DIR CACHE)
unset(OPTIX_LIB CACHE)
unset(OPTIX_FOUND CACHE)

find_path( OPTIX_ROOT_DIR optix.1.lib
  ${OPTIX_LOCATION}
  $ENV{OPTIX_LOCATION}
  ${PROJECT_SOURCE_DIR}/shared_external/Optix
  ${PROJECT_SOURCE_DIR}/../shared_external/Optix
)

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

if(OPTIX_ROOT_DIR)

	#-------- Locate DLLS
	_find_files( OPTIX_DLL OPTIX_ROOT_DIR "cudart64_42_9.dll" "cudart64_42_9.dll" "")
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "optix.1.dll" "optix.1.dll" "")
    _find_files( OPTIX_DLL OPTIX_ROOT_DIR "optixu.1.dll" "optixu.1.dll" "")
	_find_files( OPTIX_DLL OPTIX_ROOT_DIR "optix_prime.1.dll" "optix_prime.1.dll" "")
    if(NOT OPTIX_DLL)
      message(STATUS "setting OPTIX_DLL to ${OPTIX_DLL}" )
    endif()
	
	#-------- Locate LIBS
    _find_files( OPTIX_LIB OPTIX_ROOT_DIR "optix.1.lib" "optix.1.lib" "")
    _find_files( OPTIX_LIB OPTIX_ROOT_DIR "optixu.1.lib" "optixu.1.lib" "")
	_find_files( OPTIX_LIB OPTIX_ROOT_DIR "optix_prime.1.lib" "optix_prime.1.lib" "")
    if(NOT OPTIX_LIB)
      message(STATUS "setting OPTIX_LIB to ${OPTIX_LIB}" )
    endif()

	#-------- Locate HEADERS
	_find_files( OPTIX_HEADERS OPTIX_ROOT_DIR "optix.h" "optix.h" "include/" )

    if(OPTIX_DLL)
	   set( OPTIX_FOUND "YES" )      
    endif(OPTIX_DLL)
else(OPTIX_ROOT_DIR)

  message(WARNING "
      OPTIX not found. 
      The OPTIX folder you would specify with OPTIX_LOCATION should contain:
      - lib folder: containing the Optix[64_]*.dll
      - include folder: containing the include files
      OR this folder could directly contain the dll and headers, put together
      For now, samples will run without additional UI. But that's okay ;-)"
  )
endif(OPTIX_ROOT_DIR)

include(FindPackageHandleStandardArgs)

SET(OPTIX_DLL ${OPTIX_DLL} CACHE PATH "path")
SET(OPTIX_LIB ${OPTIX_LIB} CACHE PATH "path")
SET(OPTIX_INCLUDE_DIR "${OPTIX_ROOT_DIR}/include")

find_package_handle_standard_args(OPTIX DEFAULT_MSG
    OPTIX_INCLUDE_DIR
    OPTIX_DLL
)

mark_as_advanced( OPTIX_FOUND )
