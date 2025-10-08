#!/bin/bash

set -exu

LAUNCHER="none"

pushd ./build

export PDC_TMPDIR=/pscratch/sd/n/nlewi26/pdc_tmp
export PDC_DATA_LOC=/pscratch/sd/n/nlewi26/pdc_data

pkill pdc_server || true
pdc_server  > pdc_write_server.log 2>&1 &
./zfp_baseline "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_raw_write.json"
close_server

find /pscratch/sd/n/nlewi26/pdc_data -type f -exec stat --printf="%s %n\n" {} \;

pkill pdc_server || true
pdc_server restart  > pdc_write_server.log 2>&1 &
./zfp_baseline "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_raw_read.json"
close_server

popd
