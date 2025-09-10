#ifndef PDC_IO_IMPL
#define PDC_IO_IMPL

#include <stdbool.h>
#include "../common/common.h"
#include "../common/log.h"

#define PDC_NEG_ASSERT(val)                                                    \
    do {                                                                       \
        if ((val) < 0) {                                                       \
            PRINT_ERROR("PDC_ASSERT failed: %s < 0 at %s:%d\n", #val,          \
                        __FILE__, __LINE__);                                   \
            abort();                                                           \
        }                                                                      \
    } while (0)

#define PDC_ZERO_ASSERT(val)                                                   \
    do {                                                                       \
        if ((val) == 0) {                                                      \
            PRINT_ERROR("PDC_ASSERT failed: %s == 0 at %s:%d\n", #val,         \
                        __FILE__, __LINE__);                                   \
            abort();                                                           \
        }                                                                      \
    } while (0)

void pdc_io_init(char* params);
void pdc_io_deinit();
void pdc_io_init_dataset(MPI_Comm comm, uint32_t elements_per_dim, int my_rank,
                         int num_ranks, int chunks_per_rank);
void pdc_io_create_dataset();
void pdc_io_enable_compression_on_dataset();
void pdc_io_write_chunk(uint32_t elements_per_dim, double *buffer,
                        io_participation_t io_participation, int rank,
                        int chunks_per_rank, int chunk, MPI_Comm comm);
void pdc_io_read_chunk(uint32_t elements_per_dim, double *buffer,
                       io_participation_t io_participation, int rank,
                       int chunks_per_rank, int chunk, MPI_Comm comm);
void pdc_io_flush();
void pdc_io_close_dataset();
void pdc_io_reopen_dataset();

#endif