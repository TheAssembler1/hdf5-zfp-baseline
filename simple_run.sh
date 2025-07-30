#!/bin/bash

set -e
set -x
set -u

./rebuild.sh

NUM_NODES=1

STATIC_CHUNK=1
STATIC_RANK=1

SCALE_BY_RANK=1
DONT_SCALE_BY_RANK=0

COLLECTIVE_IO=1
DONT_COLLECTIVE_IO=0

ZFP_FILTER=1
DONT_ZFP_FILTER=0

HDF5_IMPL=0
PDC_IMPL=1
IO_IMPL=$HDF5_IMPL

pushd ./build
if [[ $IO_IMPL -eq $PDC_IMPL ]]; then
    mpirun -np $STATIC_RANK close_server || true
    pkill pdc_server || true 
    mpirun -np $STATIC_RANK pdc_server > pdc_server.log 2>&1 &
fi

srun -N $NUM_NODES -n $STATIC_RANK ./zfp_baseline $DONT_COLLECTIVE_IO $STATIC_CHUNK $SCALE_BY_RANK $ZFP_FILTER $HDF5_IMPL

if [[ $IO_IMPL -eq $PDC_IMPL ]]; then
    mpirun -np $STATIC_RANK close_server || true
    pkill pdc_server || true 
fi
popd
