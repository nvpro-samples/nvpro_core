cmake_minimum_required(VERSION 2.8)

project(shaderc_wrapper)

if (NOT WIN32)
  message( SEND_ERROR "This project is only meant for win32 applications" )
endif()


file(GLOB SHADERC_WRAPPER_FILES *.cpp *.hpp *.inl *.h *.c *.def)

if (MSVC)
  # shaderc_combined was built without debug runtime support
  # avoid problems by building the dll the same way
  set(CompilerFlags
          CMAKE_CXX_FLAGS
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_RELEASE
          CMAKE_C_FLAGS
          CMAKE_C_FLAGS_DEBUG
          CMAKE_C_FLAGS_RELEASE
          )
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MDd" "/MD" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()

  SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT" )
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS
  )
endif(MSVC)

add_library(shaderc_wrapper SHARED 
  ${SHADERC_WRAPPER_FILES}
)

if (NVSHADERC_LIB)
  target_link_libraries(shaderc_wrapper PRIVATE ${NVSHADERC_LIB})
  Message(STATUS "--> using NVShaderC for shaderc_wrapper")
else()
  target_link_libraries(shaderc_wrapper PRIVATE ${VULKANSDK_SHADERC_LIB})
endif()

install(TARGETS shaderc_wrapper RUNTIME CONFIGURATIONS Release DESTINATION "bin_${ARCH}")
install(TARGETS shaderc_wrapper RUNTIME CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug")