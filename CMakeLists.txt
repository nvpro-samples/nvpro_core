cmake_minimum_required(VERSION 3.5)
#####################################################################################
# offer the choice of having nvpro_core as a sub-folder... good for packaging a sample
#
# if BASE_DIRECTORY not defined, it means this cmake file was called as the first entry point and not included
if(NOT BASE_DIRECTORY) # if not defined, it means this cmake file was called as the first entry point and not included
  message(FATAL_ERROR "cannot be directly invoked")
else()
  Message(STATUS "-------------------------------")
  Message(STATUS "Processing nvpro_core files")
endif()


set(NVPWINDOW_SOURCE
  ${BASE_DIRECTORY}/nvpro_core/nvpwindow.cpp
  ${BASE_DIRECTORY}/nvpro_core/nvpwindow.hpp
  ${BASE_DIRECTORY}/nvpro_core/nvpsystem.cpp
  ${BASE_DIRECTORY}/nvpro_core/nvpsystem.hpp
)

if(UNIX) 
  LIST(APPEND NVPWINDOW_SOURCE ${BASE_DIRECTORY}/nvpro_core/nvpsystem_linux.cpp )
else()
  LIST(APPEND NVPWINDOW_SOURCE ${BASE_DIRECTORY}/nvpro_core/nvpsystem_win32.cpp )
endif()

if(NOT (CMAKE_SIZEOF_VOID_P EQUAL 8))
  message( FATAL_ERROR "64-bit builds are mandatory for this framework" )
endif()



# Add third party libraries
add_subdirectory(third_party)

# add packages that may be needed by some of the samples
_optional_package_VulkanSDK()
_optional_package_ShaderC()
_optional_package_OpenGL()
_optional_package_DirectX12()
_optional_package_Freeimage()
_optional_package_Optix()
_optional_package_Cuda()
_optional_package_NVToolsExt()
_optional_package_Sockets()

# process the rest of some cmake code that needs to be done after the packages
_process_shared_cmake_code()

file(GLOB FILEFORMATS_SOURCE fileformats/*.cpp fileformats/*.hpp fileformats/*.inl fileformats/*.h)
file(GLOB NVMATH_SOURCE nvmath/*.cpp nvmath/*.hpp nvmath/*.inl nvmath/*.h)
file(GLOB NVHELPERS_SOURCE nvh/*.cpp nvh/*.hpp nvh/*.inl nvh/*.h)


set(library_name "nvpro_core")

if(USING_OPENGL)
  file(GLOB NVHELPERSGL_SOURCE nvgl/*.cpp nvgl/*.hpp nvgl/*.inl nvgl/*.h)
  source_group("nvgl" FILES ${NVHELPERSGL_SOURCE})
  
  string(CONCAT library_name ${library_name} "_gl")
endif()

if(USING_VULKANSDK)
  file(GLOB NVHELPERSVK_SOURCE nvvk/*.cpp nvvk/*.hpp nvvk/*.inl nvvk/*.h)
  source_group("nvvk" FILES ${NVHELPERSVK_SOURCE})

  string(CONCAT library_name ${library_name} "_vk")
  
  if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
  endif(WIN32)
  if(UNIX)
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
  endif(UNIX)
endif()

if(USING_IMGUI)
  set(IMGUI_SOURCE 
      imgui/imgui_camera_widget.cpp 
      imgui/imgui_camera_widget.h 
      imgui/imgui_helper.cpp
      imgui/imgui_helper.h
      imgui/imgui_orient.cpp  
      imgui/imgui_orient.h
      ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
      ${IMGUI_DIR}/backends/imgui_impl_glfw.h
      )

  include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)
  
  if(USING_OPENGL)
    list(APPEND IMGUI_SOURCE 
         imgui/backends/imgui_impl_gl.cpp
         imgui/backends/imgui_impl_gl.h
         # ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
         # ${IMGUI_DIR}/backends/imgui_impl_opengl3.h
         )
  endif() 
 
  if(USING_VULKANSDK)
    list(APPEND IMGUI_SOURCE 
         imgui/backends/imgui_vk_extra.cpp 
         imgui/backends/imgui_vk_extra.h
         ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp 
         ${IMGUI_DIR}/backends/imgui_impl_vulkan.h)
  endif()

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
  
  string(CONCAT library_name ${library_name} "_optix")
endif()

source_group("fileformats" FILES ${FILEFORMATS_SOURCE})
source_group("noise" FILES ${NOISE_SOURCE})
source_group("nvmath" FILES ${NVMATH_SOURCE})
source_group("nvh" FILES ${NVHELPERS_SOURCE})
source_group("nvp" FILES ${NVPWINDOW_SOURCE})
source_group("imgui" FILES ${IMGUI_SOURCE})

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)


if(USING_OPENGL)
  set(OPENGL_FILES
    ${has_GL_FILES} 
  )
endif()

add_library(nvpro_core STATIC 
  ${NVHELPERS_SOURCE}
  ${NVMATH_SOURCE}
  ${IMGUI_SOURCE}
  ${FILEFORMATS_SOURCE}
  ${NVPWINDOW_SOURCE}
  ${OPENGL_FILES}
  ${NVHELPERSVK_SOURCE}
  ${NVHELPERSGL_SOURCE}
  ${NVHELPERSDX12_SOURCE}
  ${PACKAGE_SOURCE_FILES}
  ${NVHELPERSOPTIX_FILES}
)

# Create precompiled header (added in CMake 3.16)
if((${CMAKE_VERSION} VERSION_GREATER "3.16.0") OR (${CMAKE_VERSION} VERSION_EQUAL "3.16.0"))
  target_precompile_headers(nvpro_core PRIVATE
    "<algorithm>"
    ${VULKAN_HEADERS}
  )
endif()



target_link_libraries(nvpro_core
  ${PLATFORM_LIBRARIES} 
  cgltf
  glfw 
  imgui
  stb
  tinygltf
  tinyobjloader
  vma
  nvtx
 )


set_target_properties(nvpro_core PROPERTIES OUTPUT_NAME ${library_name})
_set_target_output(nvpro_core)

message(STATUS "nvpro_core library name: ${library_name}")

