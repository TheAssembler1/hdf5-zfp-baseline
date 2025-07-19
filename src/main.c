#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>
#include "hdf5.h"

#define CSV_FILENAME "output.csv"

#define MB_PER_CHUNK 64
#define DATASET_NAME "dataset"
#define USE_COLLECTIVE_IO 1

#define PRINT_RANK0(fmt, ...) \
    do { \
        int __rank; \
        MPI_Comm_rank(MPI_COMM_WORLD, &__rank); \
        if (__rank == 0) { \
            printf(fmt, ##__VA_ARGS__); \
            fflush(stdout); \
        } \
    } while (0)

typedef enum timer_tags_t {
    H5_WRITE_CHUNK,
    H5_WRITE_ALL_CHUNKS,
    H5_READ_CHUNK,
    H5_READ_ALL_CHUNKS,
    TIMER_TAGS_COUNT
} timer_tags_t;

const char* timer_tags[] = {
    "h5_write_chunk",
    "h5_write_all_chunks",
    "h5_read_chunk",
    "h5_read_all_chunks"
};

static double timer_start_times[TIMER_TAGS_COUNT];
static double timer_accumulated[TIMER_TAGS_COUNT] = {0};

// Start the timer for a tag
#define START_TIMER(tag) \
    do { timer_start_times[(tag)] = MPI_Wtime(); } while(0)

// Stop the timer and accumulate elapsed time
#define STOP_TIMER(tag) \
    do { \
        double elapsed = MPI_Wtime() - timer_start_times[(tag)]; \
        timer_accumulated[(tag)] += elapsed; \
    } while(0)

static void print_all_timers_csv(const char *filename, int chunks_per_rank, int num_ranks, bool scale_by_rank) {
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

            if(i == H5_WRITE_ALL_CHUNKS || i == H5_READ_ALL_CHUNKS)
                fprintf(fp, "%s,%d,%d,%s,%f\n", scale_type, chunks_per_rank, num_ranks, timer_tags[i], timer_accumulated[i]);
            else 
                fprintf(fp, "%s,%d,%d,%s,%f\n", scale_type, chunks_per_rank, num_ranks, timer_tags[i], timer_accumulated[i] / chunks_per_rank);
        }

        fclose(fp);
        printf("Timer results appended to %s\n", filename);
    }
}

#define USAGE "./zfp_baseline <collective_io> <chunks_per_rank> <scale_by_rank>\n"

