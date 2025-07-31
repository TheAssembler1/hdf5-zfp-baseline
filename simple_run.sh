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
IO_IMPL=$PDC_IMPL

pushd ./build
if [[ $IO_IMPL -eq $PDC_IMPL ]]; then
    pkill pdc_server || true 
    pdc_server > pdc_server.log 2>&1 &
    sleep 4
fi

./zfp_baseline $DONT_COLLECTIVE_IO $STATIC_CHUNK $SCALE_BY_RANK $DONT_ZFP_FILTER $IO_IMPL || true

if [[ $IO_IMPL -eq $PDC_IMPL ]]; then
    srun -N $NUM_NODES -n $NUM_NODES close_server || true
    pkill pdc_server || true 
fi
popd
