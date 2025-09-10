#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <mpi.h>

extern char *io_impl_strings[];
typedef enum io_impl_t {
    HDF5_IMPL,
    HDF5_ZFP_IMPL,
    PDC_IMPL,
    PDC_ZFP_IMPL,
    NUM_IO_IMPL
} io_impl_t;

extern char* io_filter_strings[];
typedef enum io_filter_t {
    IO_FILTER_RAW,
    IO_FILTER_ZFP_COMPRESS,
    IO_FILTER_ZFP_COMPRESS_TRANSFORM,
    NUM_IO_FILTERS
} io_filter_t;

extern const char *io_participation_strings[];
typedef enum io_participation_t {
    COLLECTIVE_IO,
    INDEPENDENT_IO,
    NUM_IO_PARTICIPATIONS
} io_participation_t;

typedef struct io_impl_funcs_t {
    io_filter_t io_filter;
    /**
     * Initializes the I/O library or backend.
     * Perform any necessary global or library-wide setup before any dataset or
     * file operations. This may include initializing internal state, setting up
     * parallel I/O contexts, or checking for available features or filters.
     */
    void (*init)(char* params);
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
    void (*init_dataset)(MPI_Comm comm, uint32_t elements_per_dim, int my_rank,
                         int num_ranks, int chunks_per_rank);
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
    void (*write_chunk)(uint32_t elements_per_dim, double *buffer,
                        io_participation_t io_participation, int rank,
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
    void (*read_chunk)(uint32_t elements_per_dim, double *buffer,
                       io_participation_t io_participation, int rank,
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
                          int num_ranks, char *workload_name, 
                          uint64_t chunk_size_bytes, 
                          io_participation_t io_participation,
                          io_filter_t io_filter);

#endif
