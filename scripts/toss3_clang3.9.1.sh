#!/bin/bash

##
## Copyright (c) 2017, Lawrence Livermore National Security, LLC.
##
## Produced at the Lawrence Livermore National Laboratory.
##
## LLNL-CODE-738930
##
## All rights reserved.
## 
## This file is part of the RAJA Performance Suite.
##
## For details about use and distribution, please read raja-perfsuite/LICENSE.
##

rm -rf build_toss3-clang-3.9.1 2>/dev/null
mkdir build_toss3-clang-3.9.1 && cd build_toss3-clang-3.9.1

module load cmake/3.5.2

PERFSUITE_DIR=$(git rev-parse --show-toplevel)

cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -C ${PERFSUITE_DIR}/host-configs/toss3/clang_3_9_1.cmake \
  -DENABLE_OPENMP=On \
  -DPERFSUITE_ENABLE_WARNINGS=Off \
  -DENABLE_ALL_WARNINGS=Off \
  -DCMAKE_INSTALL_PREFIX=../install_toss3-clang-3.9.1 \
  "$@" \
  ${PERFSUITE_DIR}
