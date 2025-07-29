#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

#include "common.h"

const char* timer_tags[] = {
    "write_chunk",
    "write_all_chunks",
    "read_chunk",
    "read_all_chunks"
};
double timer_start_times[TIMER_TAGS_COUNT];
double timer_accumulated[TIMER_TAGS_COUNT] = {0};

void print_all_timers_csv(const char *filename, int chunks_per_rank, int num_ranks, bool scale_by_rank) {
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
            fprintf(stderr, "Error: could not open file %s for appending\n", filename);
            return;
        }

        // If file did not exist, print header
        if (!file_exists) {
            fprintf(fp, "scale_type,chunks_per_rank,num_ranks,timer_tag,elapsed_seconds\n");
        }

        for (int i = 0; i < TIMER_TAGS_COUNT; i++) {
            char* scale_type;
            if(scale_by_rank)
                scale_type = "rank";
            else 
                scale_type = "chunk";

            if(i == WRITE_ALL_CHUNKS || i == READ_ALL_CHUNKS)
                fprintf(fp, "%s,%d,%d,%s,%f\n", scale_type, chunks_per_rank, num_ranks, timer_tags[i], timer_accumulated[i]);
            else 
                fprintf(fp, "%s,%d,%d,%s,%f\n", scale_type, chunks_per_rank, num_ranks, timer_tags[i], timer_accumulated[i] / chunks_per_rank);
        }

        fclose(fp);
        printf("Timer results appended to %s\n", filename);
    }
}