#!/bin/bash

set -e
set -x
set -u

./rebuild.sh

LAUNCHER="mpirun"

pushd ./build

echo "Removing old output.csv"
rm output.csv || true

# Launch the pdc server(s)
pkill pdc_server || true 
if [[ "$LAUNCHER" == "mpirun" ]]; then
	mpirun -np 1 pdc_server > pdc_server.log 2>&1 &
else
	srun -n 1 pdc_server > pdc_server.log 2>&1 &
fi
sleep 1

# Run the benchmark
./zfp_baseline "/home/ta1/src/hdf5-zfp-baseline/workloads.json"

# Stop the pdc server(s)
echo "Stopping PDC server(s)"
pkill pdc_server || true 

popd
