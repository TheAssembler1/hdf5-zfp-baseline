#!/bin/bash

set -x
set -u

pushd ./build
awk -F, 'NR==1 || ($1=="rank" && $4=="h5_write_chunk")' output.csv > rank_write_chunk.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="h5_write_all_chunks")' output.csv > rank_write_all_chunks.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="h5_read_chunk")' output.csv > rank_read_chunk.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="h5_read_all_chunks")' output.csv > rank_read_all_chunks.csv

awk -F, 'NR==1 || ($1=="chunk" && $4=="h5_write_chunk")' output.csv > chunk_write_chunk.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="h5_write_all_chunks")' output.csv > chunk_write_all_chunks.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="h5_read_chunk")' output.csv > chunk_read_chunk.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="h5_read_all_chunks")' output.csv > chunk_read_all_chunks.csv
popd
