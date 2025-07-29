#!/bin/bash

set -xe

export HG_HOST=wlp0s20f3

pushd build
pkill pdc_server || true
pdc_server
popd
