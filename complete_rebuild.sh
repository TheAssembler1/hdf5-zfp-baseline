#!/bin/bash

set -x
set -u

export CMAKE_PREFIX_PATH=/home/ta1/src/hdf5/install
export CC=mpicc

rm -rf ./build || true
mkdir build

pushd ./build
cmake ..
make
popd
