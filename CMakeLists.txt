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

set(NVPWINDOW_SOURCE ${BASE_DIRECTORY}/shared_sources/nvpwindow_internal.hpp )
if(UNIX) 
  SET(NVPWINDOW_FILE ${BASE_DIRECTORY}/shared_sources/nvpwindow_internal_x11.cpp ) #TODO
else(UNIX)
  if(APPLE)
    SET(NVPWINDOW_FILE ${BASE_DIRECTORY}/shared_sources/nvpwindow_internal_osx.cpp ) #TODO
  else(APPLE)
    if(WIN32)
      SET(NVPWINDOW_FILE ${BASE_DIRECTORY}/shared_sources/nvpwindow_internal_win32.cpp )
    endif(WIN32)
  endif(APPLE)
endif(UNIX)

LIST(APPEND NVPWINDOW_SOURCE
    ${NVPWINDOW_FILE}
    ${BASE_DIRECTORY}/shared_sources/nvpwindow.cpp
    ${BASE_DIRECTORY}/shared_sources/nvpwindow.hpp
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

file(GLOB NVMATH_SOURCE nvmath/*.cpp nvmath/*.hpp nvmath/*.inl nvmath/*.h)
file(GLOB NVHELPERS_SOURCE nvh/*.cpp nvh/*.hpp nvh/*.inl nvh/*.h)
file(GLOB NOISE_SOURCE noise/*.cpp noise/*.hpp noise/*.inl noise/*.h)

set(library_name "shared_sources")

if(USING_OPENGL)
  file(GLOB NVHELPERSGL_SOURCE nvgl/*.cpp nvgl/*.hpp nvgl/*.inl nvgl/*.h)
  source_group("nvgl" FILES ${NVHELPERSGL_SOURCE})
  
  string(CONCAT library_name ${library_name} "_gl")
endif()

if(USING_VULKANSDK)
  file(GLOB NVHELPERSVK_SOURCE nvvk/*.cpp nvvk/*.hpp nvvk/*.inl nvvk/*.h)
  source_group("nvvk" FILES ${NVHELPERSVK_SOURCE})

  if (USING_NVVK2)
      file(GLOB NVHELPERSVK2_SOURCE nvvk2/*.cpp nvvk2/*.hpp nvvk2/*.inl nvvk2/*.h)
      source_group("nvvk2" FILES ${NVHELPERSVK2_SOURCE})
  endif()

  string(CONCAT library_name ${library_name} "_vk")
endif()

if (USING_DIRECTX11)
  file(GLOB NVHELPERSDX11_SOURCE nvdx11/*.cpp nvdx11/*.hpp nvdx11/*.inl nvdx11/*.h)
  source_group("nvdx11" FILES ${NVHELPERSDX11_SOURCE})

  string(CONCAT library_name ${library_name} "_dx11")
endif()

if (USING_DIRECTX12)
  file(GLOB NVHELPERSDX12_SOURCE nvdx12/*.cpp nvdx12/*.hpp nvdx12/*.inl nvdx12/*.h)
  source_group("nvdx12" FILES ${NVHELPERSDX12_SOURCE})

  string(CONCAT library_name ${library_name} "_dx12")
endif()


if (USING_OPTIX)
    Message(STATUS "Note: adding Optix utility files")
    file(GLOB NVHELPERSOPTIX_FILES nvoptix/*.cpp nvoptix/*.h)
    source_group("nvoptix" FILES ${NVHELPERSOPTIX_FILES})
endif()




source_group("nvmath" FILES ${NVMATH_SOURCE})
source_group("nvh" FILES ${NVHELPERS_SOURCE})
source_group("noise" FILES ${NOISE_SOURCE})
source_group("nvpwindow" FILES ${NVPWINDOW_SOURCE})

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(SUPPORT_NVTOOLSEXT)
  include_directories(
      ${BASE_DIRECTORY}/shared_external/NSight
  )
  LIST(APPEND PACKAGE_SOURCE_FILES 
    ${BASE_DIRECTORY}/shared_sources/nvh/nsightevents.h
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
  ${NVPWINDOW_SOURCE}
  ${OPENGL_FILES}
  ${NVHELPERSVK_SOURCE}
  ${NVHELPERSVK2_SOURCE}
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
