PROJECT(newqmc)

SET(QMCAPPDIR 
    ParticleSetPool.cpp
    WaveFunctionPool.cpp
    HamiltonianPool.cpp
    InitMolecularSystem.cpp
    QMCAppBase.cpp
    QMCDriverFactory.cpp
    QMCMain.cpp
    QMCInterface.cpp
)

####################################
# create libqmc 
####################################
ADD_LIBRARY(qmc ${QMCAPPDIR})
#IF(QMC_BUILD_STATIC)
#  ADD_LIBRARY(qmc STATIC ${QMCAPPDIR})
#ELSE(QMC_BUILD_STATIC)
#  ADD_LIBRARY(qmc SHARED ${QMCAPPDIR})
#ENDIF(QMC_BUILD_STATIC)

####################################
#sources for new qmcapp, general QMC code to handle multiple
#configurations, hamiltonians and wavefunctions.
####################################
SET(QMCMAIN 
  qmcapp.cpp
)

####################################
# sources for mo2grid
####################################
#SET(MOSRCS 
#  ${QMCDRIVERS}
#  ParticleSetPool.cpp
#  QMCAppBase.cpp
#  MO2Grid3D.cpp 
#  mo2grid3d_main.cpp
#)

####################################
# link lapack/blas and fortran library 
####################################
#LINK_LIBRARIES(
#  ${LAPACK_LIBRARY} 
#  ${BLAS_LIBRARY} 
#  ${FORTRAN_LIBS} 
#)

####################################
# adding thread library: e.g., -lguide -lpthread
####################################
IF(INTEL_MKL)
  LINK_LIBRARIES(${INTEL_GUIDE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
ELSE(INTEL_MKL)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS_INIT})
ENDIF(INTEL_MKL)


####################################
# build mo2grid3d executable
####################################
#IF(NOT HAVE_MPI)
#ADD_EXECUTABLE(mo2grid3d ${MOSRCS})
#TARGET_LINK_LIBRARIES(mo2grid3d qmc qmcbase)
#ENDIF(NOT HAVE_MPI)

####################################
#general qmcapp for multiple configurations, hamiltonians and wavefunctions.
####################################
ADD_EXECUTABLE(qmcapp ${QMCMAIN})

IF(MAC_VECLIB)
  TARGET_LINK_LIBRARIES(qmcapp qmc qmcdriver qmcham qmcwfs qmcbase qmcutil)
ELSE(MAC_VECLIB)
  TARGET_LINK_LIBRARIES(qmcapp qmc qmcdriver qmcham qmcwfs qmcbase qmcutil 
  ${LAPACK_LIBRARY}
  ${BLAS_LIBRARY} 
  ${FORTRAN_LIBS})
ENDIF(MAC_VECLIB)

