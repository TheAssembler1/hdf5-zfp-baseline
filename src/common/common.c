#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

#include "common.h"
#include "log.h"

char* io_filter_strings[] = {"raw", "zfp_compress"};
const char *io_participation_strings[] = {"collective", "indepdendent"};
char *io_impl_strings[] = {"hdf5", "hdf5_zfp", "pdc", "pdc_zfp"};
const char *timer_tags[] = {"write_chunk", "write_all_chunks", "read_chunk",
                            "read_all_chunks"};

double timer_start_times[TIMER_TAGS_COUNT];
double timer_accumulated[TIMER_TAGS_COUNT] = {0};

void print_all_timers_csv(const char *filename, int chunks_per_rank,
                          int num_ranks, char *workload_name, 
                          uint64_t chunk_size_bytes, 
                          io_participation_t io_participation,
                          io_filter_t io_filter) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // Check if file exists
        FILE *fp_check = fopen(filename, "r");
        int file_exists = (fp_check != NULL);
        if (fp_check) fclose(fp_check);

        // Open file in append mode
        FILE *fp = fopen(filename, "a");
        if (!fp) {
            PRINT_ERROR("Could not open file %s for appending\n", filename);
            return;
        }

        // If file did not exist, print header
        if (!file_exists) {
            fprintf(fp, "[0]workload_name,[1]chunks_per_rank,[2]num_ranks,[3]timer_tag,"
                        "[4]elapsed_seconds,[5]chunk_size_bytes,[6]io_participation,[7]filter\n");
        }

        

        for (int i = 0; i < TIMER_TAGS_COUNT; i++) {
            double time;
            if (i == WRITE_ALL_CHUNKS || i == READ_ALL_CHUNKS)
                time = timer_accumulated[i];
            else 
                time = timer_accumulated[i] / chunks_per_rank;

            fprintf(fp, "%s,%d,%d,%s,%f,%lu,%s,%s\n", workload_name, chunks_per_rank,
                    num_ranks, timer_tags[i],
                    time, chunk_size_bytes,
                    io_participation_strings[io_participation],
                    io_filter_strings[io_filter]);
        }

        fclose(fp);
        PRINT_RANK0("Timer results appended to %s\n", filename);
    }
}