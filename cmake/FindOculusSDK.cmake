# Try to find SvcMFCUI project dll and include file
#
include(FindPackageHandleStandardArgs)

unset(OCULUSSDK_LIBS CACHE)
unset(OCULUSSDK_LIBS_DEBUG CACHE)
unset(OCULUSSDK_INCLUDE_DIRS CACHE)
unset(OCULUSSDK_FOUND CACHE)

if( DEFINED ENV{OCULUSSDK_LOCATION} )
  set(OCULUSSDK_LOCATION "$ENV{OCULUSSDK_LOCATION}")
endif()

set(OCULUSSDK_LOCATION "${OCULUSSDK_LOCATION}" CACHE PATH "Oculus SDK root directory")


if(OCULUSSDK_LOCATION)

  # put together the include dirs
  list(APPEND OCULUSSDK_INCLUDE_DIRS ${OCULUSSDK_LOCATION}/LibOVR/Include)
  list(APPEND OCULUSSDK_INCLUDE_DIRS ${OCULUSSDK_LOCATION}/LibOVRKernel/Src)
  mark_as_advanced(OCULUSSDK_INCLUDE_DIRS)

  # find the Oculus VR lib (libOVR)
  # TODO: Linux handling
  if(MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_OVR_ARCH "x64")
    else()
      set(_OVR_ARCH "Win32")
    endif()
  else()
    message(ERROR "OVR Linux support not yet implemented.")
  endif()
  mark_as_advanced(_OVR_ARCH)

  if(MSVC)
    if(MSVC11)
      set(_OVR_MSVC "VS2012")
    elseif(MSVC12)
      set(_OVR_MSVC "VS2013")
    else()
      message(ERROR "FindOculusSDK: unsupported MSVC version.")
    endif()
    mark_as_advanced(_OVR_MSVC)
  endif()


  # TODO: Linux handling
  find_library(OCULUSSDK_LIB LibOVR.lib HINTS 
                ${OCULUSSDK_LOCATION}/LibOVR/Lib/Windows/${_OVR_ARCH}/Release/${_OVR_MSVC}
              )  
  mark_as_advanced(OCULUSSDK_LIB)
             
  find_library(OCULUSSDK_KERNEL_LIB LibOVRKernel.lib HINTS 
                ${OCULUSSDK_LOCATION}/LibOVRKernel/Lib/Windows/${_OVR_ARCH}/Release/${_OVR_MSVC}
              )  
  mark_as_advanced(OCULUSSDK_KERNEL_LIB)
             
  find_library(OCULUSSDK_LIB_DEBUG LibOVR.lib HINTS 
                ${OCULUSSDK_LOCATION}/LibOVR/Lib/Windows/${_OVR_ARCH}/Debug/${_OVR_MSVC}
              )  
  mark_as_advanced(OCULUSSDK_LIB_DEBUG)

  find_library(OCULUSSDK_KERNEL_LIB_DEBUG LibOVRKernel.lib HINTS 
                ${OCULUSSDK_LOCATION}/LibOVRKernel/Lib/Windows/${_OVR_ARCH}/Debug/${_OVR_MSVC}
              )  
  mark_as_advanced(OCULUSSDK_KERNEL_LIB_DEBUG)
             
  list(APPEND OCULUSSDK_LIBS ${OCULUSSDK_LIB})
  list(APPEND OCULUSSDK_LIBS ${OCULUSSDK_KERNEL_LIB})
  mark_as_advanced(OCULUSSDK_LIBS)

  list(APPEND OCULUSSDK_LIBS_DEBUG ${OCULUSSDK_LIB_DEBUG})
  list(APPEND OCULUSSDK_LIBS_DEBUG ${OCULUSSDK_KERNEL_LIB_DEBUG})
  mark_as_advanced(OCULUSSDK_LIBS_DEBUG)


  if(OCULUSSDK_LIB)
    set( OCULUSSDK_FOUND "YES" )
  endif(OCULUSSDK_LIB)

else(OCULUSSDK_LOCATION)

  message(WARNING "
      OculusSDK not found. 
      The OCULUSSDK folder you would specify with the OCULUSSDK_LOCATION env var and should contain
      LibOVR and LibOVRKernel folders in the structure the Oculus SDK is delivered."
  )

endif(OCULUSSDK_LOCATION)

find_package_handle_standard_args(OCULUSSDK DEFAULT_MSG
  OCULUSSDK_LOCATION
  OCULUSSDK_INCLUDE_DIRS
  OCULUSSDK_LIBS
  OCULUSSDK_LIBS_DEBUG
  OCULUSSDK_INCLUDE_DIRS
)

mark_as_advanced( OCULUSSDK_FOUND )

