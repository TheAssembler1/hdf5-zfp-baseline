#!/bin/bash

#SBATCH --nodes=256
#SBATCH --ntasks-per-node=128
#SBATCH --cpus-per-task=1
#SBATCH --account=m2621
#SBATCH --time=01:00:00
#SBATCH --constraint=cpu
#SBATCH --qos=regular

set -x
set -u

export TOTAL_NODES=256
export TASKS_PER_NODE=32
export TOTAL_TASKS=$(($TOTAL_NODES * $TASKS_PER_NODE))

echo "TOTAL_NODES = $TOTAL_NODES"
echo "TASKS_PER_NODE = $TASKS_PER_NODE"
echo "TOTAL_TASKS = $TOTAL_TASKS"

echo "going to hdf5-zfp-baseline"
cd /pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline
echo "sourcing env"
source /pscratch/sd/n/nlewi26/src/work_space/pdc_env.sh
source /pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/perlmutter_env.sh

echo "launching run"
./run.sh
