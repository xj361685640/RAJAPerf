#!/bin/bash

##
## Copyright (c) 2017, Lawrence Livermore National Security, LLC.
##
## Produced at the Lawrence Livermore National Laboratory.
##
## LLNL-CODE-xxxxxx
##
## All rights reserved.
##
## This file is part of the RAJA Performance Suite.
##
## For more information, see the file LICENSE in the top-level directory.
##

rm -rf build_blueos-clang-4.0.0 2>/dev/null
mkdir build_blueos-clang-4.0.0 && cd build_blueos-clang-4.0.0

module load cmake/3.7.2

PERFSUITE_DIR=$(git rev-parse --show-toplevel)

cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -C ${PERFSUITE_DIR}/host-configs/blueos/clang_4_0_0.cmake \
  -DENABLE_OPENMP=On \
  -DPERFSUITE_ENABLE_WARNINGS=Off \
  -DENABLE_ALL_WARNINGS=Off \
  -DCMAKE_INSTALL_PREFIX=../install_blueos-clang-4.0.0 \
  "$@" \
  ${PERFSUITE_DIR}
