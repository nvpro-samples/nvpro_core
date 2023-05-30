# Try to find KIT_SDK project so and include file
#
unset(KIT_SDK_LIBRARIES CACHE)
unset(KIT_SDK_INCLUDE_DIR CACHE)
unset(KIT_SDK_LIBRARY_DIR CACHE)
unset(KIT_SDK_FOUND CACHE)
unset(KIT_SDK_ALL_DEPS_FILE CACHE)

if(EXISTS ${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
  include(${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
endif()

if(USE_PACKMAN)
  message(STATUS "attempting to using packman to source kit-sdk")

  pull_dependencies(DEPENDENCY_FILE "kit-sdk-deps.packman.xml")

  set(KitSDK_DIR "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/kit")

  find_file(KIT_SDK_ALL_DEPS_FILE
      NAMES all-deps.packman.xml
      PATHS ${KitSDK_DIR}/dev
  )
  if (NOT KIT_SDK_ALL_DEPS_FILE)
    message(WARNING "Kit all-deps.packman.xml not found.")
  endif()
endif()

if (WIN32)
    find_file(KIT_SDK_LAUNCH_SCRIPT
        NAMES omni.app.full.bat
        PATHS ${KitSDK_DIR}
    )
    find_file(KIT_APP
        NAMES kit.exe
        PATHS ${KitSDK_DIR}
    )
elseif(UNIX)
    find_file(KIT_SDK_LAUNCH_SCRIPT
        NAMES omni.app.full.sh
        PATHS ${KitSDK_DIR}
    )
    find_file(KIT_APP
        NAMES kit
        PATHS ${KitSDK_DIR}
    )
endif()

if(KIT_SDK_LAUNCH_SCRIPT)
  message(STATUS " Kit launch script found at ${KIT_SDK_LAUNCH_SCRIPT}")
else(KIT_SDK_LAUNCH_SCRIPT)
  message(WARNING "
      Kit launch script not found.")
endif(KIT_SDK_LAUNCH_SCRIPT)


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(KitSDK DEFAULT_MSG
  KIT_SDK_LAUNCH_SCRIPT
  KIT_APP
  KitSDK_DIR
  KIT_SDK_ALL_DEPS_FILE
)

# Do we have to rewrite the variable here...
set(KIT_SDK_LAUNCH_SCRIPT ${KIT_SDK_LAUNCH_SCRIPT} CACHE FILEPATH "filepath")
set(KIT_APP               ${KIT_APP} CACHE FILEPATH "filepath")
set(KitSDK_DIR            ${KitSDK_DIR} CACHE PATH "path")

mark_as_advanced( KIT_SDK_FOUND )
