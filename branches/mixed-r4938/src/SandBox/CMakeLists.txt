PROJECT(sandbox)

IF(INTEL_MKL)
  LINK_LIBRARIES(${INTEL_GUIDE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
ELSE(INTEL_MKL)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS_INIT})
ENDIF(INTEL_MKL)

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  LINK_LIBRARIES(${LAPACK_LIBRARY} ${BLAS_LIBRARY} ${FORTRAN_LIBS})
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

ADD_EXECUTABLE(mpi_group group.cpp)
TARGET_LINK_LIBRARIES(mpi_group qmc qmcbase qmcutil)

ADD_EXECUTABLE( observable_helper_test  observable_helper_test.cpp)
TARGET_LINK_LIBRARIES( observable_helper_test qmcbase qmcutil)

ADD_EXECUTABLE( einspline_bench  einspline.cpp)
TARGET_LINK_LIBRARIES(einspline_bench qmcbase qmcutil)

ADD_EXECUTABLE( einspline_smp  einspline_smp.cpp)
TARGET_LINK_LIBRARIES(einspline_smp qmcbase qmcutil)

#ADD_EXECUTABLE(gnubug gnubug.cpp)
#TARGET_LINK_LIBRARIES(gnubug qmcbase qmcutil)


#FIND_LIBRARY(BOOST_SERIALIZATION_LIBRARY  boost_serialization-il-1_33
#/u/ncsa/jnkim/share/boost/bin/boost/libs/serialization/build/libboost_serialization.so/intel-linux/release/shared-linkable-true/
#)

#ADD_EXECUTABLE(serialization serialization.cpp)
#TARGET_LINK_LIBRARIES(serialization boost)

#ADD_EXECUTABLE(cubicspline cubicspline.cpp)
#TARGET_LINK_LIBRARIES(cubicspline qmcbase qmcutil)
#
#ADD_EXECUTABLE(cubicspline1 cubicspline1.cpp)
#TARGET_LINK_LIBRARIES(cubicspline1 qmcbase qmcutil)
#
#ADD_EXECUTABLE(bspline bspline.cpp)
#TARGET_LINK_LIBRARIES(bspline qmc qmcbase)
#
#ADD_EXECUTABLE(tribsplineComplex tribsplineComplex.cpp)
#TARGET_LINK_LIBRARIES(tribsplineComplex qmc qmcbase)
#ADD_EXECUTABLE(tribsplineReal tribsplineReal.cpp)
#TARGET_LINK_LIBRARIES(tribsplineReal qmc qmcbase)


#INCLUDE(${PROJECT_CMAKE}/FindBlitz++.cmake)
#INCLUDE_DIRECTORIES(${BLITZ_INCLUDE_DIR})
#ADD_EXECUTABLE(bspline_blitz bspline_blitz.cpp)
#TARGET_LINK_LIBRARIES(bspline_blitz qmc qmcbase)

#ADD_EXECUTABLE(reduce_test reduce_performance_test.cpp)
#TARGET_LINK_LIBRARIES(reduce_test boost)
