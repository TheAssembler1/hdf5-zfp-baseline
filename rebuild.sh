#!/bin/bash

set -x
set -u

pushd ./build
cmake ..
make
popd
