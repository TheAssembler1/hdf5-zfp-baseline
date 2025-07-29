#!/bin/bash

set -xe

pkill pdc_server || true
pdc_server
