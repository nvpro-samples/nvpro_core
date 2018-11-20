cmake_minimum_required(VERSION 3.02)
#####################################################################################
# offer the choice of having shared_sources as a sub-folder... good for packaging a sample
#
# if BASE_DIRECTORY not defined, it means this cmake file was called as the first entry point and not included
if(NOT BASE_DIRECTORY) # if not defined, it means this cmake file was called as the first entry point and not included
  Message(FATAL_ERROR "cannot be directly invoked")
else()
  Message(STATUS "-------------------------------")
  Message(STATUS "Processing shared_sources files")
endif()

set(MAIN_SOURCE ${BASE_DIRECTORY}/shared_sources/main.h )
if(UNIX) 
  SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_x11.cpp ) #TODO
  #Add the OpenGL code : some project in the group of stuff to build need it
  if(USING_OPENGL)
    LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_x11_gl.cpp)
  endif()
  if(USING_VULKANSDK)
    LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_x11_vk.cpp)
  endif()
else(UNIX)
  if(APPLE)
    SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_osx.cpp ) #TODO
  else(APPLE)
    if(WIN32)
      SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_win32.cpp )
      if(USING_OPENGL)
        LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_gl.cpp)
      endif()
      if(USING_VULKANSDK)
        LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_vk.cpp)
        LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_vk.hpp)
      endif()
      if(USING_DIRECTX11)
        LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_dx11.cpp)
      endif()
      if(USING_DIRECTX12)
        LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_dx12.cpp)
        LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_dx12.hpp)
      endif()		
    endif(WIN32)
  endif(APPLE)
endif(UNIX)

LIST(APPEND MAIN_SOURCE
    ${MAIN_FILE}
    ${WININTERNAL_FILE}
)

# add packages that may be needed by some of the samples
_optional_package_VulkanSDK()
_optional_package_ShaderC()
_optional_package_OpenGL()
_optional_package_DirectX11()
_optional_package_DirectX12()
_optional_package_AntTweakBar()
_optional_package_OculusSDK()
_optional_package_OpenVRSDK()
_optional_package_Optix()
_optional_package_Ftizb()
_optional_package_Cuda()
_optional_package_OpenCL()
_optional_package_NSight()
_optional_package_ZLIB()
_optional_package_Sockets()
_optional_package_ImGUI()

# process the rest of some cmake code that needs to be done after the packages
_process_shared_cmake_code()

file(GLOB NVMATH_SOURCE nv_math/*.cpp nv_math/*.hpp nv_math/*.inl nv_math/*.h)
file(GLOB NVHELPERS_SOURCE nv_helpers/*.cpp nv_helpers/*.hpp nv_helpers/*.inl nv_helpers/*.h)
file(GLOB NOISE_SOURCE noise/*.cpp noise/*.hpp noise/*.inl noise/*.h)

set(library_name "shared_sources")

if(USING_OPENGL)
  file(GLOB NVHELPERSGL_SOURCE nv_helpers_gl/*.cpp nv_helpers_gl/*.hpp nv_helpers_gl/*.inl nv_helpers_gl/*.h)
  source_group("nv_helpers_gl" FILES ${NVHELPERSGL_SOURCE})
  
  string(CONCAT library_name ${library_name} "_gl")
endif()

if(USING_VULKANSDK)
  file(GLOB NVHELPERSVK_SOURCE nv_helpers_vk/*.cpp nv_helpers_vk/*.hpp nv_helpers_vk/*.inl nv_helpers_vk/*.h)
  source_group("nv_helpers_vk" FILES ${NVHELPERSVK_SOURCE})

  string(CONCAT library_name ${library_name} "_vk")
endif()

if (USING_DIRECTX11)
  file(GLOB NVHELPERSDX11_SOURCE nv_helpers_dx11/*.cpp nv_helpers_dx11/*.hpp nv_helpers_dx11/*.inl nv_helpers_dx11/*.h)
  source_group("nv_helpers_dx11" FILES ${NVHELPERSDX11_SOURCE})

  string(CONCAT library_name ${library_name} "_dx11")
endif()

if (USING_DIRECTX12)
  file(GLOB NVHELPERSDX12_SOURCE nv_helpers_dx12/*.cpp nv_helpers_dx12/*.hpp nv_helpers_dx12/*.inl nv_helpers_dx12/*.h)
  source_group("nv_helpers_dx12" FILES ${NVHELPERSDX12_SOURCE})

  string(CONCAT library_name ${library_name} "_dx12")
endif()


if (USING_OPTIX)
    Message(STATUS "Note: adding Optix utility files")
    file(GLOB NVHELPERSOPTIX_FILES nv_helpers_optix/*.cpp nv_helpers_optix/*.h)
    source_group("nv_helpers_optix" FILES ${NVHELPERSOPTIX_FILES})
endif()




source_group("nv_math" FILES ${NVMATH_SOURCE})
source_group("nv_helpers" FILES ${NVHELPERS_SOURCE})
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
    ${BASE_DIRECTORY}/shared_sources/nv_helpers/nsightevents.h
    ${BASE_DIRECTORY}/shared_external/NSight/nvToolsExt.h
  )
  add_definitions(-DSUPPORT_NVTOOLSEXT)
endif()

if(USING_OPENGL)
  SET(OPENGL_FILES
    ${has_GL_FILES} 
  )
endif()

add_library(shared_sources STATIC 
  ${NVHELPERS_SOURCE} 
  ${NVMATH_SOURCE} 
  ${NOISE_SOURCE} 
  ${MAIN_SOURCE}
  ${OPENGL_FILES}
  ${NVHELPERSVK_SOURCE}
  ${NVHELPERSGL_SOURCE}
  ${NVHELPERSDX11_SOURCE}
  ${NVHELPERSDX12_SOURCE}
  ${PACKAGE_SOURCE_FILES}
  ${NVHELPERSOPTIX_FILES}
)

target_link_libraries(shared_sources
  ${PLATFORM_LIBRARIES}
 )

set_target_properties(shared_sources PROPERTIES OUTPUT_NAME ${library_name})
message(STATUS "shared_sources library name: ${library_name}")
