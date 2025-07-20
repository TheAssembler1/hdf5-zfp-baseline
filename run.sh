#!/bin/bash

set -e
set -x
set -u

./rebuild.sh

STATIC_CHUNK=32
STATIC_RANK=16

SCALE_BY_RANK=1
DONT_SCALE_BY_RANK=0

COLLECTIVE_IO=1
DONT_COLLECTIVE_IO=0

ZFP_FILTER=1
DONT_ZFP_FILTER=0

required_vars=(
  CC
  CMAKE_PREFIX_PATH
  HDF5_INSTALL_DIR
  ZFP_INSTALL_DIR
  ZFP_FILTER_INSTALL_DIR
  HDF5_PLUGIN_PATH
)

for var in "${required_vars[@]}"; do
  if [ -z "${!var-}" ]; then
    echo "Error: environment variable '$var' is not set or empty." >&2
    exit 1
  fi
done

echo "Using STATIC_CHUNK = $STATIC_CHUNK"
echo "Using STATIC_RANK = $STATIC_RANK"


function init() {
    # remove past files
    rm *.csv || true
    rm *.png || true

    # rm past folders
    rm -rf independent || true
    rm -rf collective || true 

    # recreate folders
    mkdir independent
    mkdir collective

    # recreate zfp folders
    mkdir independent/zfp
    mkdir independent/raw

    mkdir collective/zfp
    mkdir collective/raw
}


function store_csv_png() {
    local is_zfp=$1
    local is_collective=$2

    popd
    ./plot.sh
    pushd ./build

    local target_dir
    if [[ $is_collective -eq 1 ]]; then
      target_dir="collective"
    else
      target_dir="independent"
    fi

    if [[ $is_zfp -eq 1 ]]; then
      target_dir="$target_dir/zfp"
    else
      target_dir="$target_dir/raw"
    fi

    mv *.csv "$target_dir"/
    mv *.png "$target_dir"/
}

function run_scaling_rank() {
  local max_rank=$1
  local chunk_count=$2
  local collective_io_flag=$3
  local scale_by_rank_flag=$4
  local zfp_filter_flag=$5

  echo "=== Scaling by rank with fixed chunk count ==="
  for (( rank=1; rank<=max_rank; rank*=2 )); do
    echo "Running with ranks=$rank, chunks=$chunk_count"
    mpirun --mca btl_tcp_if_include lo --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $rank ./zfp_baseline \
      "$collective_io_flag" "$chunk_count" "$scale_by_rank_flag" "$zfp_filter_flag"
  done
}

function run_chunk_scaling() {
  local fixed_rank=$1
  local max_chunk=$2
  local collective_io_flag=$3
  local scale_by_rank_flag=$4
  local zfp_filter_flag=$5

  echo "=== Scaling by chunk count with fixed rank count ==="
  for (( chunk=1; chunk<=max_chunk; chunk*=2 )); do
    echo "Running with ranks=$fixed_rank, chunks=$chunk"
    mpirun --mca btl_tcp_if_include lo --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $fixed_rank ./zfp_baseline \
      "$collective_io_flag" "$chunk" "$scale_by_rank_flag" "$zfp_filter_flag"
  done
}

pushd ./build

init

# NO FILTER & INDEPENDENT
#run_scaling_rank $STATIC_RANK $STATIC_CHUNK $DONT_COLLECTIVE_IO $SCALE_BY_RANK $DONT_ZFP_FILTER
#run_chunk_scaling $STATIC_RANK $STATIC_CHUNK $DONT_COLLECTIVE_IO $DONT_SCALE_BY_RANK $DONT_ZFP_FILTER
#store_csv_png $DONT_ZFP_FILTER $DONT_COLLECTIVE_IO 

# NO FILTER & COLLECTIVE
run_scaling_rank $STATIC_RANK $STATIC_CHUNK $COLLECTIVE_IO $SCALE_BY_RANK $DONT_ZFP_FILTER
run_chunk_scaling $STATIC_RANK $STATIC_CHUNK $COLLECTIVE_IO $DONT_SCALE_BY_RANK $DONT_ZFP_FILTER

store_csv_png $DONT_ZFP_FILTER $COLLECTIVE_IO 

# FILTER & INDEPENDENT
#run_scaling_rank $STATIC_RANK $STATIC_CHUNK $DONT_COLLECTIVE_IO $SCALE_BY_RANK $ZFP_FILTER
#run_chunk_scaling $STATIC_RANK $STATIC_CHUNK $DONT_COLLECTIVE_IO $DONT_SCALE_BY_RANK $ZFP_FILTER
#store_csv_png $ZFP_FILTER $DONT_COLLECTIVE_IO 

# FILTER & COLLECTIVE
run_scaling_rank $STATIC_RANK $STATIC_CHUNK $COLLECTIVE_IO $SCALE_BY_RANK $ZFP_FILTER
run_chunk_scaling $STATIC_RANK $STATIC_CHUNK $COLLECTIVE_IO $DONT_SCALE_BY_RANK $ZFP_FILTER

store_csv_png $ZFP_FILTER $COLLECTIVE_IO 

popd
