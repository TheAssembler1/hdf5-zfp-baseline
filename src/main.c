/**
 * Each I/O library needs to implement the following functions:
 *     1. init: intiailizes the library
 */

#include <stdbool.h>
#include <stdlib.h>
#include <mpi.h>

#include "common/common.h"
#include "hdf5_impl/hdf5_io_impl.h"
#include "pdc_impl/pdc_io_impl.h"

#define USAGE                                                                  \
    "./zfp_baseline <collective_io_enable:bool> "                              \
    "<chunks_per_rank:int> <scale_by_rank_enable:bool> "                       \
    "<zfp_filter_enable:bool>" \
    "<io_impl:0, 1 - PDC, HDF5>"

int main(int argc, char **argv) {
    int chunks_per_rank = 0;
    bool collective_io = 0;
    bool scale_by_rank = 0;
    bool zfp_compress = 0;
    io_impl_t cur_io_impl;

    io_impl_funcs_t io_impl_funcs[NUM_IMPL] = {
        [HDF5_IMPL] = {.init = hdf5_io_init,
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
        [PDC_IMPL] = {.init = pdc_io_init,
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

    if (argc < 6) {
        fprintf(stderr, "Invalid number of program arguments %d\n", argc);
        fprintf(stderr, USAGE);
        return 1;
    }
    // first arg is collective io
    collective_io = atoi(argv[1]);
    // second arg is chunks per rank
    chunks_per_rank = atoi(argv[2]);
    if (chunks_per_rank <= 0) {
        fprintf(stderr, "chunks per rank must be greater than 0\n");
        return 1;
    }
    // third arg is the scale type
    scale_by_rank = atoi(argv[3]);
    zfp_compress = atoi(argv[4]);
    cur_io_impl = atoi(argv[5]);

    if(cur_io_impl > 2 || cur_io_impl < 0) {
        fprintf(stderr, "Invalid io impl %d\n", cur_io_impl);
        fprintf(stderr, USAGE);
        return 1;
    }
    MPI_Init(&argc, &argv);

    int rank, num_ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    switch(cur_io_impl) {
        case HDF5_IMPL:
            PRINT_RANK0("Using HDF5 IO implementation\n");
            break;
        case PDC_IMPL:
            PRINT_RANK0("Using PDC IO implementation\n");
            break;
        default:
            abort();
    }

    io_impl_funcs[cur_io_impl].init();

    PRINT_RANK0("Running with %d rank(s)\n", num_ranks);
    PRINT_RANK0("Chunks per rank %d\n", chunks_per_rank);
    PRINT_RANK0("Total data to be written: %.2f MB\n",
                (chunks_per_rank * num_ranks * ELEMENTS_PER_CHUNK *
                 ELEMENTS_PER_CHUNK * sizeof(float)) /
                    (1024.0 * 1024.0));

    if (collective_io)
        PRINT_RANK0("Using collective I/O\n");
    else
        PRINT_RANK0("Using independent I/O\n");

    if (scale_by_rank) PRINT_RANK0("Detected scale by rank\n");

    MPI_Barrier(MPI_COMM_WORLD);
    PRINT_RANK0("\n========= Starting WRITE phase =========\n");

    // init dataset
    PRINT_RANK0("Calling init_dataset on impl\n");
    io_impl_funcs[cur_io_impl].init_dataset(MPI_COMM_WORLD, rank, num_ranks, chunks_per_rank);

    if (zfp_compress) {
        PRINT_RANK0("ZFP Compression filter enabled\n");
        // FIXME: need a global config with compression settings
        PRINT_RANK0("Calling enable_compression_on_dataset on impl\n");
        io_impl_funcs[cur_io_impl].enable_compression_on_dataset();
    } else
        PRINT_RANK0("ZFP Compression filter disabled\n");

    PRINT_RANK0("Calling create_dataset on impl\n");
    io_impl_funcs[cur_io_impl].create_dataset();

    // Allocate write buffer
    uint32_t chunk_bytes =
        ELEMENTS_PER_CHUNK * ELEMENTS_PER_CHUNK * sizeof(float);
    float *buffer = (float *) malloc(chunk_bytes);
    for (hsize_t i = 0; i < ELEMENTS_PER_CHUNK; i++) {
        for (hsize_t j = 0; j < ELEMENTS_PER_CHUNK; j++) {
            buffer[i * ELEMENTS_PER_CHUNK + j] = (float) rank;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("Rank %d: Starting write\n", rank);

    START_TIMER(WRITE_ALL_CHUNKS);
    for (int c = 0; c < chunks_per_rank; c++) {
        printf("Rank %d: Starting chunk write %d\n", rank, c + 1);
        START_TIMER(WRITE_CHUNK);
        PRINT_RANK0("Calling wrte_chunk on impl\n");
        io_impl_funcs[cur_io_impl].write_chunk(buffer, collective_io, rank,
                                               chunks_per_rank, c, MPI_COMM_WORLD);
        STOP_TIMER(WRITE_CHUNK);
        printf("Rank %d: Finished chunk write %d\n", rank, c + 1);
    }
    PRINT_RANK0("Calling flush on impl\n");
    io_impl_funcs[cur_io_impl].flush();
    STOP_TIMER(WRITE_ALL_CHUNKS);

    printf("Rank %d: Finished write\n", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    // close dataset
    free(buffer);
    PRINT_RANK0("Calling close_dataset on impl\n");
    io_impl_funcs[cur_io_impl].close_dataset();

    MPI_Barrier(MPI_COMM_WORLD);
    PRINT_RANK0("\n========= Starting READ phase =========\n");

    PRINT_RANK0("Calling reopen on impl\n");
    io_impl_funcs[cur_io_impl].reopen_dataset();

    // Allocate read buffer
    float *read_buf = (float *) malloc(chunk_bytes);
    // set init random value that's invalid rank
    for (int i = 0; i < ELEMENTS_PER_CHUNK * ELEMENTS_PER_CHUNK; i++) {
        read_buf[i] = -1;
    }

    bool chunks_read_valid = true;

    START_TIMER(READ_ALL_CHUNKS);
    for (int c = 0; c < chunks_per_rank; c++) {
        printf("Rank %d: Starting chunk read %d\n", rank, c + 1);
        START_TIMER(READ_CHUNK);
        PRINT_RANK0("Calling read_chunk on impl\n");
        io_impl_funcs[cur_io_impl].read_chunk(read_buf, collective_io, rank,
                                              chunks_per_rank, c, MPI_COMM_WORLD);
#ifdef VALIDATE_DATA_READ
        for (int i = 0; i < ELEMENTS_PER_CHUNK * ELEMENTS_PER_CHUNK; i++) {
            if ((int) read_buf[i] != rank) {
                fprintf(stderr,
                        "Read mismatch at index %d: expected %d got %d\n", i,
                        rank, (int) read_buf[i]);
                chunks_read_valid = false;
                break;
            }
        }
#endif
        STOP_TIMER(READ_CHUNK);
        printf("Rank %d: Finished chunk read %d\n", rank, c + 1);
    }
    STOP_TIMER(READ_ALL_CHUNKS);

    free(read_buf);

    PRINT_RANK0("Calling close_dataset on impl\n");
    io_impl_funcs[cur_io_impl].close_dataset();
    PRINT_RANK0("Calling deinit on impl\n");
    io_impl_funcs[cur_io_impl].deinit();

    if (chunks_read_valid)
        PRINT_RANK0("All chunk reads were valid\n");
    else
        PRINT_RANK0("ERROR: Not all chunks reads were valid\n");

    print_all_timers_csv(CSV_FILENAME, chunks_per_rank, num_ranks,
                         scale_by_rank, cur_io_impl);

    MPI_Finalize();

    return 0;
}
