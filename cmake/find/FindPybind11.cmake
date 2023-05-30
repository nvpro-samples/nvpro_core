# Try to find PYBIND11 project so and include file
#
set(PYBIND11_LOCATION "" CACHE STRING "Set to location of pybind11 library/headers")
unset(PYBIND11_INCLUDE_DIR CACHE)
unset(PYBIND11_FOUND CACHE)

if(EXISTS ${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
  include(${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
endif()

if(USE_PACKMAN)
  message(STATUS "attempting to using packman to source pybind11")

  pull_dependencies(DEPENDENCY_FILE "pybind11-deps.packman.xml")

  set(PYBIND11_LOCATION "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/pybind11")
endif()

if (NOT DEFINED PYBIND11_LOCATION)
  message(WARNING "PYBIND11_LOCATION is not defined")
elseif(NOT EXISTS ${PYBIND11_LOCATION})
  message(WARNING "PYBIND11_LOCATION doesn't exist")
endif()

find_path(PYBIND11_INCLUDE_DIR
  NAMES pybind11/pybind11.h
  PATHS ${PYBIND11_LOCATION}
)

if(PYBIND11_INCLUDE_DIR)
  message(STATUS " pybind11.h found in ${PYBIND11_INCLUDE_DIR}")

  set( PYBIND11_FOUND "YES" )

else(PYBIND11_INCLUDE_DIR)
  message(WARNING "
      pybind11 not found.")
endif(PYBIND11_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Pybind11 DEFAULT_MSG
    PYBIND11_INCLUDE_DIR
)

set(PYBIND11_INCLUDE_DIR ${PYBIND11_INCLUDE_DIR} CACHE PATH "path")

mark_as_advanced( PYBIND11_FOUND )
