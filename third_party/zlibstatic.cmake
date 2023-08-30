set(ZLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib" )

# these options are problematic (see hints in assimp)
set( ASM686 FALSE CACHE INTERNAL "Override ZLIB flag to turn off assembly" FORCE )
set( AMD64 FALSE CACHE INTERNAL "Override ZLIB flag to turn off assembly" FORCE )


include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)

# Check to see if we have large file support.
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
if(HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
endif()
if(HAVE_STDDEF_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
endif()
check_type_size(off64_t SIZEOF_OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${ZLIB_DIR})
endif()

set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file( ${ZLIB_DIR}/zlib.pc.cmakein
		${ZLIB_PC} @ONLY)
configure_file(	${ZLIB_DIR}/zconf.h.cmakein
		${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})


#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
    ${ZLIB_DIR}/zlib.h
)
set(ZLIB_PRIVATE_HDRS
    ${ZLIB_DIR}/crc32.h
    ${ZLIB_DIR}/deflate.h
    ${ZLIB_DIR}/gzguts.h
    ${ZLIB_DIR}/inffast.h
    ${ZLIB_DIR}/inffixed.h
    ${ZLIB_DIR}/inflate.h
    ${ZLIB_DIR}/inftrees.h
    ${ZLIB_DIR}/trees.h
    ${ZLIB_DIR}/zutil.h
)
set(ZLIB_SRCS
    ${ZLIB_DIR}/adler32.c
    ${ZLIB_DIR}/compress.c
    ${ZLIB_DIR}/crc32.c
    ${ZLIB_DIR}/deflate.c
    ${ZLIB_DIR}/gzclose.c
    ${ZLIB_DIR}/gzlib.c
    ${ZLIB_DIR}/gzread.c
    ${ZLIB_DIR}/gzwrite.c
    ${ZLIB_DIR}/inflate.c
    ${ZLIB_DIR}/infback.c
    ${ZLIB_DIR}/inftrees.c
    ${ZLIB_DIR}/inffast.c
    ${ZLIB_DIR}/trees.c
    ${ZLIB_DIR}/uncompr.c
    ${ZLIB_DIR}/zutil.c
)

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${ZLIB_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})


add_library(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})

if(UNIX)
    # On unix-like platforms the library is almost always called libz
   set_target_properties(zlibstatic PROPERTIES OUTPUT_NAME z)

   # Tell zlib to include unistd.h, avoiding implicit declaration of write() etc.
   target_compile_definitions(zlibstatic PRIVATE HAVE_UNISTD_H)
endif()


set_property(TARGET zlibstatic PROPERTY FOLDER "ThirdParty")
target_include_directories(zlibstatic INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib" ${CMAKE_CURRENT_BINARY_DIR})

