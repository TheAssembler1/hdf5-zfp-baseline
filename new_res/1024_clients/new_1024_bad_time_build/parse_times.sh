#!/bin/bash

# File to read
INPUT_FILE="pdc_server_8_pdc_zfp.log"

function calc_stats() {
	# Extract only "open time_elapsed" lines and extract the time values
	times=($(grep "$1 time_elapsed" "$2" | awk '{print $(NF-1)}'))

	count=${#times[@]}

	if [ $count -eq 0 ]; then
	    echo "No '$1 time_elapsed' entries found."
	    exit 1
	fi

	# Convert strings to floats
	sum=0
	min=${times[0]}
	max=${times[0]}

	for t in "${times[@]}"; do
	    sum=$(awk "BEGIN {print $sum + $t}")
	    (( $(awk "BEGIN {print ($t < $min)}") )) && min=$t
	    (( $(awk "BEGIN {print ($t > $max)}") )) && max=$t
	done

	# Calculate average
	avg=$(awk "BEGIN {print $sum / $count}")

	# Calculate standard deviation
	sum_sq_diff=0
	for t in "${times[@]}"; do
	    diff=$(awk "BEGIN {print $t - $avg}")
	    sq_diff=$(awk "BEGIN {print $diff * $diff}")
	    sum_sq_diff=$(awk "BEGIN {print $sum_sq_diff + $sq_diff}")
	done

	stdev=$(awk "BEGIN {print sqrt($sum_sq_diff / $count)}")

	# Output results
	echo -e "\t$1:"
	echo -e "\t\tCount: $count"
	echo -e "\t\tAverage: $avg s"
	echo -e "\t\tMinimum: $min s"
	echo -e "\t\tMaximum: $max s"
	echo -e "\t\tStandard Deviation $1: $stdev s"
}

calc_all_stats() {
	calc_stats "open" $1
	calc_stats "close" $1
	calc_stats "read" $1
	calc_stats "write" $1
	calc_stats "zfp_compress" $1
	calc_stats "zfp_decompress" $1
	calc_stats "PDCtf_exec_graph" $1
}

# Loop through powers of 2: 1, 2, 4, ..., 1024
for ((n=0; n<=10; n++)); do
    nodes=$((2**n))
    echo "${nodes} NODE(S) from pdc_server_${nodes}_pdc_zfp.log"
    
    # Replace with actual logic to get the correct log file per node count
    calc_all_stats "pdc_server_${nodes}_pdc_zfp.log"
done
