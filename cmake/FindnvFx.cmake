# Try to find NVFX project dll and include file
#
unset(NVFX_DLL CACHE)
unset(NVFX_INCLUDE_DIR CACHE)
unset(NVFX_FOUND CACHE)
unset(NVFX_INSTALL_PATH CACHE)

if(WIN32)
if( ARCH STREQUAL "x64" )
  set( NVFX_INSTALL_PATH "C:/Program Files/nvFx")
  set( ARCHSUFFIX "64")
else ()
  set( NVFX_INSTALL_PATH "C:/Program Files (x86)/nvFx")
endif()
endif(WIN32)

#message(STATUS "SEARCHING for NVFX in ${NVFX_INSTALL_PATH}" )

find_path( NVFX_INCLUDE_DIR FxLib.h
  ${NVFX_LOCATION}/include
  $ENV{NVFX_LOCATION}/include
  ${NVFX_INSTALL_PATH}/include
  ${GITPROJECTS_LOCATION}/nvFX/include
  $ENV{GITPROJECTS_LOCATION}/nvFX/include
  ${PROJECT_SOURCE_DIR}/../nvFX/include
  ${PROJECT_SOURCE_DIR}/nvFX/include
  ${PROJECT_SOURCE_DIR}/../shared_external/nvFX/include
  ${PROJECT_SOURCE_DIR}/shared_external/nvFX/include
)

find_path( NVFX_LIB_DIR "FxLib${ARCHSUFFIX}.lib"
  ${NVFX_LOCATION}/lib
  $ENV{NVFX_LOCATION}/lib
  ${NVFX_INSTALL_PATH}/lib
  ${GITPROJECTS_LOCATION}/nvFX/lib
  $ENV{GITPROJECTS_LOCATION}/nvFX/lib
  ${PROJECT_SOURCE_DIR}/../nvFX/lib
  ${PROJECT_SOURCE_DIR}/nvFX/lib
  ${PROJECT_SOURCE_DIR}/../shared_external/nvFX/lib
  ${PROJECT_SOURCE_DIR}/shared_external/nvFX/lib
)

set( NVFX_FOUND "NO" )

if(NVFX_INCLUDE_DIR)
  if(NVFX_LIB_DIR)
    find_path( NVFX_LIBD_DIR "FxLib${ARCHSUFFIX}D.lib" ${NVFX_LIB_DIR} )
    if(NVFX_LIBD_DIR)
      set(DBGSUFFIX "D")
    else()
      message(STATUS 
      "Warning: no NVFX debug libs. Using release one only. But _ITERATOR_DEBUG_LEVEL must be 0 everywhere" )
      set(DBGSUFFIX "")
    endif()
    set( NVFX_FOUND "YES" )
    set( NVFX_HEADERS 
      "${NVFX_INCLUDE_DIR}/FxLib.h" 
      "${NVFX_INCLUDE_DIR}/FxLibEx.h"
      "${NVFX_INCLUDE_DIR}/FxParser.h"
    )
    set( NVFX_LIBRARIES
      "${NVFX_LIB_DIR}/FxLib${ARCHSUFFIX}.lib" 
      "${NVFX_LIB_DIR}/FxParser${ARCHSUFFIX}.lib"
    )
    set( NVFX_LIBRARIES_DEBUG
      "${NVFX_LIB_DIR}/FxLib${ARCHSUFFIX}${DBGSUFFIX}.lib" 
      "${NVFX_LIB_DIR}/FxParser${ARCHSUFFIX}${DBGSUFFIX}.lib"
    )
    set( NVFX_LIBRARIES_GL
      "${NVFX_LIB_DIR}/FxLibGL${ARCHSUFFIX}.lib" 
    )
    set( NVFX_LIBRARIES_GL_DEBUG
      "${NVFX_LIB_DIR}/FxLibGL${ARCHSUFFIX}${DBGSUFFIX}.lib" 
    )
    set( NVFX_LIBRARIES_D3D
      "${NVFX_LIB_DIR}/FxLibD3D${ARCHSUFFIX}.lib" 
    )
    set( NVFX_LIBRARIES_D3D_DEBUG
      "${NVFX_LIB_DIR}/FxLibD3D${ARCHSUFFIX}${DBGSUFFIX}.lib" 
    )
  endif()
  else()
  Message(STATUS 
"Warning: nvFX not found. Sample might complain. nvFx can be found at https://github.com/tlorach/nvFX
Compile it; compile INSTALL and setup NVFX_LOCATION in cmake or as Env.Variable (NVFX_LOCATION=C:/Program Files/nvFx for example).
")
endif()

#include(FindPackageHandleStandardArgs)

#find_package_handle_standard_args(NVFX DEFAULT_MSG
#    NVFX_INCLUDE_DIR
#    NVFX_LIB_DIR
#)

mark_as_advanced( NVFX_FOUND )
