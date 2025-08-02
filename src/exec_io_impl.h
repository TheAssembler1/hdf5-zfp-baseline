#ifndef EXEC_IO_IMPL_H
#define EXEC_IO_IMPL_H

#include "common/common.h"

void exec_io_impl(io_impl_t cur_io_impl, io_impl_funcs_t io_impl_funcs,
                  uint32_t elements_per_dim, int num_ranks, int chunks_per_rank,
                  int rank, io_participation_t io_participation,
                  bool validate_read);

#endif