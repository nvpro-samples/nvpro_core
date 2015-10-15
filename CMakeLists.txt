cmake_minimum_required(VERSION 2.8)
#####################################################################################
# offer the choice of having shared_sources as a sub-folder... good for packaging a sample
#
# if BASE_DIRECTORY not defined, it means this cmake file was called as the first entry point and not included
if(NOT BASE_DIRECTORY) # if not defined, it means this cmake file was called as the first entry point and not included
  set(PROJNAME shared_sources)
  Project(${PROJNAME})
  Message(STATUS "*******************************")
  Message(STATUS "Processing Project ${PROJNAME}:")
  SET(BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
  INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_include.txt)
endif()

set(MAIN_SOURCE ${BASE_DIRECTORY}/shared_sources/main.h )
if(GLFW_FOUND AND USE_GLFW)
  SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_glfw.cpp)
else()
  if(UNIX) 
    SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_x11.cpp ) #TODO
  else(UNIX)
    if(APPLE)
      SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_osx.cpp ) #TODO
    else(APPLE)
      if(WIN32)
        SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_win32.cpp )
      endif(WIN32)
    endif(APPLE)
  endif(UNIX)
endif()
LIST(APPEND MAIN_SOURCE
    ${MAIN_FILE}
)
#MESSAGE(STATUS "${BASE_DIRECTORY}/shared_sources/main.h")

file(GLOB NVMATH_SOURCE nv_math/*.cpp nv_math/*.hpp nv_math/*.inl nv_math/*.h)
file(GLOB NVHELPERS_SOURCE nv_helpers/*.cpp nv_helpers/*.hpp nv_helpers/*.inl nv_helpers/*.h)
file(GLOB NVHELPERSGL_SOURCE nv_helpers_gl/*.cpp nv_helpers_gl/*.hpp nv_helpers_gl/*.inl nv_helpers_gl/*.h)
file(GLOB NOISE_SOURCE noise/*.cpp noise/*.hpp noise/*.inl noise/*.h)
file(GLOB OPENGLTEXT nv_helpers_gl/OpenGLText/*.cpp nv_helpers_gl/OpenGLText/*.hpp nv_helpers_gl/OpenGLText/*.inl nv_helpers_gl/OpenGLText/*.h)
file(GLOB TRACEDISPLAY nv_helpers_gl/traceDisplay/*.cpp nv_helpers_gl/traceDisplay/*.hpp nv_helpers_gl/traceDisplay/*.inl nv_helpers_gl/traceDisplay/*.h)

if(GLEW_SOURCE)
  file(GLOB GLEW_FILES glew/include/GL/*.h glew/src/glew.c)
  source_group("glew" FILES ${GLEW_FILES})
endif()

source_group("OpenGLText" FILES ${OPENGLTEXT})
source_group("traceDisplay" FILES ${TRACEDISPLAY})
source_group("nv_math" FILES ${NVMATH_SOURCE})
source_group("nv_helpers" FILES ${NVHELPERS_SOURCE})
source_group("nv_helpers_gl" FILES ${NVHELPERSGL_SOURCE})
source_group("noise" FILES ${NOISE_SOURCE})
source_group("main" FILES ${MAIN_SOURCE})

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)
if(SUPPORT_NVTOOLSEXT)
  include_directories(
      ${BASE_DIRECTORY}/shared_external/NSight
  )
  LIST(APPEND PACKAGE_SOURCE_FILES 
    ${BASE_DIRECTORY}/shared_external/NSight/NSightEvents.h
    ${BASE_DIRECTORY}/shared_external/NSight/nvToolsExt.h
  )
  add_definitions(-DSUPPORT_NVTOOLSEXT)
endif()

add_library(shared_sources STATIC 
  ${OPENGLTEXT}
  ${TRACEDISPLAY}
  ${NVHELPERS_SOURCE} 
  ${NVHELPERSGL_SOURCE} 
  ${NVMATH_SOURCE} 
  ${NOISE_SOURCE} 
  ${GLEW_FILES} 
  ${MAIN_SOURCE}
)

target_link_libraries(shared_sources ${PLATFORM_LIBRARIES})
