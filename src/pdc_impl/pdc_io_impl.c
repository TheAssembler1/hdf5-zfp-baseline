#include <mpi.h>

#include "pdc_io_impl.h"
#include "../common/util.h"
#include "../common/common.h"
#include "../common/log.h"
#include "../common/config.h"

#include "pdc.h"

/**
 * NOTE: we don't close and reopen the container
 */

static pdcid_t pdc_g = 0;
static pdcid_t cont_g = 0;
static pdcid_t cont_prop_g = 0;
static pdcid_t obj_prop_g = 0;
static pdcid_t obj_g = 0;
static uint64_t dims[2];
static char *dataset_name = NULL;
static char *cont_name = NULL;
static int num_init_g = 0;

#define MAX_NAME_SIZE 255

void pdc_io_init() {
    static int num_init = 0;

    dataset_name = malloc(MAX_NAME_SIZE * sizeof(char));
    cont_name = malloc(MAX_NAME_SIZE * sizeof(char));
    sprintf(dataset_name, "obj_%d", num_init);
    sprintf(cont_name, "cont_%d", num_init);
    num_init++;

    if (num_init <= 1) pdc_g = PDCinit("pdc");
    PDC_ZERO_ASSERT(pdc_g);
}

void pdc_io_deinit() {
    free(dataset_name);
    free(cont_name);

    // PDC_NEG_ASSERT(PDCclose(pdc_g));
}

void pdc_io_init_dataset(MPI_Comm comm, uint32_t elements_per_dim, int my_rank,
                         int num_ranks, int chunks_per_rank) {
    uint64_t total_chunks = num_ranks * chunks_per_rank;

    cont_prop_g = PDCprop_create(PDC_CONT_CREATE, pdc_g);
    PRINT_RANK0("Creating container with name %s\n", cont_name);
    cont_g = PDCcont_create_col(cont_name, cont_prop_g);

    PDC_ZERO_ASSERT(cont_prop_g);
    PDC_ZERO_ASSERT(cont_g);

    dims[0] = total_chunks * elements_per_dim;
    dims[1] = elements_per_dim;

    obj_prop_g = PDCprop_create(PDC_OBJ_CREATE, pdc_g);
    PDC_ZERO_ASSERT(obj_prop_g);
    PDC_NEG_ASSERT(PDCprop_set_obj_dims(obj_prop_g, 2, dims));
    PDC_NEG_ASSERT(PDCprop_set_obj_type(obj_prop_g, PDC_FLOAT));

    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_OBJ_STATIC);
    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_STATIC);
    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_DYNAMIC);
    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_LOCAL);

    if (my_rank == 0) {
        PRINT_RANK0("Creating object with name %s\n", dataset_name);
        obj_g = PDCobj_create(cont_g, dataset_name, obj_prop_g);
        PDC_ZERO_ASSERT(obj_g);
        PDC_NEG_ASSERT(PDCobj_close(obj_g));
    }

    MPI_Barrier(comm);

    obj_g = PDCobj_open_col(dataset_name, cont_g);
    PDC_ZERO_ASSERT(obj_g);
}

void pdc_io_create_dataset() {
    // Already created during init (obj_g)
}

void pdc_io_enable_compression_on_dataset() {
    // Enabled at compilation time
}

void pdc_io_write_chunk(uint32_t elements_per_dim, float *buffer,
                        io_participation_t io_participation, int rank,
                        int chunks_per_rank, int chunk, MPI_Comm comm) {
    uint64_t local_offset[2], global_offset[2], offset_length[2];
    local_offset[0] = 0;
    local_offset[1] = 0;
    global_offset[0] =
        (uint64_t) (rank * chunks_per_rank + chunk) * elements_per_dim;
    global_offset[1] = 0;
    offset_length[0] = elements_per_dim;
    offset_length[1] = elements_per_dim;

    pdcid_t reg = PDCregion_create(2, local_offset, offset_length);
    pdcid_t reg_global = PDCregion_create(2, global_offset, offset_length);
    pdcid_t transfer =
        PDCregion_transfer_create(buffer, PDC_WRITE, obj_g, reg, reg_global);
    PDC_ZERO_ASSERT(reg);
    PDC_ZERO_ASSERT(reg_global);
    PDC_ZERO_ASSERT(transfer);

    if (io_participation == INDEPENDENT_IO)
        PDC_NEG_ASSERT(PDCregion_transfer_start(transfer));
    else
        PDC_NEG_ASSERT(PDCregion_transfer_start_mpi(transfer, comm));
    PDC_NEG_ASSERT(PDCregion_transfer_wait(transfer));
    PDC_NEG_ASSERT(PDCregion_transfer_close(transfer));

    PDC_NEG_ASSERT(PDCregion_close(reg));
    PDC_NEG_ASSERT(PDCregion_close(reg_global));
}

void pdc_io_read_chunk(uint32_t elements_per_dim, float *buffer,
                       io_participation_t io_participation, int rank,
                       int chunks_per_rank, int chunk, MPI_Comm comm) {
    uint64_t local_offset[2], global_offset[2], offset_length[2];
    local_offset[0] = 0;
    local_offset[1] = 0;
    global_offset[0] =
        (uint64_t) (rank * chunks_per_rank + chunk) * elements_per_dim;
    global_offset[1] = 0;
    offset_length[0] = elements_per_dim;
    offset_length[1] = elements_per_dim;

    pdcid_t reg = PDCregion_create(2, local_offset, offset_length);
    pdcid_t reg_global = PDCregion_create(2, global_offset, offset_length);
    pdcid_t transfer =
        PDCregion_transfer_create(buffer, PDC_READ, obj_g, reg, reg_global);

    PDC_ZERO_ASSERT(reg);
    PDC_ZERO_ASSERT(reg_global);
    PDC_ZERO_ASSERT(transfer);

    if (io_participation == INDEPENDENT_IO)
        PDC_NEG_ASSERT(PDCregion_transfer_start(transfer));
    else
        PDC_NEG_ASSERT(PDCregion_transfer_start_mpi(transfer, comm));
    PDC_NEG_ASSERT(PDCregion_transfer_wait(transfer));
    PDC_NEG_ASSERT(PDCregion_transfer_close(transfer));

    PDC_NEG_ASSERT(PDCregion_close(reg));
    PDC_NEG_ASSERT(PDCregion_close(reg_global));
}

void pdc_io_flush() {
    // No explicit flush API in PDC
}

void pdc_io_close_dataset() { PDC_NEG_ASSERT(PDCobj_close(obj_g)); }

void pdc_io_reopen_dataset() {
    obj_g = PDCobj_open_col(dataset_name, cont_g);
    PDC_ZERO_ASSERT(obj_g);
}