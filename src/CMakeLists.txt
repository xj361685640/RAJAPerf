###############################################################################
# Copyright (c) 2017, Lawrence Livermore National Security, LLC.
#
# Produced at the Lawrence Livermore National Laboratory
#
# LLNL-CODE-738930
#
# All rights reserved.
#
# This file is part of the RAJA Performance Suite.
#
# For details about use and distribution, please read raja-perfsuite/LICENSE.
#
###############################################################################

include_directories(.)

add_subdirectory(common)
add_subdirectory(apps)
add_subdirectory(basic)
add_subdirectory(lcals)
add_subdirectory(polybench)
add_subdirectory(stream)

set(RAJA_PERFSUITE_EXECUTABLE_DEPENDS
    common
    apps
    basic
    lcals
    polybench
    stream)
list(APPEND RAJA_PERFSUITE_EXECUTABLE_DEPENDS ${RAJA_PERFSUITE_DEPENDS}) 

blt_add_executable(
  NAME raja-perf.exe
  SOURCES RAJAPerfSuiteDriver.cpp
  DEPENDS_ON ${RAJA_PERFSUITE_EXECUTABLE_DEPENDS}
  )


blt_add_executable(
  NAME raja-perf-nolibs.exe
  SOURCES RAJAPerfSuiteDriver.cpp
  apps/DEL_DOT_VEC_2D.cpp
  apps/ENERGY.cpp
  apps/FIR.cpp
  apps/PRESSURE.cpp
  apps/VOL3D.cpp
  apps/WIP-COUPLE.cpp
  apps/LTIMES.cpp
  basic/IF_QUAD.cpp
  basic/INIT3.cpp
  basic/MULADDSUB.cpp
  basic/NESTED_INIT.cpp
  basic/REDUCE3_INT.cpp
  basic/TRAP_INT.cpp
  basic/INIT_VIEW1D.cpp
  basic/INIT_VIEW1D_OFFSET.cpp
  common/DataUtils.cpp
  common/Executor.cpp
  common/KernelBase.cpp
  common/OutputUtils.cpp
  common/RAJAPerfSuite.cpp
  common/RPTypes.hpp
  common/RunParams.cpp
  lcals/DIFF_PREDICT.cpp
  lcals/EOS.cpp
  lcals/FIRST_DIFF.cpp
  lcals/HYDRO_1D.cpp
  lcals/INT_PREDICT.cpp
  lcals/PLANCKIAN.cpp
  polybench/POLYBENCH_2MM.cpp
  polybench/POLYBENCH_3MM.cpp
  polybench/POLYBENCH_GEMMVER.cpp
  stream/ADD.cpp
  stream/COPY.cpp
  stream/DOT.cpp
  stream/MUL.cpp
  stream/TRIAD.cpp
  DEPENDS_ON ${RAJA_PERFSUITE_DEPENDS}
)
