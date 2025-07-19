#!/bin/bash

export CMAKE_PREFIX_PATH=/home/ta1/src/hdf5/install
export CC=mpicc

set -x
set -u

pushd ./build
cmake ..
make
popd
