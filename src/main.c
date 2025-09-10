/**
 * Each I/O library needs to implement the following functions:
 *     1. init: intiailizes the library
 */

#include <stdbool.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <string.h>

#include "common/common.h"
#include "common/log.h"
#include "common/config.h"
#include "hdf5_impl/hdf5_io_impl.h"
#include "pdc_impl/pdc_io_impl.h"
#include "exec_io_impl.h"

#define USAGE "./zfp_baseline <json_config_path>"

int main(int argc, char **argv) {  
    printf("Entered main\n");

    MPI_Init(&argc, &argv);

    int rank, num_ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    char *config_path = argv[1];
    config_t *config = init_config(config_path);

    io_impl_funcs_t io_impl_funcs[NUM_IO_IMPL] = {
        [HDF5_IMPL] = {.io_filter = IO_FILTER_RAW,
                       .init = hdf5_io_init,
                       .deinit = hdf5_io_deinit,
                       .init_dataset = hdf5_io_init_dataset,
                       .create_dataset = hdf5_io_create_dataset,
                       .enable_compression_on_dataset =
                           hdf5_io_enable_compression_on_dataset,
                       .write_chunk = hdf5_io_write_chunk,
                       .read_chunk = hdf5_io_read_chunk,
                       .flush = hdf5_io_flush,
                       .close_dataset = hdf5_io_close_dataset,
                       .reopen_dataset = hdf5_io_reopen_dataset},
        [PDC_IMPL] = {.io_filter = IO_FILTER_RAW,
                      .init = pdc_io_init,
                      .deinit = pdc_io_deinit,
                      .init_dataset = pdc_io_init_dataset,
                      .create_dataset = pdc_io_create_dataset,
                      .enable_compression_on_dataset =
                          pdc_io_enable_compression_on_dataset,
                      .write_chunk = pdc_io_write_chunk,
                      .read_chunk = pdc_io_read_chunk,
                      .flush = pdc_io_flush,
                      .close_dataset = pdc_io_close_dataset,
                      .reopen_dataset = pdc_io_reopen_dataset}};

    for (uint32_t i = 0; i < config->num_workloads; i++) {
        io_impl_t cur_io_impl = -1;
        for (int j = 0; j < NUM_IO_IMPL; j++) {
            if (strcmp(config->workloads[i].implementation,
                       io_impl_strings[j]) == 0) {
                cur_io_impl = j;
                break;
            }
        }
        ASSERT((int) cur_io_impl != -1,
               "Failed to find io implementation for workload %s\n",
               config->workloads[i].implementation);

        for (uint32_t j = 0; j < config->workloads[i].num_io_participations;
             j++) {
            const char *cur_participation_str =
                config->workloads[i].io_participations[j];
            io_participation_t cur_io_participation = -1;
            for (int k = 0; k < NUM_IO_IMPL; k++) {
                if (strcmp(cur_participation_str,
                           io_participation_strings[k]) == 0) {
                    cur_io_participation = k;
                    break;
                }
            }
            ASSERT((int) cur_io_participation != -1,
                   "Failed to find io participation for workload %s\n",
                   cur_participation_str);

            uint32_t elements_per_dim =
                (config->chunk_size_bytes / sizeof(double)) /
                (uint32_t) sqrt(config->chunk_size_bytes);

            uint64_t total_bytes = (uint64_t) config->chunks_per_rank *
                                   num_ranks * elements_per_dim *
                                   elements_per_dim * sizeof(double);
            uint64_t total_GB = total_bytes / (1024ULL * 1024 * 1024);

            io_filter_t cur_io_filter = -1;
            for(int k = 0; k < NUM_IO_FILTERS; k++) {
                if(strcmp(io_filter_strings[k], config->workloads[i].io_filter) == 0) {
                    cur_io_filter = k;
                    break;
                }
            }
            ASSERT((int) cur_io_filter != -1,
                   "Failed to find io filter for workload %s\n",
                   config->workloads[i].io_filter);
            io_impl_funcs[cur_io_impl].io_filter = cur_io_filter;

            // print workload run information
            MPI_Barrier(MPI_COMM_WORLD);
            TOGGLE_COLOR(COLOR_GREEN);
            PRINT_RANK0("==============================================\n");
            PRINT_RANK0("Starting workload %s\n", config->workloads[i].name);
            PRINT_RANK0("Running with %d rank(s)\n", num_ranks);
            PRINT_RANK0("Chunks per rank %lu\n", config->chunks_per_rank);
            PRINT_RANK0("Params %s", config->workloads[i].params);
            if (cur_io_participation == COLLECTIVE_IO)
                PRINT_RANK0("Using collective I/O\n");
            else
                PRINT_RANK0("Using independent I/O\n");
            PRINT_RANK0("Requested chunk size %lu bytes\n",
                        config->chunk_size_bytes);
            PRINT_RANK0("Rounded chunk size %lu bytes\n",
                        elements_per_dim * elements_per_dim * sizeof(double));
            PRINT_RANK0("Total data to be written: %lu GB (%lu) bytes\n",
                        total_GB, total_bytes);
            PRINT_RANK0("==============================================\n");
            TOGGLE_COLOR(COLOR_RESET);
            MPI_Barrier(MPI_COMM_WORLD);

            char* shell_path = NULL;

            int res = system(shell_path);
            ASSERT(res != -1, "Failed to recompile PDC for ZFP compress/decompression\n");

            // start workload
            exec_io_impl(config->workloads[i].params, cur_io_impl, io_impl_funcs[cur_io_impl],
                         elements_per_dim, num_ranks, config->chunks_per_rank,
                         rank, cur_io_participation, config->validate_read);

            // print workload finished information
            MPI_Barrier(MPI_COMM_WORLD);
            TOGGLE_COLOR(COLOR_GREEN);
            PRINT_RANK0("==============================================\n");
            print_all_timers_csv(CSV_FILENAME, config->chunks_per_rank,
                                 num_ranks, config->workloads[i].name, config->chunk_size_bytes, 
                                 cur_io_participation, cur_io_filter);
            PRINT_RANK0("Finished workload %s\n", config->workloads[i].name);
            PRINT_RANK0("==============================================\n");
            TOGGLE_COLOR(COLOR_RESET);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    free(config);

    return 0;
}
