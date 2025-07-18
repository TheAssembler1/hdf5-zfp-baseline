#!/bin/bash

set -x
set -u

pushd ./build
rm -rf *
cmake ..
make
popd
