#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <mpi.h>

#include "config.h"

extern char *io_impl_strings[];
typedef enum io_impl_t {
    HDF5_IMPL,
    HDF5_ZFP_IMPL,
    PDC_IMPL,
    PDC_ZFP_IMPL,
    NUM_IO_IMPL
} io_impl_t;

typedef struct io_impl_funcs_t {
    /**
     * Initializes the I/O library or backend.
     * Perform any necessary global or library-wide setup before any dataset or
     * file operations. This may include initializing internal state, setting up
     * parallel I/O contexts, or checking for available features or filters.
     */
    void (*init)(config_t *config, config_workload_t *config_workload);
    /**
     * Deinitializes the I/O library or backend.
     * Clean up any global state or resources allocated during initialization.
     * Ensure the library is properly closed and all resources are freed.
     */
    void (*deinit)(config_t *config, config_workload_t *config_workload);
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
    void (*create_dataset)(config_t *config,
                           config_workload_t *config_workload);
    /**
     * Opens a dataset to be read from
     */
    void (*open_dataset)(config_t *config, config_workload_t *config_workload);
    /**
     * Enables compression or other filters on the dataset before creation.
     * Configure dataset creation properties to apply compression or other
     * transforms. This function is typically called after dataset
     * initialization and before creation.
     */
    void (*enable_compression_on_dataset)(config_t *config,
                                          config_workload_t *config_workload);
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
    void (*write_chunk)(config_t *config, config_workload_t *config_workload,
                        double *buffer);
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
    void (*read_chunk)(config_t *config, config_workload_t *config_workload,
                       double *buffer);
    /**
     * Flushes any buffered writes to storage.
     * Ensures data consistency by committing any pending I/O operations to
     * durable storage.
     */
    void (*flush)(config_t *config, config_workload_t *config_workload);
    /**
     * Closes the dataset and file handles.
     * Release all resources associated with the dataset and file, ensuring a
     * clean shutdown.
     */
    void (*close_dataset)(config_t *config, config_workload_t *config_workload);
} io_impl_funcs_t;

typedef enum timer_tags_t {
    WRITE_CHUNK,
    WRITE_ALL_CHUNKS,
    READ_CHUNK,
    READ_ALL_CHUNKS,
    WRITE_FLUSH,
    READ_FLUSH,
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

void print_all_timers_csv(config_t *config, config_workload_t *config_workload);

#endif
