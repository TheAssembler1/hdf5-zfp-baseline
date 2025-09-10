#!/bin/bash

set -e
set -x
set -u

LAUNCHER="srun"

echo "Using $LAUNCHER with $NUM_NODES nodes"

pushd ./build

echo "Removing old output.csv"
rm output.csv || true
echo "Removing old pdc_server.log and pdc_client.log"
rm pdc_server_*.log || true
rm pdc_client_*.log || true

# Run the benchmark on 
for ((i=1; i<=$TOTAL_TASKS; i*=2)); do
	CLIENT_LOG=pdc_client_${i}.log
	SERVER_LOG=pdc_server_${i}.log

	echo "CLIENT_LOG=$CLIENT_LOG, SERVER_LOG=$SERVER_LOG"

	# Launch the pdc server(s)
	if [[ "$LAUNCHER" == "mpirun" ]]; then
		mpirun -np $NUM_NODES -n $NUM_NODES pdc_server > $SERVER_LOG 2>&1 &
	else
		echo "starting servers"
		srun --nodes=$NUM_NODES --ntasks-per-node=1 --error=pdc_server_${i}.err --output=pdc_server_${i}.log pdc_server &
		sleep 1
	fi

	echo "Running with $LAUNCHER and ${i} total tasks"

	if [[ "$LAUNCHER" == "mpirun" ]]; then 
    	mpirun -np ${i} ./zfp_baseline "/home/ta1/src/hdf5-zfp-baseline/workloads.json" > $CLIENT_LOG 2>&1
	else
		srun --ntasks=${i} \
			   --output=pdc_client_${i}.log \
			   --error=pdc_client_${i}.err \
			   ./zfp_baseline "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/transform_workloads.json"
	fi

	# Launch the pdc server(s)
	if [[ "$LAUNCHER" == "mpirun" ]]; then
		mpirun -np $NUM_NODES -n $NUM_NODES close_server > close_$SERVER_LOG 2>&1 &
	else
		echo "closing servers"
		srun --nodes=$NUM_NODES --ntasks-per-node=1 --error=close_pdc_server_${i}.err --output=close_pdc_server_${i}.log close_server &
		sleep 1
	fi
done

popd
