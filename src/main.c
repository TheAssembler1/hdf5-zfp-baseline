/**
 * Each I/O library needs to implement the following functions:
 *     1. init: intiailizes the library
 */

#include <stdbool.h>
#include <stdlib.h>

#include "common/common.h"
#include "hdf5_impl/hdf5_io_impl.h"
#include "pdc_impl/pdc_io_impl.h"

#define USAGE                                                                  \
    "./zfp_baseline <collective_io_enable:bool> "                              \
    "<chunks_per_rank:int> <scale_by_rank_enable:bool> "                       \
    "<zfp_filter_enable:bool>\n"

typedef enum io_impl_t { HDF5_IMPL, PDC_IMPL, NUM_IMPL } io_impl_t;

typedef struct io_impl_funcs_t {
    /**
     * Initializes the I/O library or backend.
     * Perform any necessary global or library-wide setup before any dataset or
     * file operations. This may include initializing internal state, setting up
     * parallel I/O contexts, or checking for available features or filters.
     */
    void (*init)();
    /**
     * Deinitializes the I/O library or backend.
     * Clean up any global state or resources allocated during initialization.
     * Ensure the library is properly closed and all resources are freed.
     */
    void (*deinit)();
    /**
     * Prepares the dataset and file structures for I/O operations.
     * Set up file access properties, create or open files, define dataset
     * dimensions and layouts, and prepare any metadata needed to manage the
     * dataset.
     *
     * Parameters:
     *  - num_ranks: number of parallel processes
     *  - chunks_per_rank: number of data chunks each process will handle
     */
    void (*init_dataset)(int num_ranks, int chunks_per_rank);
    /**
     * Creates the dataset within the file, using the configuration set during
     * initialization. Allocate storage, set chunking, compression, and other
     * dataset-specific properties. Must be called after dataset initialization
     * and before any read/write operations.
     */
    void (*create_dataset)();
    /**
     * Enables compression or other filters on the dataset before creation.
     * Configure dataset creation properties to apply compression or other
     * transforms. This function is typically called after dataset
     * initialization and before creation.
     */
    void (*enable_compression_on_dataset)();
    /**
     * Writes a chunk of data to the dataset.
     *
     * Parameters:
     *  - buffer: pointer to the data to write
     *  - collective_io: if true, use collective I/O semantics; otherwise,
     * independent I/O
     *  - rank: identifier of the current process (e.g., MPI rank)
     *  - chunks_per_rank: total chunks assigned to each process
     *  - chunk: index of the chunk to write within the process's assigned
     * chunks
     *
     * The implementation should select the appropriate region in the dataset
     * and write the data from the buffer according to the provided parameters.
     */
    void (*write_chunk)(float *buffer, bool collective_io, int rank,
                        int chunks_per_rank, int chunk);
    /**
     * Reads a chunk of data from the dataset.
     *
     * Parameters:
     *  - buffer: pointer to the memory where read data will be stored
     *  - collective_io: if true, use collective I/O semantics; otherwise,
     * independent I/O
     *  - rank: identifier of the current process
     *  - chunks_per_rank: total chunks assigned to each process
     *  - chunk: index of the chunk to read within the process's assigned chunks
     *
     * The implementation should select the appropriate region in the dataset
     * and read the data into the provided buffer according to the parameters.
     */
    void (*read_chunk)(float *buffer, bool collective_io, int rank,
                       int chunks_per_rank, int chunk);
    /**
     * Flushes any buffered writes to storage.
     * Ensures data consistency by committing any pending I/O operations to
     * durable storage.
     */
    void (*flush)();
    /**
     * Closes the dataset and file handles.
     * Release all resources associated with the dataset and file, ensuring a
     * clean shutdown.
     */
    void (*close_dataset)();
    /**
     * Reopens the dataset for further I/O after it has been closed.
     * This function should open the file and dataset (typically in read-only
     * mode) to prepare for subsequent read operations or further processing.
     */
    void (*reopen_dataset)();
} io_impl_funcs_t;

int main(int argc, char **argv) {
    int chunks_per_rank = 0;
    bool collective_io = 0;
    bool scale_by_rank = 0;
    bool zfp_compress = 0;
    io_impl_t cur_io_impl = PDC_IMPL;

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

    if (argc < 5) {
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

    MPI_Init(&argc, &argv);

    int rank, num_ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

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
    io_impl_funcs[cur_io_impl].init_dataset(num_ranks, chunks_per_rank);

    if (zfp_compress) {
        PRINT_RANK0("ZFP Compression filter enabled\n");
        // FIXME: need a global config with compression settings
        io_impl_funcs[cur_io_impl].enable_compression_on_dataset();
    } else
        PRINT_RANK0("ZFP Compression filter disabled\n");

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
        io_impl_funcs[cur_io_impl].write_chunk(buffer, collective_io, rank,
                                               chunks_per_rank, c);
        STOP_TIMER(WRITE_CHUNK);
        printf("Rank %d: Finished chunk write %d\n", rank, c + 1);
    }
    io_impl_funcs[cur_io_impl].flush();
    STOP_TIMER(WRITE_ALL_CHUNKS);

    printf("Rank %d: Finished write\n", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    // close dataset
    free(buffer);
    io_impl_funcs[cur_io_impl].close_dataset();

    MPI_Barrier(MPI_COMM_WORLD);
    PRINT_RANK0("\n========= Starting READ phase =========\n");

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
        io_impl_funcs[cur_io_impl].read_chunk(read_buf, collective_io, rank,
                                              chunks_per_rank, c);
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

    io_impl_funcs[cur_io_impl].close_dataset();
    io_impl_funcs[cur_io_impl].deinit();

    if (chunks_read_valid)
        PRINT_RANK0("All chunk reads were valid\n");
    else
        PRINT_RANK0("ERROR: Not all chunks reads were valid\n");

    print_all_timers_csv(CSV_FILENAME, chunks_per_rank, num_ranks,
                         scale_by_rank);

    MPI_Finalize();

    return 0;
}
