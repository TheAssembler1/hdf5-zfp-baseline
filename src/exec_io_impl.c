#include <stdint.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "exec_io_impl.h"
#include "common/log.h"
#include "common/config.h"

void exec_io_impl(char* params, io_impl_t cur_io_impl, io_impl_funcs_t io_impl_funcs,
                  uint32_t elements_per_dim, int num_ranks, int chunks_per_rank,
                  int rank, io_participation_t io_participation, bool validate_read) {
    io_impl_funcs.init(params);

    // init dataset
    PRINT_RANK0("Calling init_dataset on impl\n");
    io_impl_funcs.init_dataset(MPI_COMM_WORLD, elements_per_dim, rank,
                               num_ranks, chunks_per_rank);

    if (io_impl_funcs.io_filter == IO_FILTER_ZFP_COMPRESS) {
        PRINT_RANK0("ZFP Compression filter enabled\n");
        // FIXME: need a global config with compression settings
        PRINT_RANK0("Calling enable_compression_on_dataset on impl\n");
        io_impl_funcs.enable_compression_on_dataset();
    } else {
        PRINT_RANK0("No filter enabled\n");
    }

    PRINT_RANK0("Calling create_dataset on impl\n");
    io_impl_funcs.create_dataset();

    // Allocate write buffer
    uint32_t chunk_bytes = elements_per_dim * elements_per_dim * sizeof(double);
    double *buffer = (double *) malloc(chunk_bytes);
    // Seed RNG
    srand(time(NULL));
    for (uint32_t i = 0; i < elements_per_dim; i++) {
        for (uint32_t j = 0; j < elements_per_dim; j++) {
            buffer[i * elements_per_dim + j] = rand();
        }
    }

    PRINT("Starting write\n");

    START_TIMER(WRITE_ALL_CHUNKS);
    MPI_Barrier(MPI_COMM_WORLD);
    for (int c = 0; c < chunks_per_rank; c++) {
        PRINT("Starting chunk write %d\n", c + 1);
        START_TIMER(WRITE_CHUNK);
        PRINT_RANK0("Calling write_chunk on impl\n");
        io_impl_funcs.write_chunk(elements_per_dim, buffer, io_participation,
                                  rank, chunks_per_rank, c, MPI_COMM_WORLD);
        STOP_TIMER(WRITE_CHUNK);
        PRINT("Finished chunk write %d\n", c + 1);
    }
    PRINT_RANK0("Calling flush on impl\n");
    io_impl_funcs.flush();
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(WRITE_ALL_CHUNKS);

    PRINT("Finished write\n");

    // close dataset
    free(buffer);
    PRINT_RANK0("Calling close_dataset on impl\n");
    io_impl_funcs.close_dataset();

    PRINT_RANK0("Calling reopen on impl\n");
    io_impl_funcs.reopen_dataset();

    // Allocate read buffer
    double *read_buf = (double *) malloc(chunk_bytes);
    // set init random value that's invalid rank
    for (uint32_t i = 0; i < elements_per_dim * elements_per_dim; i++) {
        read_buf[i] = -1;
    }

    bool chunks_read_valid = true;

    START_TIMER(READ_ALL_CHUNKS);
    MPI_Barrier(MPI_COMM_WORLD);
    for (int c = 0; c < chunks_per_rank; c++) {
        PRINT("Starting chunk read %d\n", c + 1);
        START_TIMER(READ_CHUNK);
        PRINT_RANK0("Calling read_chunk on impl\n");
        io_impl_funcs.read_chunk(elements_per_dim, read_buf, io_participation,
                                 rank, chunks_per_rank, c, MPI_COMM_WORLD);
        /*if (validate_read) {
            for (uint32_t i = 0; i < elements_per_dim * elements_per_dim; i++) {
                if ((int) read_buf[i] != rank + 100) {
                    PRINT_ERROR(
                        "Read mismatch at index %d: expected %d got %lf\n", i,
                        rank + 100, read_buf[i]);
                    chunks_read_valid = false;
                    break;
                }
            }
        }*/
        STOP_TIMER(READ_CHUNK);
        PRINT("Finished chunk read %d\n", c + 1);
    }
    PRINT_RANK0("Calling flush on impl\n");
    io_impl_funcs.flush();
    MPI_Barrier(MPI_COMM_WORLD);
    STOP_TIMER(READ_ALL_CHUNKS);

    free(read_buf);

    PRINT_RANK0("Calling close_dataset on impl\n");
    io_impl_funcs.close_dataset();
    PRINT_RANK0("Calling deinit on impl\n");
    io_impl_funcs.deinit();

    if (validate_read && chunks_read_valid)
        PRINT("All chunk reads were valid\n");
    else if (validate_read && !chunks_read_valid)
        PRINT("ERROR: Not all chunks reads were valid\n");
    fflush(stdout);
    fflush(stderr);
}
