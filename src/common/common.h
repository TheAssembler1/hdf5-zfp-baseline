#ifndef COMMON_H
#define COMMON_H

#define CSV_FILENAME "output.csv"

#define ELEMENTS_PER_CHUNK 4096
#define DATASET_NAME "dataset"
#define VALIDATE_DATA_READ

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
    WRITE_CHUNK,
    WRITE_ALL_CHUNKS,
    READ_CHUNK,
    READ_ALL_CHUNKS,
    TIMER_TAGS_COUNT
} timer_tags_t;

extern const char* timer_tags[];
extern double timer_start_times[TIMER_TAGS_COUNT];
extern double timer_accumulated[TIMER_TAGS_COUNT];

// Start the timer for a tag
#define START_TIMER(tag) \
    do { timer_start_times[(tag)] = MPI_Wtime(); } while(0)

// Stop the timer and accumulate elapsed time
#define STOP_TIMER(tag) \
    do { \
        double elapsed = MPI_Wtime() - timer_start_times[(tag)]; \
        timer_accumulated[(tag)] += elapsed; \
    } while(0)


void print_all_timers_csv(const char *filename, int chunks_per_rank, int num_ranks, bool scale_by_rank);

#endif