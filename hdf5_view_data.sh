#!/bin/bash

pushd build

stat output.h5
h5dump -H output.h5
