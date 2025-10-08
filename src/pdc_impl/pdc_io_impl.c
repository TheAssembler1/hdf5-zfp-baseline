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

#define OBJ_NAME "obj"
#define CONT_NAME "cont"
#define PDC_NAME "pdc"

void pdc_io_init(config_t *config, config_workload_t *config_workload) {
    pdc_g = PDCinit(PDC_NAME);
    PDC_ZERO_ASSERT(pdc_g);
}

void pdc_io_deinit(config_t *config, config_workload_t *config_workload) {
    PDC_NEG_ASSERT(PDCcont_close(cont_g));
    PDC_NEG_ASSERT(PDCclose(pdc_g));
}

void pdc_io_create_dataset(config_t *config,
                           config_workload_t *config_workload) {
    uint64_t total_chunks = config->num_ranks * config->chunks_per_rank;

    cont_prop_g = PDCprop_create(PDC_CONT_CREATE, pdc_g);
    PRINT_RANK0("Creating container with name %s\n", CONT_NAME);
    cont_g = PDCcont_create_col(CONT_NAME, cont_prop_g);

    PDC_ZERO_ASSERT(cont_prop_g);
    PDC_ZERO_ASSERT(cont_g);

    dims[0] = total_chunks * config->elements_per_dim;
    dims[1] = config->elements_per_dim;

    obj_prop_g = PDCprop_create(PDC_OBJ_CREATE, pdc_g);
    PDC_ZERO_ASSERT(obj_prop_g);
    PDC_NEG_ASSERT(PDCprop_set_obj_dims(obj_prop_g, 2, dims));
    PDC_NEG_ASSERT(PDCprop_set_obj_type(obj_prop_g, PDC_DOUBLE));

    PDCprop_set_obj_transfer_region_type(obj_prop_g, PDC_REGION_LOCAL);

    if (config->my_rank == 0) {
        PRINT_RANK0("Creating object with name %s\n", OBJ_NAME);
        obj_g = PDCobj_create(cont_g, OBJ_NAME, obj_prop_g);
        PDC_ZERO_ASSERT(obj_g);
        PDC_NEG_ASSERT(PDCobj_close(obj_g));
    }

    obj_g = PDCobj_open_col(OBJ_NAME, cont_g);
    PDC_ZERO_ASSERT(obj_g);

    if (!strcmp(config_workload->io_filter, "zfp")) {
        PRINT_RANK0("Enabling ZFP filter");
        dg_id_g = PDCtf_dg_json_create(TF_GRAPHS_DIR "compression.json");
        PDCtf_attach_to_obj(dg_id_g, obj_g, "decompressed", "compressed");
    }
}

pdcid_t transfers[1024];
static int num_transfers = 0;

static void pdc_io_helper(config_t *config, config_workload_t *config_workload,
                          double *buffer, pdc_access_t access_type) {
    num_transfers = config->cur_chunk + 1;

    uint64_t local_offset[2], global_offset[2], offset_length[2];
    local_offset[0] = 0;
    local_offset[1] = 0;
    global_offset[0] = (uint64_t) (config->my_rank * config->chunks_per_rank +
                                   config->cur_chunk) *
                       config->elements_per_dim;
    global_offset[1] = 0;
    offset_length[0] = config->elements_per_dim;
    offset_length[1] = config->elements_per_dim;

    pdcid_t reg = PDCregion_create(2, local_offset, offset_length);
    pdcid_t reg_global = PDCregion_create(2, global_offset, offset_length);

    transfers[config->cur_chunk] =
        PDCregion_transfer_create(buffer, access_type, obj_g, reg, reg_global);

    PDC_ZERO_ASSERT(reg);
    PDC_ZERO_ASSERT(reg_global);
    PDC_ZERO_ASSERT(transfers[config->cur_chunk]);

    // Check if we are in individual or batch mode
    if (!strcmp("batch", config_workload->params)) {
        // On last chunk need to start writing
        if (config->cur_chunk + 1 == config->chunks_per_rank) {
            if (!strcmp(config->io_participation, "independent"))
                PDC_NEG_ASSERT(
                    PDCregion_transfer_start_all(transfers, num_transfers));
            else {
                PRINT_ERROR("Collective I/O not supported in PDC\n");
                abort();
            }
        }
    } else if (!strcmp("individual", config_workload->params)) {
        if (!strcmp(config->io_participation, "independent"))
            PDC_NEG_ASSERT(
                PDCregion_transfer_start(transfers[config->cur_chunk]));
        else {
            PRINT_ERROR("Collective I/O not supported in PDC\n");
            abort();
        }
    } else {
        PRINT_ERROR("Invalid config_workload->params");
        abort();
    }

    PDC_NEG_ASSERT(PDCregion_close(reg));
    PDC_NEG_ASSERT(PDCregion_close(reg_global));
}

void pdc_io_write_chunk(config_t *config, config_workload_t *config_workload,
                        double *buffer) {
    pdc_io_helper(config, config_workload, buffer, PDC_WRITE);
}

void pdc_io_read_chunk(config_t *config, config_workload_t *config_workload,
                       double *buffer) {
    pdc_io_helper(config, config_workload, buffer, PDC_READ);
}

void pdc_io_flush(config_t *config, config_workload_t *config_workload) {
    // No explicit flush API in PDC
    if (!strcmp("batch", config_workload->params)) {
        PDC_NEG_ASSERT(PDCregion_transfer_wait_all(transfers, num_transfers));
    } else if (!strcmp("individual", config_workload->params)) {
        for (int i = 0; i < num_transfers; i++)
            PDC_NEG_ASSERT(PDCregion_transfer_wait(transfers[i]));
    } else {
        PRINT_ERROR("Invalid params_g");
        abort();
    }

    for (int i = 0; i < num_transfers; i++)
        PDC_NEG_ASSERT(PDCregion_transfer_close(transfers[i]));
}

void pdc_io_close_dataset(config_t *config,
                          config_workload_t *config_workload) {
    PDC_NEG_ASSERT(PDCobj_close(obj_g));
}

void pdc_io_open_dataset(config_t *config, config_workload_t *config_workload) {
    cont_g = PDCcont_open_col(CONT_NAME, pdc_g);
    PDC_ZERO_ASSERT(cont_g);
    obj_g = PDCobj_open_col(OBJ_NAME, cont_g);
    PDC_ZERO_ASSERT(obj_g);
}
