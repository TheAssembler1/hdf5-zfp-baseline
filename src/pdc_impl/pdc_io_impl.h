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

void pdc_io_init(config_t *config, config_workload_t *config_workload);
void pdc_io_deinit(config_t *config, config_workload_t *config_workload);
void pdc_io_create_dataset(config_t *config,
                           config_workload_t *config_workload);
void pdc_io_open_dataset(config_t *config, config_workload_t *config_workload);
void pdc_io_close_dataset(config_t *config, config_workload_t *config_workload);
void pdc_io_write_chunk(config_t *config, config_workload_t *config_workload,
                        double *buffer);
void pdc_io_read_chunk(config_t *config, config_workload_t *config_workload,
                       double *buffer);
void pdc_io_flush(config_t *config, config_workload_t *config_workload);

#endif