#!/bin/bash

set -x
set -u

./prepare_data.sh

pushd ./build

cp ../plot_timers.gnu ./
gnuplot plot_timers.gnu

popd
