#!/bin/bash

set -xe

pushd $PDC_SOURCE_DIR/build
cmake ../ -DCMAKE_INSTALL_PREFIX=$PDC_INSTALL_DIR \
          -DCMAKE_BUILD_TYPE=release \
          -DENABLE_MULTITHREAD=OFF \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DBUILD_MPI_TESTING=ON \
          -DENABLE_MPI=ON \
          -DBUILD_SHARED_LIBS=ON \
          -DPDC_SERVER_CACHE=ON \
          -DPDC_ENABLE_ZFP=ON \
          -DBUILD_TESTING=ON \
          -DPDC_ENABLE_MPI=ON \
          -DCMAKE_C_COMPILER=mpicc 
make install
popd