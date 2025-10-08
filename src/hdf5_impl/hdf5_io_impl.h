#ifndef HDF5_IO_IMPL
#define HDF5_IO_IMPL

#include "../common/log.h"
#include "../common/common.h"
#include "../common/config.h"
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

void hdf5_io_init(config_t *config, config_workload_t *config_workload);
void hdf5_io_deinit(config_t *config, config_workload_t *config_workload);
void hdf5_io_create_dataset(config_t *config,
                            config_workload_t *config_workload);
void hdf5_io_open_dataset(config_t *config, config_workload_t *config_workload);
void hdf5_io_close_dataset(config_t *config,
                           config_workload_t *config_workload);
void hdf5_io_write_chunk(config_t *config, config_workload_t *config_workload,
                         double *buffer);
void hdf5_io_read_chunk(config_t *config, config_workload_t *config_workload,
                        double *buffer);
void hdf5_io_flush(config_t *config, config_workload_t *config_workload);

#endif