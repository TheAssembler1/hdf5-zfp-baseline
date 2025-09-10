#!/bin/bash
#SBATCH --nodes=32
#SBATCH --ntasks-per-node=33
#SBATCH --cpus-per-task=1
#SBATCH --account=m2621
#SBATCH --time=01:00:00
#SBATCH --constraint=cpu
#SBATCH --qos=regular

set -x
set -u

export NUM_NODES=32
export TASKS_PER_NODE=32
export TOTAL_TASKS=$((NUM_NODES * TASKS_PER_NODE))

echo "NUM_NODES = $NUM_NODES"
echo "TASKS_PER_NODE = $TASKS_PER_NODE"
echo "TOTAL_TASKS = $TOTAL_TASKS"

echo "going to hdf5-zfp-baseline"
cd /pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline
echo "sourcing env"
source /pscratch/sd/n/nlewi26/src/work_space/pdc_env.sh
source /pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/perlmutter_env.sh

echo "launching run"
./run.sh
