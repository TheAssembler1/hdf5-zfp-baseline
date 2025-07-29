#include <mpi.h>

#include "pdc_io_impl.h"
#include "../common/common.h"

#include "pdc.h"

/**
 * NOTE: we don't close and reopen the container
 */

static pdcid_t pdc_g;
static pdcid_t cont_g;
static pdcid_t cont_prop_g;
static pdcid_t obj_prop_g;
static pdcid_t obj_g;
static uint64_t dims[2];

void pdc_io_init() {
    pdc_g = PDCinit("pdc");
    PDC_ZERO_ASSERT(pdc_g);
}

void pdc_io_deinit() { PDC_NEG_ASSERT(PDCclose(pdc_g)); }

void pdc_io_init_dataset(MPI_Comm comm, int my_rank, int num_ranks, int chunks_per_rank) {
    uint64_t total_chunks = num_ranks * chunks_per_rank;

    cont_prop_g = PDCprop_create(PDC_CONT_CREATE, pdc_g);
    cont_g = PDCcont_create_col("pdc_container", cont_prop_g);

    PDC_ZERO_ASSERT(cont_prop_g);
    PDC_ZERO_ASSERT(cont_g);

    dims[0] = total_chunks * ELEMENTS_PER_CHUNK;
    dims[1] = ELEMENTS_PER_CHUNK;

    obj_prop_g = PDCprop_create(PDC_OBJ_CREATE, pdc_g);
    PDC_ZERO_ASSERT(obj_prop_g);
    PDC_NEG_ASSERT(PDCprop_set_obj_dims(obj_prop_g, 2, dims));
    PDC_NEG_ASSERT(PDCprop_set_obj_type(obj_prop_g, PDC_FLOAT));
    PDC_NEG_ASSERT(PDCprop_set_obj_time_step(obj_prop_g, 0));
    PDC_NEG_ASSERT(PDCprop_set_obj_user_id(obj_prop_g, getuid()));
    PDC_NEG_ASSERT(PDCprop_set_obj_app_name(obj_prop_g, "zfp-baseline"));

    if(my_rank == 0) {
        obj_g = PDCobj_create(cont_g, DATASET_NAME, obj_prop_g);
        PDC_ZERO_ASSERT(obj_g);
        PDC_NEG_ASSERT(PDCobj_close(obj_g));
    }

    MPI_Barrier(comm);

    obj_g = PDCobj_open_col(DATASET_NAME, cont_g);
    PDC_ZERO_ASSERT(obj_g);
}

void pdc_io_create_dataset() {
    // Already created during init (obj_g)
}

void pdc_io_enable_compression_on_dataset() {
    // Enabled at compilation time
}

void pdc_io_write_chunk(float *buffer, bool collective_io, int rank,
                        int chunks_per_rank, int chunk) {
    uint64_t local_offset[2], global_offset[2], offset_length[2];
    local_offset[0] = 0;
    local_offset[1] = 0;
    global_offset[0] =
        (uint64_t) (rank * chunks_per_rank + chunk) * ELEMENTS_PER_CHUNK;
    global_offset[1] = 0;
    offset_length[0] = ELEMENTS_PER_CHUNK;
    offset_length[1] = ELEMENTS_PER_CHUNK;

    pdcid_t reg = PDCregion_create(2, local_offset, offset_length);
    pdcid_t reg_global = PDCregion_create(2, global_offset, offset_length);
    pdcid_t transfer =
        PDCregion_transfer_create(buffer, PDC_WRITE, obj_g, reg, reg_global);

    PDC_ZERO_ASSERT(reg);
    PDC_ZERO_ASSERT(reg_global);
    PDC_ZERO_ASSERT(transfer);

    PDC_NEG_ASSERT(PDCregion_transfer_start(transfer));
    PDC_NEG_ASSERT(PDCregion_transfer_wait(transfer));
    PDC_NEG_ASSERT(PDCregion_transfer_close(transfer));

    PDC_NEG_ASSERT(PDCregion_close(reg));
    PDC_NEG_ASSERT(PDCregion_close(reg_global));
}

void pdc_io_read_chunk(float *buffer, bool collective_io, int rank,
                       int chunks_per_rank, int chunk) {
    uint64_t local_offset[2], global_offset[2], offset_length[2];
    local_offset[0] = 0;
    local_offset[1] = 0;
    global_offset[0] =
        (uint64_t) (rank * chunks_per_rank + chunk) * ELEMENTS_PER_CHUNK;
    global_offset[1] = 0;
    offset_length[0] = ELEMENTS_PER_CHUNK;
    offset_length[1] = ELEMENTS_PER_CHUNK;

    pdcid_t reg = PDCregion_create(2, local_offset, offset_length);
    pdcid_t reg_global = PDCregion_create(2, global_offset, offset_length);
    pdcid_t transfer =
        PDCregion_transfer_create(buffer, PDC_READ, obj_g, reg, reg_global);

    PDC_ZERO_ASSERT(reg);
    PDC_ZERO_ASSERT(reg_global);
    PDC_ZERO_ASSERT(transfer);

    PDC_NEG_ASSERT(PDCregion_transfer_start(transfer));
    PDC_NEG_ASSERT(PDCregion_transfer_wait(transfer));
    PDC_NEG_ASSERT(PDCregion_transfer_close(transfer));

    PDC_NEG_ASSERT(PDCregion_close(reg));
    PDC_NEG_ASSERT(PDCregion_close(reg_global));
}

void pdc_io_flush() {
    // No explicit flush API in PDC
}

void pdc_io_close_dataset() {
    PDC_NEG_ASSERT(PDCobj_close(obj_g));
}

void pdc_io_reopen_dataset() {
    obj_g = PDCobj_open_col(DATASET_NAME, cont_g);
    PDC_ZERO_ASSERT(obj_g);
}