# Try to find OMNI_USD_RESOLVER project so and include file
#
unset(OMNI_USD_RESOLVER_LIBRARIES CACHE)
unset(OMNI_USD_RESOLVER_INCLUDE_DIR CACHE)
unset(OMNI_USD_RESOLVER_LIBRARY_DIR CACHE)
unset(OMNI_USD_RESOLVER_FOUND CACHE)

if (WIN32)
  set(OMNI_USD_RESOLVER_PACKMAN_PLATFORM "windows-x86_64")
  set(USE_PACKMAN_COMMAND "packman.cmd")
elseif (UNIX)
  set(OMNI_USD_RESOLVER_PACKMAN_PLATFORM "linux-${CMAKE_HOST_SYSTEM_PROCESSOR}")
  set(USE_PACKMAN_COMMAND "packman")
endif()

message(STATUS "Pulling OMNI_USD_RESOLVER dependencies...")
message(STATUS "    Platform: ${OMNI_USD_RESOLVER_PACKMAN_PLATFORM}")

file(COPY "${BASE_DIRECTORY}/nvpro_core/OV/omniusdresolver-deps.packman.xml"
        DESTINATION "${BASE_DIRECTORY}/nvpro_core/OV/downloaded")

execute_process(COMMAND "${BASE_DIRECTORY}/nvpro_core/OV/packman/${USE_PACKMAN_COMMAND}" pull "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/omniusdresolver-deps.packman.xml" -p ${OMNI_USD_RESOLVER_PACKMAN_PLATFORM}
                WORKING_DIRECTORY "${BASE_DIRECTORY}/nvpro_core/OV/downloaded"
                RESULT_VARIABLE OMNI_USD_RESOLVER_PACKMAN_RESULT)

Message(STATUS "execute_process(COMMAND ${BASE_DIRECTORY}/nvpro_core/OV/packman/${USE_PACKMAN_COMMAND} pull ${BASE_DIRECTORY}/nvpro_core/OV/downloaded/omniusdresolver-deps.packman.xml -p ${OMNI_USD_RESOLVER_PACKMAN_PLATFORM}
                WORKING_DIRECTORY ${BASE_DIRECTORY}/nvpro_core/OV/downloaded
                RESULT_VARIABLE OMNI_USD_RESOLVER_PACKMAN_RESULT)")


set(OMNI_USD_RESOLVER_DIR "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/omni_usd_resolver")

#message("OMNI_USD_RESOLVER_DIR: " ${OMNI_USD_RESOLVER_DIR})

if(OMNI_USD_RESOLVER_DIR)
    set(OMNI_USD_RESOLVER_PLUGINS_DIR "${OMNI_USD_RESOLVER_DIR}/$<IF:$<CONFIG:Debug>,debug,release>")
    set(OMNI_USD_RESOLVER_RESOURCES_DIR "${OMNI_USD_RESOLVER_DIR}/$<IF:$<CONFIG:Debug>,debug,release>/usd/omniverse/resources")
else(OMNI_USD_RESOLVER_DIR)
  message(WARNING "Omni usd resolver plugin not found.")
endif(OMNI_USD_RESOLVER_DIR)


include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OmniUsdResolver DEFAULT_MSG
    OMNI_USD_RESOLVER_PLUGINS_DIR
    OMNI_USD_RESOLVER_RESOURCES_DIR
)

# Do we have to rewrite the variable here...
set(OMNI_USD_RESOLVER_PLUGINS_DIR ${OMNI_USD_RESOLVER_PLUGINS_DIR} CACHE PATH "path")
set(OMNI_USD_RESOLVER_RESOURCES_DIR ${OMNI_USD_RESOLVER_RESOURCES_DIR} CACHE PATH "path")

mark_as_advanced( OMNI_USD_RESOLVER_FOUND )
