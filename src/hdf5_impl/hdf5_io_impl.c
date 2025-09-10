#include <stdlib.h>
#include <string.h>

#include "hdf5_io_impl.h"
#include "../common/util.h"
#include "../common/common.h"
#include "../common/config.h"

hid_t dcpl_g = -1;
hid_t dset_g = -1;
hid_t filespace_g = -1;
hid_t file_g = -1;
hid_t space_g = -1;
hid_t fapl_g = -1;
static char *dataset_name = NULL;

#define MAX_NAME_SIZE 255

void hdf5_io_init(char* params) {
    static int num_init = 0;

    dataset_name = malloc(MAX_NAME_SIZE * sizeof(char));
    sprintf(dataset_name, "obj_%d", num_init);

    num_init++;
    if (num_init > 1) return;

    H5_ASSERT(H5open());

    int filter_avail = H5Zfilter_avail(H5Z_FILTER_ZFP);
    PRINT_RANK0("ZFP filter available? %s\n", filter_avail ? "YES" : "NO");
    if (!filter_avail) abort();
}

void hdf5_io_deinit() {
    free(dataset_name);
    H5_ASSERT(H5close());
}

void hdf5_io_init_dataset(MPI_Comm comm, uint32_t elements_per_dim, int my_rank,
                          int num_ranks, int chunks_per_rank) {
    const hsize_t total_chunks = num_ranks * chunks_per_rank;

    fapl_g = H5Pcreate(H5P_FILE_ACCESS);
    H5_ASSERT(H5Pset_fapl_mpio(fapl_g, MPI_COMM_WORLD, MPI_INFO_NULL));

    file_g = H5Fcreate("output.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_g);
    H5_ASSERT(file_g);
    H5Pclose(fapl_g);

    hsize_t dims[2] = {total_chunks * elements_per_dim, elements_per_dim};
    hsize_t chunk_dims[2] = {elements_per_dim, elements_per_dim};

    space_g = H5Screate_simple(2, dims, NULL);

    dcpl_g = H5Pcreate(H5P_DATASET_CREATE);
    H5_ASSERT(dcpl_g);
    H5_ASSERT(H5Pset_chunk(dcpl_g, 2, chunk_dims));
}

void hdf5_io_create_dataset() {
    dset_g = H5Dcreate(file_g, dataset_name, H5T_NATIVE_DOUBLE, space_g,
                       H5P_DEFAULT, dcpl_g, H5P_DEFAULT);
    H5_ASSERT(dset_g);
    H5_ASSERT(H5Pclose(dcpl_g));
    H5_ASSERT(H5Sclose(space_g));
}

void hdf5_io_write_chunk(uint32_t elements_per_dim, double *buffer,
                         io_participation_t io_participation, int rank,
                         int chunks_per_rank, int chunk, MPI_Comm comm) {
    hsize_t offset[2] = {(rank * chunks_per_rank + chunk) * elements_per_dim,
                         0};
    hsize_t size[2] = {elements_per_dim, elements_per_dim};

    filespace_g = H5Dget_space(dset_g);
    H5_ASSERT(filespace_g);
    H5_ASSERT(H5Sselect_hyperslab(filespace_g, H5S_SELECT_SET, offset, NULL,
                                  size, NULL));

    hid_t memspace = H5Screate_simple(2, size, NULL);
    H5_ASSERT(memspace);

    hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
    H5_ASSERT(dxpl);

    if (io_participation == COLLECTIVE_IO)
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE));
    else
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_INDEPENDENT));

    H5_ASSERT(H5Dwrite(dset_g, H5T_NATIVE_DOUBLE, memspace, filespace_g, dxpl,
                       buffer));

    H5_ASSERT(H5Pclose(dxpl));
    H5_ASSERT(H5Sclose(memspace));
    H5_ASSERT(H5Sclose(filespace_g));
}

void hdf5_io_read_chunk(uint32_t elements_per_dim, double *read_buf,
                        io_participation_t io_participation, int rank,
                        int chunks_per_rank, int chunk, MPI_Comm comm) {
    hsize_t offset[2] = {(rank * chunks_per_rank + chunk) * elements_per_dim,
                         0};
    hsize_t size[2] = {elements_per_dim, elements_per_dim};

    filespace_g = H5Dget_space(dset_g);
    H5_ASSERT(filespace_g);
    H5_ASSERT(H5Sselect_hyperslab(filespace_g, H5S_SELECT_SET, offset, NULL,
                                  size, NULL));

    hid_t memspace = H5Screate_simple(2, size, NULL);
    H5_ASSERT(memspace);

    hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
    H5_ASSERT(dxpl);

    if (io_participation == COLLECTIVE_IO)
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE));
    else
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_INDEPENDENT));

    H5_ASSERT(H5Dread(dset_g, H5T_NATIVE_DOUBLE, memspace, filespace_g, dxpl,
                      read_buf));

    H5_ASSERT(H5Pclose(dxpl));
    H5_ASSERT(H5Sclose(memspace));
    H5_ASSERT(H5Sclose(filespace_g));
}

void hdf5_io_flush() { H5_ASSERT(H5Fflush(file_g, H5F_SCOPE_GLOBAL)); }

void hdf5_io_enable_compression_on_dataset() {
    unsigned int cd_values[6];
    size_t cd_nelmts = 6;

    H5Pset_zfp_reversible_cdata(cd_nelmts, cd_values);

    // Apply ZFP filter to dataset creation property list
    herr_t status = H5Pset_filter(dcpl_g, H5Z_FILTER_ZFP, H5Z_FLAG_MANDATORY,
                                  cd_nelmts, cd_values);
    if (status < 0) {
        fprintf(stderr, "Error setting ZFP filter\n");
    }
}

void hdf5_io_close_dataset() {
    H5_ASSERT(H5Dclose(dset_g));
    H5_ASSERT(H5Fclose(file_g));
}

void hdf5_io_reopen_dataset() {
    fapl_g = H5Pcreate(H5P_FILE_ACCESS);
    H5_ASSERT(fapl_g);
    H5_ASSERT(H5Pset_fapl_mpio(fapl_g, MPI_COMM_WORLD, MPI_INFO_NULL));
    file_g = H5Fopen("output.h5", H5F_ACC_RDONLY, fapl_g);
    H5_ASSERT(file_g);
    H5_ASSERT(H5Pclose(fapl_g));

    dset_g = H5Dopen(file_g, dataset_name, H5P_DEFAULT);
    H5_ASSERT(dset_g);
}
