#!/bin/bash

set -exu

LAUNCHER="none"

pushd ./build

echo "Removing old output.csv"
rm output.csv || true
rm pdc_server.log || true
rm output.h5 || true
rm -r /pscratch/sd/n/nlewi26/pdc_data/* || true

# Launch the pdc server(s)
pkill pdc_server || true 
if [[ "$LAUNCHER" == "mpirun" ]]; then
	mpirun -np 1 pdc_server > pdc_server.log 2>&1 &
elif [[ "$LAUNCHER" == "none" ]]; then
	pdc_server > pdc_server.log 2>&1 &
else
	nohup srun -n 1 pdc_server > pdc_server.log 2>&1 &
fi
sleep 1

# Run the benchmark
./zfp_baseline "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_raw.json"

close_server

find /pscratch/sd/n/nlewi26/pdc_data -type f -exec stat --printf="%s %n\n" {} \;

popd
