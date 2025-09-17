#!/bin/bash

INPUT_DIR="."  # Change to log directory if needed

function calc_stats() {
    local timer_name="$1"
    local file="$2"

    awk -v key="$timer_name" '
    $0 ~ key " time_elapsed" {
        val = $(NF-1)
        count++
        sum += val
        if (min == "" || val < min) min = val
        if (max == "" || val > max) max = val
        vals[count] = val
    }
    END {
        if (count == 0) {
            printf "\t%s: No entries found.\n", key
        } else {
            avg = sum / count
            for (i = 1; i <= count; i++) {
                diff = vals[i] - avg
                sumsq += diff * diff
            }
            stddev = sqrt(sumsq / count)
            printf "\t%s:\n", key
            printf "\t\tCount: %d\n", count
            printf "\t\tAverage: %.6f s\n", avg
            printf "\t\tMinimum: %.6f s\n", min
            printf "\t\tMaximum: %.6f s\n", max
            printf "\t\tStandard Deviation: %.6f s\n", stddev
        }
    }' "$file"
}


# Optimized calc_stats: Use awk once to compute everything
function t_calc_stats() {
    local timer_name="$1"
    local file="$2"

    awk -v key="$timer_name" '
    $0 ~ key " time_elapsed" {
        val = $(NF-1)
        count++
        sum += val
        if (min == "" || val < min) min = val
        if (max == "" || val > max) max = val
        vals[count] = val
    }
    END {
        if (count == 0) {
            printf "\t%s: No entries found.\n", key
            next
        }
        avg = sum / count
        for (i = 1; i <= count; i++) {
            diff = vals[i] - avg
            sumsq += diff * diff
        }
        stddev = sqrt(sumsq / count)
        printf "\t%s:\n", key
        printf "\t\tCount: %d\n", count
        printf "\t\tAverage: %.6f s\n", avg
        printf "\t\tMinimum: %.6f s\n", min
        printf "\t\tMaximum: %.6f s\n", max
        printf "\t\tStandard Deviation: %.6f s\n", stddev
    }' "$file"
}

# Calculate all timer types
function calc_all_stats() {
    local file="$1"
    for timer in open close read write zfp_compress zfp_decompress PDCtf_exec_graph; do
        calc_stats "$timer" "$file"
    done
}

# Loop through powers of 2: 1, 2, 4, ..., 1024
for ((n=0; n<=10; n++)); do
    nodes=$((2**n))
    logfile="$INPUT_DIR/pdc_server_${nodes}_pdc_zfp.log"

    if [[ -f "$logfile" ]]; then
        echo -e "\n${nodes} NODE(S) from $logfile"
        calc_all_stats "$logfile"
    else
        echo -e "\n${nodes} NODE(S): Log file '$logfile' not found."
    fi
done

