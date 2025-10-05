#!/bin/bash

set -exu

LAUNCHER="srun"

echo "Using $LAUNCHER with $TOTAL_NODES total nodes"

# Move into build dir
cd ./build

PDC_DATA_PATH=/pscratch/sd/n/nlewi26/pdc_data

# Remove last workloads files
# Need to be in build dir to work
function clean_old_files() {
	rm -f output.csv *.log *.err
}

function clean_between_workload_files() {
	rm -rf "${PDC_DATA_PATH:?}"/*
	rm -f output.h5
}

# 1 Argument the path to the workload to runf
# 2 Argument is just name of workloads for logging
# 3 Argument is true or false indicating whether PDC servers should be launched/closed
function run_benchmark() {
	# Run the benchmark on 
	for ((i=1; i<=$TOTAL_TASKS; i*=2)); do
		# Setup names of log files
		CLIENT_LOG=client_${i}_$2.log
		CLIENT_LOG_ERR=client_${i}_$2.err

		# PDC specific log files
		SERVER_LOG=server_${i}_$2.log
		SERVER_LOG_ERR=server_${i}_$2.err
		CLIENT_CLOSE_LOG=client_close_${i}_$2.log
		CLIENT_CLOSE_LOG_ERR=client_close_${i}_$2.err

		echo "CLIENT_LOG=$CLIENT_LOG, CLIENT_LOG_ERR=$CLIENT_LOG_ERR"

		NUM_NODES=$(( (i + 31) / 32 ))
		if [ $NUM_NODES -gt $TOTAL_NODES ]; then
			NUM_NODES=$TOTAL_NODES
		fi

		echo "Running benchmark $1 with ${i} ranks on $NUM_NODES nodes"

		# Launch the PDC server(s)
		if [ "$3" = "true" ]; then
			echo "CLIENT_CLOSE_LOG=$CLIENT_CLOSE_LOG, CLIENT_CLOSE_LOG_ERR=$CLIENT_CLOSE_LOG_ERR"
			echo "SERVER_LOG=$SERVER_LOG, SERVER_LOG_ERR=$SERVER_LOG_ERR"
			echo "Starting PDC servers"
			srun --nodes=$NUM_NODES \
				--ntasks-per-node=1 \
				--error="$SERVER_LOG_ERR" \
				--output="$SERVER_LOG" \
				pdc_server &
			echo "Sleeping after server start"
			sleep 1
		fi

		echo "Starting benchmark"
		srun --ntasks=${i} \
			--output="$CLIENT_LOG" \
			--error="$CLIENT_LOG_ERR" \
			./zfp_baseline $1

		# Launch the pdc server(s)
		if [ "$3" = "true" ]; then
			echo "Closing PDC servers"
			srun --nodes=$NUM_NODES \
				--ntasks-per-node=1 \
				--error="$CLIENT_CLOSE_LOG_ERR"\
				--output="$CLIENT_CLOSE_LOG" \
				close_server
			# Shows PDC data size
			du -h /pscratch/sd/n/nlewi26/pdc_data
		else 
			# Shows HDF5 data size
			/pscratch/sd/n/nlewi26/src/hdf5/install/bin/h5ls -v output.h5
		fi
	done

	clean_between_workload_files
}

clean_old_files
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_raw.json" "pdc_raw" true
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_zfp.json" "pdc_zfp" true
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/hdf5_raw.json" "hdf5_raw" false
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/hdf5_zfp.json" "hdf5_zfp" false
