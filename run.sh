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

# scale by rank static chunk
pushd ./build

# remove past files
rm *.csv || true
rm  *.png || true

# rm past folders
rm -rf independent || true
rm -rf collective || true 

# recreate folders
mkdir independent
mkdir collective

echo "=== Scaling by rank with fixed chunk count ==="
for ((rank=1; rank<=STATIC_RANK; rank*=2)); do
    echo "Running with ranks=$rank, chunks=$STATIC_CHUNK"
    mpirun --mca btl_tcp_if_include lo --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $rank ./zfp_baseline $DONT_COLLECTIVE_IO $STATIC_CHUNK $SCALE_BY_RANK
done

echo "=== Scaling by chunk count with fixed rank count ==="
for ((chunks=1; chunks<=STATIC_CHUNK; chunks*=2)); do
    echo "Running with ranks=$STATIC_RANK, chunks=$chunks"
    mpirun --mca btl_tcp_if_include lo  --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $STATIC_RANK ./zfp_baseline $DONT_COLLECTIVE_IO $chunks $DONT_SCALE_BY_RANK
done

popd 
./plot.sh 
pushd ./build

mv *.csv independent
mv *.png independent

echo "=== Scaling by rank with fixed chunk count ==="
for ((rank=1; rank<=STATIC_RANK; rank*=2)); do
    echo "Running with ranks=$rank, chunks=$STATIC_CHUNK"
    mpirun --mca btl_tcp_if_include lo --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $rank ./zfp_baseline $COLLECTIVE_IO $STATIC_CHUNK $SCALE_BY_RANK
done

echo "=== Scaling by chunk count with fixed rank count ==="
for ((chunks=1; chunks<=STATIC_CHUNK; chunks*=2)); do
    echo "Running with ranks=$STATIC_RANK, chunks=$chunks"
    mpirun --mca btl_tcp_if_include lo  --use-hwthread-cpus --map-by :OVERSUBSCRIBE -np $STATIC_RANK ./zfp_baseline $COLLECTIVE_IO $chunks $DONT_SCALE_BY_RANK
done

popd
./plot.sh 
pushd ./build

mv *.csv collective
mv *.png collective