# Find CDIO - The GNU Compact Disc Input and Control Library

# This module defines
#  CDIO_FOUND        if cdio was found
#  CDIO_INCLUDE_DIR  where to find the headers
#  CDIO_LIBRARIES    the libraries to link against.

set(CDIO_FOUND false)

find_path(CDIO_INCLUDE_DIR cdio/cdio.h)
find_library(CDIO_LIBRARIES NAMES cdio)

if (CDIO_INCLUDE_DIR AND CDIO_LIBRARIES)
    set(CDIO_FOUND true)
endif(CDIO_INCLUDE_DIR AND CDIO_LIBRARIES)

if (CDIO_FOUND)
    if (NOT Cdio_FIND_QUIETLY)
        message(STATUS "Found CDIO: ${CDIO_LIBRARIES}")
    endif (NOT Cdio_FIND_QUIETLY)
else (CDIO_FOUND)
    if (Cdio_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find CDIO library")
    endif (Cdio_FIND_REQUIRED)
endif (CDIO_FOUND)
