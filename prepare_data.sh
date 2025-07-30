#!/bin/bash

set -x
set -u

pushd ./build
# HDF5 OUTPUT
awk -F, 'NR==1 || ($1=="rank" && $4=="write_chunk" && $6=="hdf5")' output.csv > hdf5_rank_write_chunk.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="write_all_chunks" && $6=="hdf5")' output.csv > hdf5_rank_write_all_chunks.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="read_chunk" && $6=="hdf5")' output.csv > hdf5_rank_read_chunk.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="read_all_chunks" && $6=="hdf5")' output.csv > hdf5_rank_read_all_chunks.csv

awk -F, 'NR==1 || ($1=="chunk" && $4=="write_chunk" && $6=="hdf5")' output.csv > hdf5_chunk_write_chunk.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="write_all_chunks" && $6=="hdf5")' output.csv > hdf5_chunk_write_all_chunks.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="read_chunk" && $6=="hdf5")' output.csv > hdf5_chunk_read_chunk.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="read_all_chunks" && $6=="hdf5")' output.csv > hdf5_chunk_read_all_chunks.csv

# PDC OUTPUT
awk -F, 'NR==1 || ($1=="rank" && $4=="write_chunk" && $6=="pdc")' output.csv > pdc_rank_write_chunk.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="write_all_chunks" && $6=="pdc")' output.csv > pdc_rank_write_all_chunks.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="read_chunk" && $6=="pdc")' output.csv > pdc_rank_read_chunk.csv
awk -F, 'NR==1 || ($1=="rank" && $4=="read_all_chunks" && $6=="pdc")' output.csv > pdc_rank_read_all_chunks.csv

awk -F, 'NR==1 || ($1=="chunk" && $4=="write_chunk" && $6=="pdc")' output.csv > pdc_chunk_write_chunk.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="write_all_chunks" && $6=="pdc")' output.csv > pdc_chunk_write_all_chunks.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="read_chunk" && $6=="pdc")' output.csv > pdc_chunk_read_chunk.csv
awk -F, 'NR==1 || ($1=="chunk" && $4=="read_all_chunks" && $6=="pdc")' output.csv > pdc_chunk_read_all_chunks.csv
popd
