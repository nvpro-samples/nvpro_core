# Try to find OptiX project dll and include file
#
unset(CUDA_DLL CACHE)
unset(CUDA_ROOT_DIR CACHE)
unset(CUDA_LIB CACHE)
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

	#-------- Locate DLLS
    _find_files( CUDA_DLL CUDA_ROOT_DIR "cudart32_55.dll" "cudart64_55.dll" "bin/" RESULT)	
	if ( ${RESULT} STREQUAL "NOTFOUND" )
	   message( "CUDA 5.5 is required for this sample (${PROJECT_NAME})." )
	   message( "Please set the CUDA_LOCATION variable above to the root location of CUDA 5.5." )
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
