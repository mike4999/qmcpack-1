PROJECT(sandbox)

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
LINK_LIBRARIES(${FORTRAN_LIBS} 
${LAPACK_LIBRARY} 
${BLAS_LIBRARY} 
)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(INTEL_MKL)
  LINK_LIBRARIES(${INTEL_GUIDE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
ELSE(INTEL_MKL)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS_INIT})
ENDIF(INTEL_MKL)

ADD_EXECUTABLE(mpi_group group.cpp)
TARGET_LINK_LIBRARIES(mpi_group qmc qmcbase qmcutil)

#FIND_LIBRARY(BOOST_SERIALIZATION_LIBRARY  boost_serialization-il-1_33
#/u/ncsa/jnkim/share/boost/bin/boost/libs/serialization/build/libboost_serialization.so/intel-linux/release/shared-linkable-true/
#)

#ADD_EXECUTABLE(serialization serialization.cpp)
#TARGET_LINK_LIBRARIES(serialization boost)

ADD_EXECUTABLE(cubicspline cubicspline.cpp)
TARGET_LINK_LIBRARIES(cubicspline qmc qmcbase qmcutil)

ADD_EXECUTABLE(cubicspline1 cubicspline1.cpp)
TARGET_LINK_LIBRARIES(cubicspline1 qmc qmcbase qmcutil)

ADD_EXECUTABLE(tricubicspline tricubicspline.cpp)
TARGET_LINK_LIBRARIES(tricubicspline qmc qmcbase)

ADD_EXECUTABLE(tricubicbspline tricubicbspline.cpp)
TARGET_LINK_LIBRARIES(tricubicbspline qmc qmcbase)

#INCLUDE(${PROJECT_CMAKE}/FindBlitz++.cmake)
#INCLUDE_DIRECTORIES(${BLITZ_INCLUDE_DIR})
#ADD_EXECUTABLE(bspline_blitz bspline_blitz.cpp)
#TARGET_LINK_LIBRARIES(bspline_blitz qmc qmcbase)

#ADD_EXECUTABLE(reduce_test reduce_performance_test.cpp)
#TARGET_LINK_LIBRARIES(reduce_test boost)
