# Try to find OMNI_USD_RESOLVER project so and include file
#
unset(OMNI_USD_RESOLVER_LIBRARIES CACHE)
unset(OMNI_USD_RESOLVER_INCLUDE_DIR CACHE)
unset(OMNI_USD_RESOLVER_LIBRARY_DIR CACHE)
unset(OMNI_USD_RESOLVER_FOUND CACHE)

if ("nopy" IN_LIST OmniUsdResolver_FIND_COMPONENTS OR (DEFINED PYTHON_VERSION AND PYTHON_VERSION EQUAL "nopy"))
  set(INCLUDE_PYTHON 0)
  set(PYTHON_VERSION "nopy")
elseif()
  set(INCLUDE_PYTHON 1)
  if (NOT DEFINED PYTHON_VERSION)
    message(STATUS "PYTHON_VERSION must be set to compatible python version string ['py37, 'py310', or 'nopy']")
    message(STATUS "Defaulting PYTHON_VERSION to 'py37'")
    set(PYTHON_VERSION "py37")
  endif()
endif()

if(EXISTS ${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
  include(${BASE_DIRECTORY}/nvpro_core/cmake/utilities.cmake)
endif()

if(USE_PACKMAN)
  message(STATUS "attempting to using packman to source omni usd resolver")

  pull_dependencies(DEPENDENCY_FILE "omniusdresolver-deps.packman.xml")

  set(OMNI_USD_RESOLVER_DIR "${BASE_DIRECTORY}/nvpro_core/OV/downloaded/omni_usd_resolver")
endif()

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
