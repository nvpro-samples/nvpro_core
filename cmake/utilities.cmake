#*****************************************************************************
# Copyright 2020 NVIDIA Corporation. All rights reserved.
#*****************************************************************************
include_guard(GLOBAL)

# -------------------------------------------------------------------------------------------------
# function that copies a list of files into the target directory
#
#   target_copy_to_output_dir(TARGET foo
#       [RELATIVE <path_prefix>]                                # allows to keep the folder structure starting from this level
#       FILES <absolute_file_path> [<absolute_file_path>]
#       )
#
function(target_copy_to_output_dir)
    set(options)
    set(oneValueArgs TARGET RELATIVE DEST_SUBFOLDER)
    set(multiValueArgs FILES)
    cmake_parse_arguments(TARGET_COPY_TO_OUTPUT_DIR "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    foreach(_ELEMENT ${TARGET_COPY_TO_OUTPUT_DIR_FILES} )

        # handle absolute and relative paths
        if(TARGET_COPY_TO_OUTPUT_DIR_RELATIVE)
            set(_SOURCE_FILE ${TARGET_COPY_TO_OUTPUT_DIR_RELATIVE}/${_ELEMENT})
            set(_FOLDER_PATH ${_ELEMENT})
        else()
            set(_SOURCE_FILE ${_ELEMENT})
            get_filename_component(_FOLDER_PATH ${_ELEMENT} NAME)
            set (_ELEMENT "")
        endif()

        # handle directories and files slightly different
        if(IS_DIRECTORY ${_SOURCE_FILE})
            if(MDL_LOG_FILE_DEPENDENCIES)
                MESSAGE(STATUS "- folder to copy: ${_SOURCE_FILE}")
            endif()
            add_custom_command(
                TARGET ${TARGET_COPY_TO_OUTPUT_DIR_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${_SOURCE_FILE} $<TARGET_FILE_DIR:${TARGET_COPY_TO_OUTPUT_DIR_TARGET}>/${TARGET_COPY_TO_OUTPUT_DIR_DEST_SUBFOLDER}${_FOLDER_PATH}
            )
        else()   
            if(MDL_LOG_FILE_DEPENDENCIES)
                MESSAGE(STATUS "- file to copy:   ${_SOURCE_FILE}")
            endif()
            add_custom_command(
                TARGET ${TARGET_COPY_TO_OUTPUT_DIR_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_SOURCE_FILE} $<TARGET_FILE_DIR:${TARGET_COPY_TO_OUTPUT_DIR_TARGET}>/${TARGET_COPY_TO_OUTPUT_DIR_DEST_SUBFOLDER}${_ELEMENT}
            )
        endif()
    endforeach()
endfunction()


#------------------------------------------------------------------------------------
# Downloading the URL to FILENAME and extract its content if EXTRACT option is present
# ZIP files should have a folder of the name of the archive
# - ex. foo.zip -> foo/<data>
# Arguements
#  FILENAMES   : all filenames to download
#  EXTRACT     : if present, will extract the content of the file
#  NOINSTALL   : if present, will not make files part of install
#  INSTALL_DIR : folder for the 'install' build, default is 'media' next to the executable
#  TARGET_DIR  : folder where to download to, default is {DOWNLOAD_TARGET_DIR}
#  SOURCE_DIR  : folder on server, if not present 'scenes'
#
# Examples:
# download_files(FILENAMES sample1.zip EXTRACT) 
# download_files(FILENAMES env.hdr)
# download_files(FILENAMES zlib.zip EXTRACT TARGET_DIR ${BASE_DIRECTORY}/blah SOURCE_DIR libraries NOINSTALL) 
# 
function(download_files)
  set(options EXTRACT NOINSTALL)
  set(oneValueArgs INSTALL_DIR SOURCE_DIR TARGET_DIR)
  set(multiValueArgs FILENAMES)
  cmake_parse_arguments(DOWNLOAD_FILES  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if(NOT DEFINED DOWNLOAD_FILES_INSTALL_DIR)
    set(DOWNLOAD_FILES_INSTALL_DIR "media")
  endif()
  if(NOT DEFINED DOWNLOAD_FILES_SOURCE_DIR)
    set(DOWNLOAD_FILES_SOURCE_DIR "")
  endif()
  if(NOT DEFINED DOWNLOAD_FILES_TARGET_DIR)
    set(DOWNLOAD_FILES_TARGET_DIR ${DOWNLOAD_TARGET_DIR})
  endif()

  # Check each file to download
  foreach(FILENAME ${DOWNLOAD_FILES_FILENAMES})
   
    set(TARGET_FILENAME ${DOWNLOAD_FILES_TARGET_DIR}/${FILENAME})
    if(NOT EXISTS ${TARGET_FILENAME})
      message(STATUS "Downloading ${DOWNLOAD_SITE}/${FILENAME} to ${TARGET_FILENAME}")
      file(DOWNLOAD ${DOWNLOAD_SITE}${DOWNLOAD_FILES_SOURCE_DIR}/${FILENAME} ${TARGET_FILENAME} SHOW_PROGRESS)
  
      # Extracting the ZIP file
	    if(DOWNLOAD_FILES_EXTRACT)
		    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ${TARGET_FILENAME}
						          WORKING_DIRECTORY ${DOWNLOAD_FILES_TARGET_DIR})
        # ARCHIVE_EXTRACT needs CMake 3.18+
        # file(ARCHIVE_EXTRACT INPUT ${TARGET_FILENAME}
        #      DESTINATION ${DOWNLOAD_FILES_TARGET_DIR})
      endif()
    endif()

    # Installing the files or directory
    if (NOT DOWNLOAD_FILES_NOINSTALL)
      if(DOWNLOAD_FILES_EXTRACT)
       get_filename_component(FILE_DIR ${FILENAME} NAME_WE)
       install(DIRECTORY ${DOWNLOAD_FILES_TARGET_DIR}/${FILE_DIR} CONFIGURATIONS Release DESTINATION "bin_${ARCH}/${DOWNLOAD_FILES_INSTALL_DIR}")
       install(DIRECTORY ${DOWNLOAD_FILES_TARGET_DIR}/${FILE_DIR} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${DOWNLOAD_FILES_INSTALL_DIR}")
      else()
       install(FILES ${TARGET_FILENAME} CONFIGURATIONS Release DESTINATION "bin_${ARCH}/${DOWNLOAD_FILES_INSTALL_DIR}")
       install(FILES ${TARGET_FILENAME} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${DOWNLOAD_FILES_INSTALL_DIR}")
      endif()
    endif()

  endforeach()
endfunction()

#------------------------------------------------------------------------------------
# Find dependencies for GLSL files (#include ...)
# Call 'glslc -M' to find all dependencies of the file and return the list
# in GLSL_DEPENDENCY
#
function(get_glsl_dependecies _SRC _FLAGS)
   
  get_filename_component(FILE_NAME ${_SRC} NAME)
  get_filename_component(DIR_NAME ${_SRC} DIRECTORY)

  message(STATUS " - Find dependencies for ${FILE_NAME}")
  #message(STATUS "calling : ${GLSLC} ${_FLAGS} -M ${_SRC} OUTPUT_VARIABLE DEP RESULT_VARIABLE RES")
  separate_arguments(_FLAGS)
  execute_process(COMMAND ${GLSLC} ${_FLAGS} -M ${_SRC} OUTPUT_VARIABLE DEP RESULT_VARIABLE RES )
  if(RES EQUAL 0)
    # Removing "name.spv: "
    string(REGEX REPLACE "[^:]*: " "" DEP ${DEP})
    # Splitting each path with a ';' 
    string(REPLACE " ${DIR_NAME}"  ";${DIR_NAME}" DEP ${DEP})
    set(GLSL_DEPENDENCY ${DEP} PARENT_SCOPE)
  endif()
endfunction()


#------------------------------------------------------------------------------------
# Function to compile all GLSL source files to Spir-V
#
# SOURCE_FILES : All sources to compile
# HEADER_FILES : Dependencie header files
# DST : The destination directory (need to be absolute)
# VULKAN_TARGET : to define the vulkan target i.e vulkan1.2 (default vulkan1.1)
# HEADER ON: if ON, will generate headers instead of binary Spir-V files
# DEPENDENCY : ON|OFF will create the list of dependencies for the GLSL source file
# 
# compile_glsl(
#   SOURCES_FILES foo.vert foo.frag
#   DST ${CMAKE_CURRENT_SOURCE_DIR}/shaders
#   FLAGS -g0
# )

function(compile_glsl)
  set(oneValueArgs DST VULKAN_TARGET HEADER DEPENDENCY FLAGS)
  set(multiValueArgs SOURCE_FILES HEADER_FILES)
  cmake_parse_arguments(COMPILE  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  # Check if the GLSL compiler is present
  if(NOT GLSLANGVALIDATOR)
    message(ERROR "Could not find GLSLANGVALIDATOR to compile shaders")
    return()
  endif()

  # By default use Vulkan 1.1
  if(NOT DEFINED COMPILE_VULKAN_TARGET)
    set(COMPILE_VULKAN_TARGET vulkan1.1)
  endif()

  # If destination is not present, same as source
  if(NOT DEFINED COMPILE_DST)
    message(ERROR " --- DST not defined")
    return()
  endif()

  # Make the output directory if needed
  file(MAKE_DIRECTORY ${COMPILE_DST})

  # If no flag set -g (debug)
  if(NOT DEFINED COMPILE_FLAGS)
    set(COMPILE_FLAGS -g)
  endif()

  separate_arguments(_FLG UNIX_COMMAND ${COMPILE_FLAGS})

  # Compiling all GLSL sources
  foreach(GLSL_SRC ${COMPILE_SOURCE_FILES})

    # Find the dependency files for the GLSL source
    # or use all headers as dependencies.
    if(COMPILE_DEPENDENCY)
        get_glsl_dependecies(${GLSL_SRC} ${COMPILE_FLAGS})
    else()
      set(GLSL_DEPENDENCY ${COMPILE_HEADER_FILES}) 
    endif()

    # Default compiler command, always adding debug information (Add and option to opt-out?)
    set(COMPILE_CMD  ${_FLG} --target-env ${COMPILE_VULKAN_TARGET})

    # Compilation to headers need a variable name, the output will be a .h
    get_filename_component(FILE_NAME ${GLSL_SRC} NAME)
    if(COMPILE_HEADER)           
        STRING(REPLACE "." "_" VAR_NAME ${FILE_NAME}) # Name of the variable in the header
        list(APPEND COMPILE_CMD  --vn ${VAR_NAME})
        set(GLSL_OUT "${COMPILE_DST}/${FILE_NAME}.h")
    else()
        set(GLSL_OUT "${COMPILE_DST}/${FILE_NAME}.spv")
        list(APPEND _SPVS ${GLSL_OUT})
    endif() 


    # Appending the output name and the file source
    list(APPEND COMPILE_CMD  -o ${GLSL_OUT} ${GLSL_SRC} )

    # The custom command is added to the build system, check for the presence of the output
    # but also for changes done in GLSL headers 
    add_custom_command(
         PRE_BUILD
         OUTPUT ${GLSL_OUT}
         COMMAND echo ${GLSLANGVALIDATOR} ${COMPILE_CMD}
         COMMAND ${GLSLANGVALIDATOR} ${COMPILE_CMD}
         MAIN_DEPENDENCY ${GLSL_SRC}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
         DEPENDS ${GLSL_DEPENDENCY}
      )
  endforeach()

  # Setting OUT variables 
  set(GLSL_SOURCES ${COMPILE_SOURCE_FILES} PARENT_SCOPE)
  set(GLSL_HEADERS ${COMPILE_HEADER_FILES} PARENT_SCOPE)
  set(SPV_OUTPUT ${_SPVS} PARENT_SCOPE)

endfunction()



#------------------------------------------------------------------------------------
# Function to compile all GLSL files from a source to Spir-V
# The sources are all .vert, .frag, .r*  and the headers for the source are .glsl and .h
# This allows to modify one of the header and getting the sources recompiled.
#
# SRC : The directory source of the shaders
# DST : The destination directory (need to be absolute)
# VULKAN_TARGET : to define the vulkan target i.e vulkan1.2 (default vulkan1.1)
# HEADER ON: if present, will generate headers instead of binary Spir-V files
# DEPENDENCY : ON|OFF will create the list of dependencies for the GLSL source file 
# FLAGS : other glslValidator flags 
#
# compile_glsl_directory(
#    SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders" 
#    DST "${CMAKE_CURRENT_SOURCE_DIR}/autogen" 
#    VULKAN_TARGET "vulkan1.2"
#    HEADER ON
#    )
#
function(compile_glsl_directory)
  set(oneValueArgs SRC DST VULKAN_TARGET HEADER DEPENDENCY FLAGS)
  set(multiValueArgs)
  cmake_parse_arguments(COMPILE  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Collecting all source files
  file(GLOB GLSL_SOURCE_FILES
    "${COMPILE_SRC}/*.comp"     # Compute
    "${COMPILE_SRC}/*.frag"     # Fragment
    "${COMPILE_SRC}/*.geom"     # Geometry
    "${COMPILE_SRC}/*.mesh"     # Mesh
    "${COMPILE_SRC}/*.rahit"    # Ray any hit
    "${COMPILE_SRC}/*.rcall"    # Ray callable
    "${COMPILE_SRC}/*.rchit"    # Ray closest hit
    "${COMPILE_SRC}/*.rgen"     # Ray generation
    "${COMPILE_SRC}/*.rint"     # Ray intersection
    "${COMPILE_SRC}/*.rmiss"    # Ray miss
    "${COMPILE_SRC}/*.task"     # Task
    "${COMPILE_SRC}/*.tesc"     # Tessellation control
    "${COMPILE_SRC}/*.tese"     # Tessellation evaluation
    "${COMPILE_SRC}/*.vert"     # Vertex
    )

  # Collecting headers for dependencies
  file(GLOB GLSL_HEADER_FILES
    "${COMPILE_SRC}/*.glsl"     # Auto detect - used for header
    "${COMPILE_SRC}/*.h"
    )

  # By default use Vulkan 1.1
  if(NOT DEFINED COMPILE_VULKAN_TARGET)
    set(COMPILE_VULKAN_TARGET vulkan1.1)
  endif()

  # If destination is not present, same as source
  if(NOT DEFINED COMPILE_DST)
    set(COMPILE_DST ${COMPILE_SRC})
  endif()

  # If no flag set -g (debug)
  if(NOT DEFINED COMPILE_FLAGS)
    set(COMPILE_FLAGS -g)
  endif()

  # Compiling all GLSL
  compile_glsl(SOURCE_FILES ${GLSL_SOURCE_FILES} 
               HEADER_FILES ${GLSL_HEADER_FILES}  
               DST ${COMPILE_DST} 
               VULKAN_TARGET ${COMPILE_VULKAN_TARGET} 
               HEADER ${COMPILE_HEADER}
               DEPENDENCY ${COMPILE_DEPENDENCY}
               FLAGS ${COMPILE_FLAGS}
               )

  # Setting OUT variables 
  set(GLSL_SOURCES ${GLSL_SOURCE_FILES} PARENT_SCOPE)
  set(GLSL_HEADERS ${GLSL_HEADER_FILES} PARENT_SCOPE)
  set(SPV_OUTPUT ${SPV_OUTPUT} PARENT_SCOPE) # propagate value set in compile_glsl
endfunction()

