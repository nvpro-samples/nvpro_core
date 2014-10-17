# Try to find OPENGLTEXT project dll and include file
#
unset(OPENGLTEXT_PATH CACHE)
unset(OPENGLTEXT_CPP CACHE)
unset(OPENGLTEXT_H CACHE)
unset(OPENGLTEXT_FOUND CACHE)

find_path( OPENGLTEXT_PATH OpenGLText.h
  ${OPENGLTEXT_LOCATION}
  $ENV{OPENGLTEXT_LOCATION}
  ${GITPROJECTS_LOCATION}/OpenGLText
  $ENV{GITPROJECTS_LOCATION}/OpenGLText
  ${PROJECT_SOURCE_DIR}/../shared_external/OpenGLText
  ${PROJECT_SOURCE_DIR}/shared_external/OpenGLText
)

set( OPENGLTEXT_FOUND "NO" )

if(OPENGLTEXT_PATH)
  set( OPENGLTEXT_FOUND "YES" )
  set( OPENGLTEXT_CPP "OpenGLText.cpp")
  SET(OPENGLTEXT_H ${OPENGLTEXT_PATH}/OpenGLText.h CACHE PATH "OpenGLText.h file")
  SET(OPENGLTEXT_CPP ${OPENGLTEXT_PATH}/OpenGLText.cpp CACHE PATH "OpenGLText.cpp file")
else()
  Message(WARNING 
"OpenGLText Not Found. Take it from Github https://github.com/tlorach/OpenGLText
You can also specify its location with cmake var OPENGLTEXT_LOCATION or env. system variable equivalent.
Or you can specify GITPROJECTS_LOCATION in cmake to tell where GitHub downloads are located")
endif()

#include(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(OPENGLTEXT DEFAULT_MSG OpenGLText_PATH)

mark_as_advanced( OPENGLTEXT_FOUND )
