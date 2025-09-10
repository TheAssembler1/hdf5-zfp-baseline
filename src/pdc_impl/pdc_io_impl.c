#include <mpi.h>

#include "pdc_io_impl.h"
#include "../common/util.h"
#include "../common/common.h"
#include "../common/log.h"
#include "../common/config.h"

#define TF_GRAPHS_DIR "/pscratch/sd/n/nlewi26/src/hdf5-zfp-baseline/tf_graphs/"
#include "pdc.h"

/**
 * NOTE: we don't close and reopen the container
 */

static pdcid_t pdc_g = 0;
static pdcid_t cont_g = 0;
static pdcid_t cont_prop_g = 0;
static pdcid_t obj_prop_g = 0;
static pdcid_t obj_g = 0;
static pdcid_t dg_id_g = 0;
static uint64_t dims[2];
static char *dataset_name = NULL;
static char *cont_name = NULL;
static int num_init_g = 0;
static char* params_g = NULL;

#define MAX_NAME_SIZE 255

void pdc_io_init(char* params) {
    params_g = strdup(params);
    dataset_name = malloc(MAX_NAME_SIZE * sizeof(char));
    cont_name = malloc(MAX_NAME_SIZE * sizeof(char));
    sprintf(dataset_name, "obj_%d", num_init_g);
    sprintf(cont_name, "cont_%d", num_init_g);
    num_init_g++;

    if (num_init_g <= 1) pdc_g = PDCinit("pdc");

    PDC_ZERO_ASSERT(pdc_g);
}

void pdc_io_deinit() {
    free(dataset_name);
    free(cont_name);
    //PDC_NEG_ASSERT(PDCclose(pdc_g));
    //num_init_g = 0;
}

void pdc_io_init_dataset(MPI_Comm comm, uint32_t elements_per_dim, int my_rank,
                         int num_ranks, int chunks_per_rank) {
    uint64_t total_chunks = num_ranks * chunks_per_rank;

    cont_prop_g = PDCprop_create(PDC_CONT_CREATE, pdc_g);
    printf("Creating container with name %s\n", cont_name);
    cont_g = PDCcont_create_col(cont_name, cont_prop_g);

    PDC_ZERO_ASSERT(cont_prop_g);
    PDC_ZERO_ASSERT(cont_g);

    dims[0] = total_chunks * elements_per_dim;
    dims[1] = elements_per_dim;

    obj_prop_g = PDCprop_create(PDC_OBJ_CREATE, pdc_g);
    PDC_ZERO_ASSERT(obj_prop_g);
    PDC_NEG_ASSERT(PDCprop_set_obj_dims(obj_prop_g, 2, dims));
    PDC_NEG_ASSERT(PDCprop_set_obj_type(obj_prop_g, PDC_DOUBLE));

    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_OBJ_STATIC);
    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_STATIC);
    // PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_DYNAMIC);
    PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_LOCAL);

    if (my_rank == 0) {
        PRINT_RANK0("Creating object with name %s\n", dataset_name);
        obj_g = PDCobj_create(cont_g, dataset_name, obj_prop_g);
        PDC_ZERO_ASSERT(obj_g);
        PDC_NEG_ASSERT(PDCobj_close(obj_g));
    }

    obj_g = PDCobj_open_col(dataset_name, cont_g);
    PDC_ZERO_ASSERT(obj_g);
}

void pdc_io_create_dataset() {
    // Already created during init (obj_g)
}

void pdc_io_enable_compression_on_dataset() {
    dg_id_g = PDCtf_dg_json_create(TF_GRAPHS_DIR "compression.json");
    PDCtf_attach_to_obj(dg_id_g, obj_g, "decompressed", "compressed");
}

pdcid_t transfers[1024];
static int num_transfers = 0;

static void pdc_io_helper(uint32_t elements_per_dim, double *buffer,
                        io_participation_t io_participation, int rank,
                        int chunks_per_rank, int chunk_index, MPI_Comm comm, pdc_access_t access_type) {
    num_transfers = chunk_index + 1;

    uint64_t local_offset[2], global_offset[2], offset_length[2];
    local_offset[0] = 0;
    local_offset[1] = 0;
    global_offset[0] =
        (uint64_t) (rank * chunks_per_rank + chunk_index) * elements_per_dim;
    global_offset[1] = 0;
    offset_length[0] = elements_per_dim;
    offset_length[1] = elements_per_dim;

    pdcid_t reg = PDCregion_create(2, local_offset, offset_length);
    pdcid_t reg_global = PDCregion_create(2, global_offset, offset_length);


    transfers[chunk_index] =
        PDCregion_transfer_create(buffer, access_type, obj_g, reg, reg_global);

    PDC_ZERO_ASSERT(reg);
    PDC_ZERO_ASSERT(reg_global);
    PDC_ZERO_ASSERT(transfers[chunk_index]);

    // Check if we are in individual or batch mode
    if(!strcmp("batch", params_g)) {
        // On last chunk need to start writing
        if(chunk_index + 1 == chunks_per_rank) {
            if (io_participation == INDEPENDENT_IO)
                PDC_NEG_ASSERT(PDCregion_transfer_start_all(transfers, num_transfers));
            else {
                PRINT_ERROR("Collective I/O not supported in PDC\n");
                abort();
            }
        }
    } else if (!strcmp("individual", params_g)) {
        if (io_participation == INDEPENDENT_IO)
            PDC_NEG_ASSERT(PDCregion_transfer_start(transfers[chunk_index]));
        else {
            PRINT_ERROR("Collective I/O not supported in PDC\n");
            abort();
        }
    } else {
        PRINT_ERROR("Invalid params_g");
        abort();
    }

    PDC_NEG_ASSERT(PDCregion_close(reg));
    PDC_NEG_ASSERT(PDCregion_close(reg_global));
}

void pdc_io_write_chunk(uint32_t elements_per_dim, double *buffer,
                        io_participation_t io_participation, int rank,
                        int chunks_per_rank, int chunk_index, MPI_Comm comm) {
    pdc_io_helper(elements_per_dim, buffer, io_participation, rank, chunks_per_rank, chunk_index, comm, PDC_WRITE);
}

void pdc_io_read_chunk(uint32_t elements_per_dim, double *buffer,
                       io_participation_t io_participation, int rank,
                       int chunks_per_rank, int chunk_index, MPI_Comm comm) {
    pdc_io_helper(elements_per_dim, buffer, io_participation, rank, chunks_per_rank, chunk_index, comm, PDC_READ);
}

void pdc_io_flush() {
    // No explicit flush API in PDC
    if(!strcmp("batch", params_g)) {
        PDC_NEG_ASSERT(PDCregion_transfer_wait_all(transfers, num_transfers));
    } else if (!strcmp("individual", params_g)) {
        for(int i = 0; i < num_transfers; i++)
            PDC_NEG_ASSERT(PDCregion_transfer_wait(transfers[i]));
    } else {
        PRINT_ERROR("Invalid params_g");
        abort();
    }

    for(int i = 0; i < num_transfers; i++)
        PDC_NEG_ASSERT(PDCregion_transfer_close(transfers[i]));
}

void pdc_io_close_dataset() { 
    //PDC_NEG_ASSERT(PDCobj_close(obj_g)); 
}

void pdc_io_reopen_dataset() {
    //obj_g = PDCobj_open_col(dataset_name, cont_g);
    //PDC_ZERO_ASSERT(obj_g);
}
