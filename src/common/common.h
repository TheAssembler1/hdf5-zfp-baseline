#ifndef COMMON_H
#define COMMON_H

#define CSV_FILENAME "output.csv"


#define CHUNK_SIZE_64MB  (4096)   // 4096 x 4096 floats × 4 bytes = 64 MB
#define CHUNK_SIZE_32MB  (2896)   // 2896 x 2896 floats × 4 bytes = 32 MB
#define CHUNK_SIZE_16MB  (2048)   // 2048 x 2048 floats × 4 bytes = 16 MB
#define CHUNK_SIZE_1MB   (512)    // 512 x 512 floats × 4 bytes = 1 MB
#define CHUNK_SIZE_512KB (362)    // 362 x 362 floats × 4 bytes ≈ 512 KB
#define CHUNK_SIZE_64KB  (128)    // 128 x 128 floats × 4 bytes = 64 KB
#define CHUNK_SIZE_32KB  (90)     // 90 x 90 floats × 4 bytes = 32 KB
#define CHUNK_SIZE_1KB   (16)     // 16 x 16 floats × 4 bytes = 1 KB
#define CHUNK_SIZE_4B    (1)	  // 1  x 1  floats x 4 bytes = 4 B

#define ELEMENTS_PER_CHUNK CHUNK_SIZE_64KB

#define DATASET_NAME "dataset"
#undef VALIDATE_DATA_READ

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
    void (*init_dataset)(MPI_Comm comm, int my_rank, int num_ranks, int chunks_per_rank);
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
                        int chunks_per_rank, int chunk, MPI_Comm comm);
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
                       int chunks_per_rank, int chunk, MPI_Comm comm);
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

#define PRINT_RANK0(fmt, ...)                                                  \
    do {                                                                       \
        int __rank;                                                            \
        MPI_Comm_rank(MPI_COMM_WORLD, &__rank);                                \
        if (__rank == 0) {                                                     \
            printf(fmt, ##__VA_ARGS__);                                        \
            fflush(stdout);                                                    \
        }                                                                      \
    } while (0)

typedef enum timer_tags_t {
    WRITE_CHUNK,
    WRITE_ALL_CHUNKS,
    READ_CHUNK,
    READ_ALL_CHUNKS,
    TIMER_TAGS_COUNT
} timer_tags_t;

extern const char *timer_tags[];
extern double timer_start_times[TIMER_TAGS_COUNT];
extern double timer_accumulated[TIMER_TAGS_COUNT];

// Start the timer for a tag
#define START_TIMER(tag)                                                       \
    do {                                                                       \
        timer_start_times[(tag)] = MPI_Wtime();                                \
    } while (0)

// Stop the timer and accumulate elapsed time
#define STOP_TIMER(tag)                                                        \
    do {                                                                       \
        double elapsed = MPI_Wtime() - timer_start_times[(tag)];               \
        timer_accumulated[(tag)] += elapsed;                                   \
    } while (0)

void print_all_timers_csv(const char *filename, int chunks_per_rank,
                          int num_ranks, bool scale_by_rank, io_impl_t impl);

#endif
