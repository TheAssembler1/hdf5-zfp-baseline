#ifndef HDF5_IO_IMPL
#define HDF5_IO_IMPL

#include "../common/log.h"
#include "../common/common.h"
#include "hdf5.h"
#include "H5Zzfp.h"
#include "H5Zzfp_lib.h"
#include "H5Zzfp_plugin.h"
#include "H5Zzfp_props.h"
#include "H5Zzfp_version.h"

#define H5Z_FILTER_ZFP 32013

#define H5_ASSERT(val)                                                         \
    do {                                                                       \
        if ((val) < 0) {                                                       \
            PRINT_ERROR("H5_ASSERT failed: %s < 0 at %s:%d\n", #val, __FILE__, \
                        __LINE__);                                             \
            abort();                                                           \
        }                                                                      \
    } while (0)

void hdf5_io_init();
void hdf5_io_deinit();
void hdf5_io_init_dataset(MPI_Comm comm, uint32_t elements_per_dim, int my_rank,
                          int num_ranks, int chunks_per_rank);
void hdf5_io_create_dataset();
void hdf5_io_enable_compression_on_dataset();
void hdf5_io_write_chunk(uint32_t elements_per_dim, float *buffer,
                         io_participation_t io_participation, int rank,
                         int chunks_per_rank, int chunk, MPI_Comm comm);
void hdf5_io_read_chunk(uint32_t elements_per_dim, float *buffer,
                        io_participation_t io_participation, int rank,
                        int chunks_per_rank, int chunk, MPI_Comm comm);
void hdf5_io_flush();
void hdf5_io_close_dataset();
void hdf5_io_reopen_dataset();

#endif