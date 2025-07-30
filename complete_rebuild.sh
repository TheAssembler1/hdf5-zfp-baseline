#!/bin/bash

set -x
set -u

rm -rf ./build || true
mkdir build

pushd ./build
cmake ..
make
popd
