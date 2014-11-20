# Try to find SVCSPACEMOUSE project dll and include file
#
unset(SVCSPACEMOUSE_DLL CACHE)
unset(SVCSPACEMOUSE_INCLUDE_DIR CACHE)
unset(SVCSPACEMOUSE_FOUND CACHE)

find_path( SVCSPACEMOUSE_INCLUDE_DIR ISvcSpaceMouse.h
  ${SVCSPACEMOUSE_LOCATION}/include
  $ENV{SVCSPACEMOUSE_LOCATION}/include
  ${GITPROJECTS_LOCATION}/SvcSpaceMouse/include
  $ENV{GITPROJECTS_LOCATION}/SvcSpaceMouse/include
  ${PROJECT_SOURCE_DIR}/../shared_external/SvcSpaceMouse/include
  ${PROJECT_SOURCE_DIR}/../../shared_external/SvcSpaceMouse/include
  ${PROJECT_SOURCE_DIR}/shared_external/SvcSpaceMouse/include
)

if(ARCH STREQUAL "x86")
    file(GLOB SVCSPACEMOUSE_DLLS "${SVCSPACEMOUSE_INCLUDE_DIR}/../dll/SvcSpaceMouse_*.dll")
else()
    file(GLOB SVCSPACEMOUSE_DLLS "${SVCSPACEMOUSE_INCLUDE_DIR}/../dll/SvcSpaceMouse64_*.dll")
endif()

set( SVCSPACEMOUSE_FOUND "NO" )

list(LENGTH SVCSPACEMOUSE_DLLS NUMDLLS)
if(NUMDLLS EQUAL 0)
  message(WARNING "dll for the User Interface not found. Please compile SVCSPACEMOUSE" )
  set (SVCSPACEMOUSE_DLL "NOTFOUND")
else()
  list(GET SVCSPACEMOUSE_DLLS 0 SVCSPACEMOUSE_DLL)
endif()

if(SVCSPACEMOUSE_INCLUDE_DIR)
  if(SVCSPACEMOUSE_DLL)
    set( SVCSPACEMOUSE_FOUND "YES" )
    set( SVCSPACEMOUSE_HEADERS "${SVCSPACEMOUSE_INCLUDE_DIR}/ISvcSpaceMouse.h" "${SVCSPACEMOUSE_INCLUDE_DIR}/ISvcBase.h")
  endif(SVCSPACEMOUSE_DLL)
else()
  message(WARNING "Space mouse not available. Take it from https://github.com/tlorach/SvcSpaceMouse
  and or setup SVCSPACEMOUSE_LOCATION from cmake or as env. system variable."
  "Or you can specify GITPROJECTS_LOCATION in cmake to tell where Git downloads are located"
)
endif(SVCSPACEMOUSE_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(SVCSPACEMOUSE DEFAULT_MSG
    SVCSPACEMOUSE_INCLUDE_DIR
    SVCSPACEMOUSE_DLL
)
# I duno why I have to rewrite the variable here...
SET(SVCSPACEMOUSE_DLL ${SVCSPACEMOUSE_DLL} CACHE PATH "path")

mark_as_advanced( SVCSPACEMOUSE_FOUND )