int main(int argc, char** argv) {
    int chunks_per_rank = 0;
    bool collective_io = 0;
    bool scale_by_rank = 0;

    if(argc < 4) {
        fprintf(stderr, "Invalid number of program arguments\n");
        fprintf(stderr, USAGE);
        return 1;
    }

    // first arg is collective io
    collective_io = atoi(argv[1]);
    // second arg is chunks per rank
    chunks_per_rank = atoi(argv[2]);
    if(chunks_per_rank <= 0) {
        fprintf(stderr, "chunks per rank must be greater than 0\n");
        return 1;
    }
    // third arg is the scale type
    scale_by_rank = atoi(argv[3]);

    MPI_Init(&argc, &argv);
    H5open();

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    PRINT_RANK0("Running with %d rank(s)\n", nprocs);

#if USE_COLLECTIVE_IO
    PRINT_RANK0("Using collective I/O\n");
#else
    PRINT_RANK0("Using independent I/O\n");
#endif

    if(scale_by_rank)
        PRINT_RANK0("Detected scale by rank\n");

    MPI_Barrier(MPI_COMM_WORLD);
    PRINT_RANK0("\n========= Starting WRITE phase =========\n");

    const hsize_t chunk_bytes = MB_PER_CHUNK * 1024 * 1024;
    const hsize_t total_chunks = nprocs * chunks_per_rank;
    const hsize_t total_bytes = total_chunks * chunk_bytes;

    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL);

    hid_t file = H5Fcreate("output.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
    H5Pclose(fapl);

    hsize_t dims[1] = { total_bytes };
    hsize_t chunk_dims[1] = { chunk_bytes };

    hid_t space = H5Screate_simple(1, dims, NULL);

    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(dcpl, 1, chunk_dims);

    hid_t dset = H5Dcreate(file, DATASET_NAME, H5T_NATIVE_CHAR, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    H5Pclose(dcpl);
    H5Sclose(space);

    // Allocate write buffer
    char *buffer = (char*) malloc(chunk_bytes);
    for (hsize_t i = 0; i < chunk_bytes; i++) {
        buffer[i] = (char)(rank % 256);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    printf("Rank %d: Starting write\n", rank);

    START_TIMER(H5_WRITE_ALL_CHUNKS);
    for (int c = 0; c < chunks_per_rank; c++) {
        hsize_t offset[1] = { (rank * chunks_per_rank + c) * chunk_bytes };
        hsize_t size[1] = { chunk_bytes };

        hid_t filespace = H5Dget_space(dset);
        H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, size, NULL);

        hid_t memspace = H5Screate_simple(1, size, NULL);

        hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
#if USE_COLLECTIVE_IO
        H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE);
#else
        H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_INDEPENDENT);
#endif

        printf("Rank %d: Starting chunk write %d\n", rank, c + 1);
        START_TIMER(H5_WRITE_CHUNK);
        H5Dwrite(dset, H5T_NATIVE_CHAR, memspace, filespace, dxpl, buffer);
        H5Fflush(file, H5F_SCOPE_GLOBAL);
        STOP_TIMER(H5_WRITE_CHUNK);
        printf("Rank %d: Finished chunk write %d\n", rank, c + 1);

        H5Pclose(dxpl);
        H5Sclose(memspace);
        H5Sclose(filespace);
    }
    STOP_TIMER(H5_WRITE_ALL_CHUNKS);

    printf("Rank %d: Finished write\n", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    free(buffer);
    H5Dclose(dset);
    H5Fclose(file);

    MPI_Barrier(MPI_COMM_WORLD);
    PRINT_RANK0("\n========= Starting READ phase =========\n");

    // Reopen file and dataset
    fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(fapl, MPI_COMM_WORLD, MPI_INFO_NULL);
    file = H5Fopen("output.h5", H5F_ACC_RDONLY, fapl);
    H5Pclose(fapl);

    dset = H5Dopen(file, DATASET_NAME, H5P_DEFAULT);

    // Allocate read buffer
    char *read_buf = (char*) malloc(chunk_bytes);

    START_TIMER(H5_READ_ALL_CHUNKS);
    for (int c = 0; c < chunks_per_rank; c++) {
        hsize_t offset[1] = { (rank * chunks_per_rank + c) * chunk_bytes };
        hsize_t size[1] = { chunk_bytes };

        hid_t filespace = H5Dget_space(dset);
        H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, size, NULL);

        hid_t memspace = H5Screate_simple(1, size, NULL);

        hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);

        if(collective_io)
            H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE);
        else
            H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_INDEPENDENT);

        printf("Rank %d: Starting chunk read %d\n", rank, c + 1);
        START_TIMER(H5_READ_CHUNK);
        H5Dread(dset, H5T_NATIVE_CHAR, memspace, filespace, dxpl, read_buf);
        STOP_TIMER(H5_READ_CHUNK);
        printf("Rank %d: Finished chunk read %d\n", rank, c + 1);

        // Optionally verify or print first few bytes
        if(read_buf[0] != rank)
            fprintf(stderr, "Chunk had invalid value expected %d got %d\n", rank, read_buf[0]);

        H5Pclose(dxpl);
        H5Sclose(memspace);
        H5Sclose(filespace);
    }
    STOP_TIMER(H5_READ_ALL_CHUNKS);

    free(read_buf);
    H5Dclose(dset);
    H5Fclose(file);

    H5close();

    print_all_timers_csv(CSV_FILENAME, chunks_per_rank, nprocs, scale_by_rank);

    MPI_Finalize();

    return 0;
}
