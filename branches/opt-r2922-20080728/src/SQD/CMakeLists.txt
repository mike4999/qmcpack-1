PROJECT(sqd)

#CONFIGURE_FILE(${hf_SOURCE_DIR}/../Configuration.h.in
#              ${hf_SOURCE_DIR}/../Configuration.h)

#INCLUDE(${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
#INCLUDE(${CMAKE_ROOT}/Modules/FindQt.cmake)
#INCLUDE(${PROJECT_CMAKE}/FindQwt.cmake)

SET(QT_FOUND 0)

IF(QT_FOUND) 
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR} ${QT_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_PREFIX_INSTALL}/include)
INCLUDE_DIRECTORIES(${QWT_INCLUDE_DIR})
                                                                                                      
ADD_DEFINITIONS(-DHAVE_QT)
LINK_LIBRARIES(${QWT_LIBRARY} ${QT_LIBRARIES} ${OPENGL_LIBRARIES})
ENDIF(QT_FOUND) 

SET(HFSRCS 
  ../Numerics/Clebsch_Gordan.cpp
  SphericalPotential/RadialPotential.cpp
  SphericalPotential/ZOverRPotential.cpp
  SphericalPotential/HarmonicPotential.cpp
  SphericalPotential/StepPotential.cpp
  SphericalPotential/SJPseudoPotential.cpp
  SphericalPotential/SHEGPotential.cpp
  HartreeFock.cpp
  HartreeFock.IO.cpp
  HFApp.cpp
)

IF(QT_FOUND) 
SET(qwtheaders
  NxyCurvePlot.h
  XmlTreeBuilder.h
  SQDFrame.h
  )
                                                                                                      
SET(qwtsources
  NxyCurvePlot.cpp
  XmlTreeBuilder.cpp
  SQDFrame.cpp
)

QT_WRAP_CPP(sqd_MOC_SRCS qwtsources qwtheaders)

LINK_LIBRARIES(${FORTRAN_LIBS})
ADD_EXECUTABLE(sqd ${HFSRCS} ${qwtsources})
TARGET_LINK_LIBRARIES(sqd qmcbase qmcutil)
ELSE(QT_FOUND)
SET(HFSRCS ${HFSRCS} SQDFrame.cpp)
ADD_EXECUTABLE(sqd ${HFSRCS})
TARGET_LINK_LIBRARIES(sqd qmcbase qmcutil)
ENDIF(QT_FOUND)
