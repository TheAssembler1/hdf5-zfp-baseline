#!/bin/bash

set -e
set -x
set -u

./rebuild.sh

STATIC_CHUNK=1
STATIC_RANK=8
SCALE_TO_CHUNK=4
SCALE_TO_RANK=8
NUM_NODES=1
LAUNCHER=mpirun

if [[ "$LAUNCHER" == "mpirun" ]]; then
  static_launcher="mpirun --mca btl_tcp_if_include lo --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $STATIC_RANK"
  launcher='mpirun --mca btl_tcp_if_include lo --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $rank'
else
  echo "Launcher is srun"
  static_launcher="srun -N $NUM_NODES --ntasks=$STATIC_RANK"
  launcher='srun -N $NUM_NODES --ntasks=$rank'
fi

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
  PDC_SOURCE_DIR
  PDC_INSTALL_DIR
  MERCURY_INSTALL_DIR
  HDF5_PLUGIN_PATH
  LD_LIBRARY_PATH
)

for var in "${required_vars[@]}"; do
  if [ -z "${!var-}" ]; then
    echo "Error: environment variable '$var' is not set or empty." >&2
    exit 1
  fi
done

echo "Using STATIC_CHUNK = $STATIC_CHUNK"
echo "Using STATIC_RANK = $STATIC_RANK"
echo "Using SCALE_TO_CHUNK = $SCALE_TO_CHUNK"
echo "Using SCALE_TO_RANK = $SCALE_TO_RANK"

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

    mv *.csv "$target_dir"/ || true
    mv *.png "$target_dir"/ || true
    mv *.log "$target_dir"/ || true
}

start_pdc_server() {
  echo "Starting $NUM_NODES PDC server(s)"
  pkill pdc_server || true 
  mpirun -np 1 pdc_server > pdc_server.log 2>&1 &

  sleep 4
}

stop_pdc_server() {
  echo "Closing $NUM_NODES PDC server(s)"
  pkill pdc_server || true 
  rm -rf ./pdc_tmp || true
}

function run_scaling_rank() {
  local max_rank=$1
  local chunk_count=$2
  local collective_io_flag=$3
  local scale_by_rank_flag=$4
  local zfp_filter_flag=$5
  local io_impl_flag=$6

  echo "=== Scaling by rank with fixed chunk count ==="
  for (( rank=1; rank<=max_rank; rank*=2 )); do
    echo "Running with ranks=$rank, chunks=$chunk_count"
    if [[ $io_impl_flag -eq 1 ]]; then
      start_pdc_server
    fi
    eval "$launcher ./zfp_baseline \"$collective_io_flag\" \"$STATIC_CHUNK\" \"$scale_by_rank_flag\" \"$zfp_filter_flag\" \"$io_impl_flag\" 2>&1 | tee client.log"
    if [[ $io_impl_flag -eq 1 ]]; then
      stop_pdc_server
    fi
    rm ./pdc_tmp || true
    rm output.h5 || true
  done
}

function run_chunk_scaling() {
  local fixed_rank=$1
  local max_chunk=$2
  local collective_io_flag=$3
  local scale_by_rank_flag=$4
  local zfp_filter_flag=$5
  local io_impl_flag=$6
  echo "=== Scaling by chunk count with fixed rank count ==="
  for (( chunk=1; chunk<=max_chunk; chunk*=2 )); do
    echo "Running with ranks=$STATIC_RANK, chunks=$chunk"
    if [[ $io_impl_flag -eq 1 ]]; then
      start_pdc_server
    fi
    $static_launcher  ./zfp_baseline \
      "$collective_io_flag" "$chunk" "$scale_by_rank_flag" "$zfp_filter_flag" "$io_impl_flag" \
    2>&1 | tee client.log
    if [[ $io_impl_flag -eq 1 ]]; then
      stop_pdc_server
    fi
    rm ./pdc_tmp || true
    rm output.h5 || true
  done
}

pushd ./build

init

for collective_io in 0 1; do
  for scale_by_rank in 0 1; do
    for zfp_filter in 0 1; do
      for io_impl in 0 1; do
        # Skip if independent IO and zfp filter enabled
        # Not supported by HDF5
        if [[ $collective_io -eq 0 && $zfp_filter -eq 1 ]]; then
          continue
        fi
        # Skip PDC Compression
        if [[ $zfp_filter -eq 1 && $io_impl -eq 1 ]]; then
          echo "enabling PDC compression"
          popd 
          ./pdc_enable_compression.sh
          pushd ./build
        elif [[ $zfp_filter -eq 0 && $io_impl -eq 1 ]]; then
          echo "disabling PDC compression"
          popd 
          ./pdc_disable_compression.sh
          pushd ./build
        fi
        run_scaling_rank $SCALE_TO_RANK $SCALE_TO_CHUNK $collective_io $scale_by_rank $zfp_filter $io_impl
        run_chunk_scaling $SCALE_TO_RANK $SCALE_TO_CHUNK $collective_io 0 $zfp_filter $io_impl
      done
      store_csv_png $zfp_filter $collective_io
    done
  done
done

popd
