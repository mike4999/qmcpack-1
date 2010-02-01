#
# this module look for HDF5 (http://hdf.ncsa.uiuc.edu) support
# it will define the following values
#
# HDF5_INCLUDE_DIR  = where hdf5.h can be found
# HDF5_LIBRARY      = the library to link against (hdf5 etc)
# HDF5_FOUND        = set to true after finding the library
#
IF(EXISTS ${PROJECT_CMAKE}/Hdf5Config.cmake)
  INCLUDE(${PROJECT_CMAKE}/Hdf5Config.cmake)
ENDIF(EXISTS ${PROJECT_CMAKE}/Hdf5Config.cmake)

IF(Hdf5_INCLUDE_DIRS)

  FIND_PATH(HDF5_INCLUDE_DIR hdf5.h ${Hdf5_INCLUDE_DIRS})
  FIND_LIBRARY(HDF5_LIBRARY hdf5 ${Hdf5_LIBRARY_DIRS})

ELSE(Hdf5_INCLUDE_DIRS)

  #  SET(TRIAL_LIBRARY_PATHS
  #    $ENV{PHDF5_HOME}/lib
  #    $ENV{HDF5_HOME}/lib
  #    $ENV{HDF5_DIR}/lib
  #    $ENV{HDF_HOME}/lib
  #    /usr/apps/lib
  #    /usr/lib 
  #    /usr/local/lib
  #    /opt/lib
  #    /sw/lib
  #    )
  #
  #  SET(TRIAL_INCLUDE_PATHS
  #    $ENV{PHDF5_HOME}/include
  #    $ENV{HDF5_HOME}/include
  #    $ENV{HDF5_DIR}/include
  #    $ENV{HDF_HOME}/include
  #    /usr/apps/include
  #    /usr/include
  #    /opt/include
  #    /usr/local/include
  #    /sw/include
  #    )
  #
  #  FIND_LIBRARY(HDF5_LIBRARIES hdf5 ${TRIAL_LIBRARY_PATHS})
  #  FIND_PATH(HDF5_INCLUDE_DIR hdf5.h ${TRIAL_INCLUDE_PATHS} )
  FIND_LIBRARY(HDF5_LIBRARIES hdf5 ${QMC_LIBRARY_PATHS})
  FIND_PATH(HDF5_INCLUDE_DIR hdf5.h ude ${QMC_INCLUDE_PATHS})

ENDIF(Hdf5_INCLUDE_DIRS)

IF(HDF5_INCLUDE_DIR AND HDF5_LIBRARIES)
  MESSAGE(STATUS "HDF5_INCLUDE_DIR=${HDF5_INCLUDE_DIR}")
  MESSAGE(STATUS "HDF5_LIBRARIES=${HDF5_LIBRARIES}")
  SET(HDF5_FOUND TRUE)

  find_file(h5settings libhdf5.settings ${QMC_LIBRARY_PATHS})
  exec_program(grep  
    ARGS libraries ${h5settings}
    OUTPUT_VARIABLE HDF5_EXTRA_LIBS
    RETURN_VALUE mycheck
    )
  if(HDF5_EXTRA_LIBS MATCHES "sz")
    STRING(REGEX MATCHALL "[-][L]([^ ;])+" HDF5_EXTRA_LIBS_LINK  ${HDF5_EXTRA_LIBS})
    STRING(REGEX REPLACE "[-][L]" "" SZLIB_PATH ${HDF5_EXTRA_LIBS_LINK})
    find_library(SZLIB_LIBRARIES sz ${SZLIB_PATH})
    if(SZLIB_LIBRARIES)
      MESSAGE(STATUS "SZLIB_LIBRARIES="${SZLIB_LIBRARIES})
      set(SZLIB_FOUND TRUE)
    endif(SZLIB_LIBRARIES)
  endif(HDF5_EXTRA_LIBS MATCHES "sz")
ELSE()
  SET(HDF5_FOUND FALSE)
ENDIF()

MARK_AS_ADVANCED(
  HDF5_INCLUDE_DIR 
  HDF5_LIBRARIES 
  HDF5_FOUND
)