# Try to find KIT_SDK project so and include file
#
unset(KIT_SDK_LIBRARIES CACHE)
unset(KIT_SDK_INCLUDE_DIR CACHE)
unset(KIT_SDK_LIBRARY_DIR CACHE)
unset(KIT_SDK_FOUND CACHE)

if (WIN32)
  set(KIT_SDK_PACKMAN_PLATFORM "windows-x86_64")
  set(USE_PACKMAN_COMMAND "packman.cmd")
elseif (UNIX)
  set(KIT_SDK_PACKMAN_PLATFORM "linux-${CMAKE_HOST_SYSTEM_PROCESSOR}")
  set(USE_PACKMAN_COMMAND "packman")
endif()

message(STATUS "Pulling KIT_SDK dependencies...")
message(STATUS "    Platform: ${KIT_SDK_PACKMAN_PLATFORM}")

file(COPY "${BASE_DIRECTORY}/nvpro_core/OV/kit-sdk-deps.packman.xml"
        DESTINATION "${BASE_DIRECTORY}/nvpro_core/OV/downloaded")

execute_process(COMMAND "${BASE_DIRECTORY}/nvpro_core/OV/packman/${USE_PACKMAN_COMMAND}" pull "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/kit-sdk-deps.packman.xml" -p ${KIT_SDK_PACKMAN_PLATFORM}
                WORKING_DIRECTORY "${BASE_DIRECTORY}/nvpro_core/OV/downloaded"
                RESULT_VARIABLE KIT_SDK_PACKMAN_RESULT)

Message(STATUS "execute_process(COMMAND ${BASE_DIRECTORY}/nvpro_core/OV/packman/${USE_PACKMAN_COMMAND} pull ${BASE_DIRECTORY}/nvpro_core/OV/downloaded/kit-sdk-deps.packman.xml -p ${KIT_SDK_PACKMAN_PLATFORM}
                WORKING_DIRECTORY ${BASE_DIRECTORY}/nvpro_core/OV/downloaded
                RESULT_VARIABLE KIT_SDK_PACKMAN_RESULT)")

if (${KIT_SDK_PACKMAN_RESULT} EQUAL 0)
  message(STATUS "    Packman result: success")
else()
  message(FATAL_ERROR "    Packman result: ${KIT_SDK_PACKMAN_RESULT}")
endif()

set(KitSDK_DIR "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/kit")

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
)

# Do we have to rewrite the variable here...
set(KIT_SDK_LAUNCH_SCRIPT ${KIT_SDK_LAUNCH_SCRIPT} CACHE FILEPATH "filepath")
set(KIT_APP               ${KIT_APP} CACHE FILEPATH "filepath")
set(KitSDK_DIR            ${KitSDK_DIR} CACHE PATH "path")

mark_as_advanced( KIT_SDK_FOUND )
