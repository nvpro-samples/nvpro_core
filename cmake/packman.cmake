#*****************************************************************************
# Copyright 2023 NVIDIA Corporation. All rights reserved.
#*****************************************************************************
include_guard(GLOBAL)

unset(PYTHON_VERSION)


if (EXISTS "${BASE_DIRECTORY}/nvpro_core/OV")
  message(STATUS "Packman is available, toggle off USE_PACKMAN to disable")
  option(USE_PACKMAN "Enable to use packman dependencies where possible" ON)
else()
  option(USE_PACKMAN "Enable to use packman dependencies where possible" OFF)
endif()

if (WIN32)
  set(PACKMAN_PLATFORM "windows-x86_64" CACHE INTERNAL "")
  set(PACKMAN_COMMAND "packman.cmd" CACHE INTERNAL "")
elseif (UNIX)
  set(PACKMAN_PLATFORM "linux-${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
  set(PACKMAN_COMMAND "packman" CACHE INTERNAL "")
endif()

if (DEBUG)
  set(PACKMAN_CONFIG "debug" CACHE INTERNAL "")
else()
  set(PACKMAN_CONFIG "release" CACHE INTERNAL "")
endif()

# Configure all-deps file
function(configure_all_deps_file)
  set(oneValueArgs ALL_DEPS_FILEPATH PYTHON_VERSION_STRING)
  cmake_parse_arguments(PACKMAN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT DEFINED PACKMAN_ALL_DEPS_FILEPATH)
    set(PACKMAN_ALL_DEPS_FILEPATH "${BASE_DIRECTORY}/nvpro_core/OV/all-deps.packman.xml")
    message(STATUS "Using default packman all-deps ${PACKMAN_ALL_DEPS_FILEPATH}")
  else()
    message(STATUS "Using custom packman all-deps ${PACKMAN_ALL_DEPS_FILEPATH}")
  endif()

  if (NOT DEFINED PACKMAN_PYTHON_VERSION_STRING)
    set(PACKMAN_PYTHON_VERSION_STRING "py37")
    message(WARNING "PYTHON_VERSION_STRING not defined, defaulting to py37 (none, py37, py310)")
  endif()

  message(STATUS "PACKMAN_PYTHON_VERSION_STRING ${PACKMAN_PYTHON_VERSION_STRING}")
  message(STATUS "PACKMAN_ALL_DEPS_FILEPATH ${PACKMAN_ALL_DEPS_FILEPATH}")

  set(PYTHON_VERSION ${PACKMAN_PYTHON_VERSION_STRING} CACHE INTERNAL "")

  file(MAKE_DIRECTORY "${BASE_DIRECTORY}/nvpro_core/OV/downloaded")
  configure_file(
    "${PACKMAN_ALL_DEPS_FILEPATH}"
    "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/all-deps.packman.xml"
    @ONLY
  )
endfunction()

# Use packman to pull dependencies from ${BASE_DIRECTORY}/nvpro_core/OV/${PACKMAN_DEPENDENCY_FILE} where DEPENDENCY_FILE is an arg
function(pull_dependencies)
  set(oneValueArgs DEPENDENCY_FILE COMMAND)
  cmake_parse_arguments(PACKMAN  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT EXISTS ${BASE_DIRECTORY}/nvpro_core/OV/packman/${PACKMAN_COMMAND})
    message(FATAL_ERROR "PACKMAN_COMMAND \"${BASE_DIRECTORY}/nvpro_core/OV/packman/${PACKMAN_COMMAND}\" does not exist!")
  endif()

  if (NOT EXISTS ${BASE_DIRECTORY}/nvpro_core/OV/${PACKMAN_DEPENDENCY_FILE})
    message(FATAL_ERROR "PACKMAN_DEPENDENCY_FILE \"${BASE_DIRECTORY}/nvpro_core/OV/${PACKMAN_DEPENDENCY_FILE}\" does not exist!")
  endif()

  message(STATUS "Pulling dependencies from ${PACKMAN_DEPENDENCY_FILE}...")
  message(STATUS "    Platform: ${PACKMAN_PLATFORM}")
  message(STATUS "    Config:   ${PACKMAN_CONFIG}")
  message(STATUS "    Python:   ${PYTHON_VERSION}")
  
  file(MAKE_DIRECTORY "${BASE_DIRECTORY}/nvpro_core/OV/downloaded")
  configure_file(
    "${BASE_DIRECTORY}/nvpro_core/OV/${PACKMAN_DEPENDENCY_FILE}"
    "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/${PACKMAN_DEPENDENCY_FILE}"
    @ONLY
  )

  message(STATUS "execute_process(COMMAND ${BASE_DIRECTORY}/nvpro_core/OV/packman/${PACKMAN_COMMAND} pull ${BASE_DIRECTORY}/nvpro_core/OV/downloaded/${PACKMAN_DEPENDENCY_FILE} -p \"${PACKMAN_PLATFORM}\" -t config=${PACKMAN_CONFIG}
                  WORKING_DIRECTORY ${BASE_DIRECTORY}/nvpro_core/OV/downloaded
                  RESULT_VARIABLE PACKMAN_RESULT)")

  execute_process(COMMAND "${BASE_DIRECTORY}/nvpro_core/OV/packman/${PACKMAN_COMMAND}" pull "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/${PACKMAN_DEPENDENCY_FILE}" -p "${PACKMAN_PLATFORM}" -t config=${PACKMAN_CONFIG}
                  WORKING_DIRECTORY "${BASE_DIRECTORY}/nvpro_core/OV/downloaded"
                  RESULT_VARIABLE PACKMAN_RESULT)

  if (${PACKMAN_RESULT} EQUAL 0)
    message(STATUS "Packman result: success")
  else()
    message(FATAL_ERROR "Packman result: ${PACKMAN_RESULT}")
  endif()                  
endfunction()
