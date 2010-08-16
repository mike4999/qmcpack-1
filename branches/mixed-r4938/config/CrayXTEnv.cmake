# common variables for Cray XT
SET(QMC_BITS 64)
ADD_DEFINITIONS(-DADD_ -DINLINE_ALL=inline -D_CRAYMPI)

IF($ENV{PE_ENV} MATCHES "GNU")
  SET(HAVE_SSE 1)
  SET(HAVE_SSE2 1)
  SET(HAVE_SSE3 1)
  SET(HAVE_SSSE3 1)
  SET(USE_PREFETCH 1)
  SET(PREFETCH_AHEAD 12)
  #openmp is enabled
  SET(CMAKE_TRY_OPENMP_CXX_FLAGS "-fopenmp")
  CHECK_CXX_ACCEPTS_FLAG(${CMAKE_TRY_OPENMP_CXX_FLAGS} GNU_OPENMP_FLAGS)
  IF(GNU_OPENMP_FLAGS)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_TRY_OPENMP_CXX_FLAGS}")
    SET(ENABLE_OPENMP 1)
  ENDIF(GNU_OPENMP_FLAGS)
ELSE($ENV{PE_ENV} MATCHES "GNU")
  ADD_DEFINITIONS(-DPGI)
ENDIF($ENV{PE_ENV} MATCHES "GNU")

SET(BLAS_LIBRARY -lacml)
SET(LAPACK_LIBRARY -lacml_mv)
SET(HAVE_MPI 1)
SET(HAVE_OOMPI 1)
#SET(HAVE_ACML 1)

MARK_AS_ADVANCED(
  LAPACK_LIBRARY 
  BLAS_LIBRARY 
)
