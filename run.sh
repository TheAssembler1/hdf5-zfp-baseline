#!/bin/bash

set -e
set -x
set -u

LAUNCHER="srun"

echo "Using $LAUNCHER with $NUM_NODES nodes"

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
function run_benchmark() {
	# Run the benchmark on 
	for ((i=1; i<=$TOTAL_TASKS; i*=2)); do
		# Setup names of log files
		CLIENT_LOG=pdc_client_${i}_$2.log
		CLIENT_CLOSE_LOG=pdc_client_close_${i}_$2.log
		SERVER_LOG=pdc_server_${i}_$2.log
		CLIENT_LOG_ERR=pdc_client_${i}_$2.err
		CLIENT_CLOSE_LOG_ERR=pdc_client_close_${i}_$2.err
		SERVER_LOG_ERR=pdc_server_${i}_$2.err

		# Print log info
		echo "CLIENT_LOG=$CLIENT_LOG, SERVER_LOG=$SERVER_LOG, CLIENT_CLOSE_LOG=$CLIENT_CLOSE_LOG"
		echo "CLIENT_LOG_ERR=$CLIENT_LOG_ERR, SERVER_LOG_ERR=$SERVER_LOG_ERR, CLIENT_CLOSE_LOG_ERR=$CLIENT_CLOSE_LOG_ERR"

		echo "Running benchmark $1 with ${i} ranks"
		# Launch the pdc server(s)
		echo "Starting servers"
		srun --nodes=$NUM_NODES \
			 --ntasks-per-node=1 \
			 --error="$SERVER_LOG_ERR" \
			 --output="$SERVER_LOG" \
			 pdc_server &
		echo "Sleeping after server start"
		sleep 1

		echo "Starting benchmark"
		srun --ntasks=${i} \
			--output="$CLIENT_LOG" \
			--error="$CLIENT_LOG_ERR" \
			./zfp_baseline $1

		# Launch the pdc server(s)
		echo "Closing servers"
		srun --nodes=$NUM_NODES \
			 --ntasks-per-node=1 \
			 --error="$CLIENT_CLOSE_LOG_ERR"\
			--output="$CLIENT_CLOSE_LOG" \
			close_server
	done

	clean_between_workload_files
}

clean_old_files
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_raw.json" "pdc_raw"
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/hdf5_raw.json" "hdf5_raw"
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/pdc_zfp.json" "pdc_zfp"
run_benchmark "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/workloads/hdf5_zfp.json" "hdf5_zfp"