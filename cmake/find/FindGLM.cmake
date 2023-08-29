include(FindPackageHandleStandardArgs)

find_path( GLM_INCLUDE_DIR glm/glm.hpp HINTS ${GLM_LOCATION}
                                             $ENV{GLM_LOCATION}
                                             $ENV{VK_SDK_PATH}/include
                                             ${VULKAN_HEADERS_OVERRIDE_INCLUDE_DIR}
                                             ${Vulkan_INCLUDE_DIR} )

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(GLM DEFAULT_MSG GLM_INCLUDE_DIR)

# Define GLM_INCLUDE_DIRS
if (GLM_FOUND)
	set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
endif()

# Hide some variables
mark_as_advanced(GLM_INCLUDE_DIR)