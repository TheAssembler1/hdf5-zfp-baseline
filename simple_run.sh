#!/bin/bash

set -e
set -x
set -u

./rebuild.sh

STATIC_CHUNK=4
STATIC_RANK=4

SCALE_BY_RANK=1
DONT_SCALE_BY_RANK=0

COLLECTIVE_IO=1
DONT_COLLECTIVE_IO=0

ZFP_FILTER=1
DONT_ZFP_FILTER=0

export HDF5_PLUGIN_PATH=/home/ta1/src/H5Z-ZFP/install/plugin

pushd ./build
mpirun -np 1 ./zfp_baseline $DONT_COLLECTIVE_IO $STATIC_CHUNK $SCALE_BY_RANK $ZFP_FILTER
popd
