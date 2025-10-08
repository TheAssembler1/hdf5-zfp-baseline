#include <stdint.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "exec_io_impl.h"
#include "common/log.h"
#include "common/config.h"

void exec_io_impl(io_impl_funcs_t io_impl_funcs, config_t *config,
                  config_workload_t *config_workload) {
    io_impl_funcs.init(config, config_workload);

    uint32_t chunk_bytes =
        config->elements_per_dim * config->elements_per_dim * sizeof(double);

    if (!strcmp(config_workload->io_type, "write")) {
        PRINT_RANK0("Calling create_dataset on impl\n");
        io_impl_funcs.create_dataset(config, config_workload);

        // Allocate write buffer
        double *write_buffer = (double *) malloc(chunk_bytes);
        // Seed RNG
        srand(42);
        for (uint32_t i = 0; i < config->elements_per_dim; i++) {
            for (uint32_t j = 0; j < config->elements_per_dim; j++) {
                write_buffer[i * config->elements_per_dim + j] =
                    (double) rand() + ((double) rand() / (double) RAND_MAX);
            }
        }

        PRINT_RANK0("Starting write\n");

        START_TIMER(WRITE_ALL_CHUNKS);
        MPI_Barrier(MPI_COMM_WORLD);
        for (config->cur_chunk = 0; config->cur_chunk < config->chunks_per_rank;
             config->cur_chunk++) {
            PRINT_RANK0("Starting chunk write %lu\n", config->cur_chunk);
            START_TIMER(WRITE_CHUNK);
            PRINT_RANK0("Calling write_chunk on impl\n");
            io_impl_funcs.write_chunk(config, config_workload, write_buffer);
            STOP_TIMER(WRITE_CHUNK);
            PRINT_RANK0("Finished chunk write %lu\n", config->cur_chunk);
        }
        PRINT_RANK0("Calling write flush on impl\n");
        START_TIMER(WRITE_FLUSH);
        io_impl_funcs.flush(config, config_workload);
        STOP_TIMER(WRITE_FLUSH);
        MPI_Barrier(MPI_COMM_WORLD);
        STOP_TIMER(WRITE_ALL_CHUNKS);

        // close dataset
        free(write_buffer);
    } else if ((!strcmp(config_workload->io_type, "read"))) {
        PRINT_RANK0("Calling open_dataset on impl\n");
        io_impl_funcs.open_dataset(config, config_workload);

        // Allocate read buffer
        double *read_buf =
            (double *) calloc(1, chunk_bytes * config->chunks_per_rank);

        START_TIMER(READ_ALL_CHUNKS);
        MPI_Barrier(MPI_COMM_WORLD);
        for (config->cur_chunk = 0; config->cur_chunk < config->chunks_per_rank;
             config->cur_chunk++) {
            PRINT_RANK0("Starting chunk read %lu\n", config->cur_chunk);
            START_TIMER(READ_CHUNK);
            PRINT_RANK0("Calling read_chunk on impl\n");
            io_impl_funcs.read_chunk(
                config, config_workload,
                &(read_buf[config->elements_per_dim * config->elements_per_dim *
                           config->cur_chunk]));
            STOP_TIMER(READ_CHUNK);
            PRINT_RANK0("Finished chunk read %lu\n", config->cur_chunk);
        }
        PRINT_RANK0("Calling read flush on impl\n");
        START_TIMER(READ_FLUSH);
        io_impl_funcs.flush(config, config_workload);
        STOP_TIMER(READ_FLUSH);
        MPI_Barrier(MPI_COMM_WORLD);
        STOP_TIMER(READ_ALL_CHUNKS);

        for (config->cur_chunk = 0; config->cur_chunk < config->chunks_per_rank;
             config->cur_chunk++) {
            srand(42);
            for (uint32_t i = 0; i < config->elements_per_dim; i++) {
                for (uint32_t j = 0; j < config->elements_per_dim; j++) {
                    double ran =
                        (double) rand() + ((double) rand() / (double) RAND_MAX);
                    double read_val =
                        read_buf[(config->cur_chunk * config->elements_per_dim *
                                  config->elements_per_dim) +
                                 (i * config->elements_per_dim + j)];

                    // Allow small difference due to rounding
                    double diff = fabs(read_val - ran);
                    const double tolerance = 1e-9;

                    if (diff > tolerance) {
                        PRINT_ERROR("Invalid data read (diff = %g)\n", diff);
                        abort();
                    }
                }
            }
        }
        PRINT_RANK0("Data read was valid :)\n");
        free(read_buf);
    } else {
        PRINT_ERROR("Invalid io type: %s\n", config_workload->io_type);
        abort();
    }

    PRINT_RANK0("Calling close_dataset on impl\n");
    io_impl_funcs.close_dataset(config, config_workload);
    PRINT_RANK0("Calling deinit on impl\n");
    io_impl_funcs.deinit(config, config_workload);

    fflush(stdout);
    fflush(stderr);
}
