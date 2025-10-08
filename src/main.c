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
    MPI_Init(&argc, &argv);

    int my_rank, num_ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    char *config_path = argv[1];
    config_t *config = init_config(config_path);

    // set the number of ranks
    config->num_ranks = num_ranks;
    config->my_rank = my_rank;

    io_impl_funcs_t io_impl_funcs[NUM_IO_IMPL] = {
        [HDF5_IMPL] = {.init = hdf5_io_init,
                       .deinit = hdf5_io_deinit,
                       .create_dataset = hdf5_io_create_dataset,
                       .write_chunk = hdf5_io_write_chunk,
                       .read_chunk = hdf5_io_read_chunk,
                       .flush = hdf5_io_flush,
                       .close_dataset = hdf5_io_close_dataset,
                       .open_dataset = hdf5_io_open_dataset},
        [PDC_IMPL] = {.init = pdc_io_init,
                      .deinit = pdc_io_deinit,
                      .create_dataset = pdc_io_create_dataset,
                      .write_chunk = pdc_io_write_chunk,
                      .read_chunk = pdc_io_read_chunk,
                      .flush = pdc_io_flush,
                      .close_dataset = pdc_io_close_dataset,
                      .open_dataset = pdc_io_open_dataset}};

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
            config->elements_per_dim =
                (uint32_t) sqrt(config->chunk_size_bytes / sizeof(double));
            config->total_bytes = (uint64_t) config->chunks_per_rank *
                                  num_ranks * config->elements_per_dim *
                                  config->elements_per_dim * sizeof(double);
            uint64_t total_GB = config->total_bytes / (1024ULL * 1024 * 1024);
            strcpy(config->io_participation,
                   config->workloads[i].io_participations[j]);

            // print workload run information
            MPI_Barrier(MPI_COMM_WORLD);
            TOGGLE_COLOR(COLOR_GREEN);
            PRINT_RANK0("==============================================\n");
            PRINT_RANK0("Starting workload %s\n", config->workloads[i].name);
            PRINT_RANK0("Running with %d rank(s)\n", num_ranks);
            PRINT_RANK0("Chunks per rank %lu\n", config->chunks_per_rank);
            PRINT_RANK0("Params %s", config->workloads[i].params);
            PRINT_RANK0("IO participation %s\n", config->io_participation);
            PRINT_RANK0("Requested chunk size %lu bytes\n",
                        config->chunk_size_bytes);
            PRINT_RANK0("Elements per dimension: %lu\n",
                        config->elements_per_dim);
            PRINT_RANK0("Total data to be written: %lu GB (%lu) bytes\n",
                        total_GB, config->total_bytes);
            PRINT_RANK0("IO type: %s\n", config->workloads[i].io_type);
            PRINT_RANK0("==============================================\n");
            TOGGLE_COLOR(COLOR_RESET);
            MPI_Barrier(MPI_COMM_WORLD);

            // start workload
            exec_io_impl(io_impl_funcs[cur_io_impl], config,
                         &(config->workloads[i]));

            // print workload finished information
            MPI_Barrier(MPI_COMM_WORLD);
            TOGGLE_COLOR(COLOR_GREEN);
            PRINT_RANK0("==============================================\n");
            print_all_timers_csv(config, &(config->workloads[i]));
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
