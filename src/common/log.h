#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <assert.h>

#define PRINT_RANK0(fmt, ...)                                                  \
    do {                                                                       \
        int __rank;                                                            \
        MPI_Comm_rank(MPI_COMM_WORLD, &__rank);                                \
        if (__rank == 0) {                                                     \
            printf("Rank[0] " fmt, ##__VA_ARGS__);                             \
            fflush(stdout);                                                    \
        }                                                                      \
    } while (0)

#define PRINT(fmt, ...)                                                        \
    do {                                                                       \
        int __rank;                                                            \
        MPI_Comm_rank(MPI_COMM_WORLD, &__rank);                                \
        printf("Rank[%d] " fmt, __rank, ##__VA_ARGS__);                        \
        fflush(stdout);                                                        \
    } while (0)

#define PRINT_ERROR(fmt, ...)                                                  \
    do {                                                                       \
        int __rank;                                                            \
        MPI_Comm_rank(MPI_COMM_WORLD, &__rank);                                \
        fprintf(stderr, "ERROR [Rank %d] " fmt, __rank, ##__VA_ARGS__);        \
        fflush(stderr);                                                        \
    } while (0)

#define PRINT_ERROR_RANK0(fmt, ...)                                            \
    do {                                                                       \
        int __rank;                                                            \
        MPI_Comm_rank(MPI_COMM_WORLD, &__rank);                                \
        if (__rank == 0) {                                                     \
            fprintf(stderr, "ERROR [Rank 0] " fmt, ##__VA_ARGS__);             \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

#define ASSERT(val, msg, ...)                                                  \
    do {                                                                       \
        if (!(val)) {                                                          \
            PRINT_ERROR(msg, ##__VA_ARGS__);                                   \
            assert(val);                                                       \
        }                                                                      \
    } while (0)

#define COLOR_RESET "\033[0m"

#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

#define BOLD_BLACK "\033[1;30m"
#define BOLD_RED "\033[1;31m"
#define BOLD_GREEN "\033[1;32m"
#define BOLD_YELLOW "\033[1;33m"
#define BOLD_BLUE "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN "\033[1;36m"
#define BOLD_WHITE "\033[1;37m"

#undef ENABLE_COLOR

#ifdef ENABLE_COLOR
#define TOGGLE_COLOR(color) printf("%s", color)
#else 
#define TOGGLE_COLOR(color)
#endif

#endif