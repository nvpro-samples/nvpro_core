# Try to find OptiX project dll and include file
#
unset(CUDA_DLL CACHE)
unset(CUDA_ROOT_DIR CACHE)
unset(CUDA_LIB CACHE)
unset(CUDA_VERSION CACHE)
unset(CUDA_FOUND CACHE)
unset(RESULT CACHE)

if ( NOT DEFINED ${CUDA_LOCATION} )
	SET(CUDA_LOCATION "$ENV{CUDA_PATH}" CACHE PATH "path")
endif()

find_path( CUDA_ROOT_DIR include/cuda.h
  ${CUDA_LOCATION}  
)

macro(_find_files targetVar incDir dllName dllName64 folder result)
  unset ( result )
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
    set (${result} "NOTFOUND" )
  else()
    set (${result} "OK" )
  endif()
  list(APPEND ${targetVar} ${fileList} )

endmacro()

if(CUDA_ROOT_DIR)
    #---------Retrieve VERSION
	set( CUDA_VERSION_MAX "0.0" )
    string( REGEX REPLACE "(^.*)/[^/]*$" "\\1" CUDA_GENERAL_DIR ${CUDA_ROOT_DIR} )
	file( GLOB CUDA_VERSION_PATHS ${CUDA_GENERAL_DIR}/* )
	foreach( CUDA_VERSION_PATH ${CUDA_VERSION_PATHS} )
	  if( IS_DIRECTORY ${CUDA_VERSION_PATH} )
        string( REGEX REPLACE "^.*/v([^/]*)$" "\\1" CUDA_VERSION_CURRENT ${CUDA_VERSION_PATH} )	
	      if( CUDA_VERSION_CURRENT MATCHES "[0-9]+[.]+[0-9]+" )	
		    if( ${CUDA_VERSION_MAX} VERSION_LESS ${CUDA_VERSION_CURRENT} )
		      set( CUDA_VERSION_MAX ${CUDA_VERSION_CURRENT} ) 
		    endif()
		  endif()
	  endif()
	endforeach()

    SET(CUDA_VERSION ${CUDA_VERSION_MAX} CACHE STRING "string")
    string( REGEX REPLACE "(^[^.]*)[.]+.*$" "\\1" CUDA_VERSION_MAJOR ${CUDA_VERSION} )
    string( REGEX REPLACE "^[^.]*[.]+(.*$)" "\\1" CUDA_VERSION_MINOR ${CUDA_VERSION} )
	
	if( CUDA_VERSION_MAX STREQUAL "0.0" )
	  message( "CUDA directory is not valid" )
	else()
	  set( CUDA_ROOT_DIR ${CUDA_GENERAL_DIR}/v${CUDA_VERSION} )
	endif()


	#-------- Locate DLLS
    _find_files( CUDA_DLL CUDA_ROOT_DIR "cudart32_${CUDA_VERSION_MAJOR}${CUDA_VERSION_MINOR}.dll" "cudart64_${CUDA_VERSION_MAJOR}${CUDA_VERSION_MINOR}.dll" "bin/" RESULT)	
	if ( ${RESULT} STREQUAL "NOTFOUND" )
	   message( "CUDA is required for this sample (${PROJECT_NAME})." )
	   message( "CUDA binaries have not been found. Please set the CUDA_LOCATION variable above to the root location of the desired CUDA version." )
	endif()
	
	#-------- Locate LIBS
    _find_files( CUDA_LIB CUDA_ROOT_DIR "win32/cudart.lib" "x64/cudart.lib" "lib/" RESULT)
    _find_files( CUDA_LIB CUDA_ROOT_DIR "win32/cublas.lib" "x64/cublas.lib" "lib/" RESULT)

	#-------- Locate HEADERS
	_find_files( CUDA_HEADERS CUDA_ROOT_DIR "cuda.h" "cuda.h" "include/" RESULT)

    if(CUDA_DLL)
	   set( CUDA_FOUND "YES" )      
    endif(CUDA_DLL)
else(CUDA_ROOT_DIR)

  message(INFO "CUDA not found. Please set the CUDA_LOCATION variable above to the root location of CUDA 5.5.")

endif(CUDA_ROOT_DIR)

include(FindPackageHandleStandardArgs)

SET(CUDA_DLL ${CUDA_DLL} CACHE PATH "path")
SET(CUDA_LIB ${CUDA_LIB} CACHE PATH "path")
SET(CUDA_INCLUDE_DIR "${CUDA_ROOT_DIR}/include")

find_package_handle_standard_args(CUDA DEFAULT_MSG
    CUDA_INCLUDE_DIR
    CUDA_DLL
)

mark_as_advanced( CUDA_FOUND )
