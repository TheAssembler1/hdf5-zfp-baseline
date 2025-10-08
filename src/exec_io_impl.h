#ifndef EXEC_IO_IMPL_H
#define EXEC_IO_IMPL_H

#include "common/common.h"
#include "common/config.h"

void exec_io_impl(io_impl_funcs_t io_impl_funcs, config_t *config,
                  config_workload_t *config_workload);

#endif