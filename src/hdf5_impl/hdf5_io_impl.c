#include <stdlib.h>

#include "hdf5_io_impl.h"
#include "../common/common.h"

hid_t dcpl_g = -1;
hid_t dset_g = -1;
hid_t filespace_g = -1;
hid_t file_g = -1;
hid_t space_g = -1;
hid_t fapl_g = -1;


void hdf5_io_init() {
    H5_ASSERT(H5open());

    int filter_avail = H5Zfilter_avail(H5Z_FILTER_ZFP);
    PRINT_RANK0("ZFP filter available? %s\n", filter_avail ? "YES" : "NO");
}

void hdf5_io_deinit() { H5_ASSERT(H5close()); }

void hdf5_io_init_dataset(MPI_Comm comm, int my_rank, int num_ranks, int chunks_per_rank) {
    const hsize_t total_chunks = num_ranks * chunks_per_rank;

    fapl_g = H5Pcreate(H5P_FILE_ACCESS);
    H5_ASSERT(H5Pset_fapl_mpio(fapl_g, MPI_COMM_WORLD, MPI_INFO_NULL));

    file_g = H5Fcreate("output.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_g);
    H5_ASSERT(file_g);
    H5Pclose(fapl_g);

    hsize_t dims[2] = {total_chunks * ELEMENTS_PER_CHUNK, ELEMENTS_PER_CHUNK};
    hsize_t chunk_dims[2] = {ELEMENTS_PER_CHUNK, ELEMENTS_PER_CHUNK};

    space_g = H5Screate_simple(2, dims, NULL);

    dcpl_g = H5Pcreate(H5P_DATASET_CREATE);
    H5_ASSERT(dcpl_g);
    H5_ASSERT(H5Pset_chunk(dcpl_g, 2, chunk_dims));
}

void hdf5_io_create_dataset() {
    dset_g = H5Dcreate(file_g, DATASET_NAME, H5T_NATIVE_FLOAT, space_g,
                       H5P_DEFAULT, dcpl_g, H5P_DEFAULT);
    H5_ASSERT(dset_g);
    H5_ASSERT(H5Pclose(dcpl_g));
    H5_ASSERT(H5Sclose(space_g));
}

void hdf5_io_write_chunk(float *buffer, bool collective_io, int rank,
                         int chunks_per_rank, int chunk, MPI_Comm comm) {
    hsize_t offset[2] = {(rank * chunks_per_rank + chunk) * ELEMENTS_PER_CHUNK,
                         0};
    hsize_t size[2] = {ELEMENTS_PER_CHUNK, ELEMENTS_PER_CHUNK};

    filespace_g = H5Dget_space(dset_g);
    H5_ASSERT(filespace_g);
    H5_ASSERT(H5Sselect_hyperslab(filespace_g, H5S_SELECT_SET, offset, NULL,
                                  size, NULL));

    hid_t memspace = H5Screate_simple(2, size, NULL);
    H5_ASSERT(memspace);

    hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
    H5_ASSERT(dxpl);

    if (collective_io)
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE));
    else
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_INDEPENDENT));

    H5_ASSERT(H5Dwrite(dset_g, H5T_NATIVE_FLOAT, memspace, filespace_g, dxpl,
                       buffer));

    H5_ASSERT(H5Pclose(dxpl));
    H5_ASSERT(H5Sclose(memspace));
    H5_ASSERT(H5Sclose(filespace_g));
}

void hdf5_io_read_chunk(float *read_buf, bool collective_io, int rank,
                        int chunks_per_rank, int chunk, MPI_Comm comm) {
    hsize_t offset[2] = {(rank * chunks_per_rank + chunk) * ELEMENTS_PER_CHUNK,
                         0};
    hsize_t size[2] = {ELEMENTS_PER_CHUNK, ELEMENTS_PER_CHUNK};

    filespace_g = H5Dget_space(dset_g);
    H5_ASSERT(filespace_g);
    H5_ASSERT(H5Sselect_hyperslab(filespace_g, H5S_SELECT_SET, offset, NULL,
                                  size, NULL));

    hid_t memspace = H5Screate_simple(2, size, NULL);
    H5_ASSERT(memspace);

    hid_t dxpl = H5Pcreate(H5P_DATASET_XFER);
    H5_ASSERT(dxpl);

    if (collective_io)
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE));
    else
        H5_ASSERT(H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_INDEPENDENT));

    H5_ASSERT(H5Dread(dset_g, H5T_NATIVE_FLOAT, memspace, filespace_g, dxpl,
                      read_buf));

    H5_ASSERT(H5Pclose(dxpl));
    H5_ASSERT(H5Sclose(memspace));
    H5_ASSERT(H5Sclose(filespace_g));
}

void hdf5_io_flush() { H5_ASSERT(H5Fflush(file_g, H5F_SCOPE_GLOBAL)); }

void hdf5_io_enable_compression_on_dataset() {
    unsigned int cd_values[6];
    size_t cd_nelmts = 6;
    double precision = 0.01;  // target max error (or accuracy) you want

    // Encode precision mode parameters into client data array
    H5Pset_zfp_precision_cdata(precision, cd_nelmts, cd_values);

    // Set the ZFP filter on the dataset creation property list
    herr_t status = H5Pset_filter(dcpl_g, H5Z_FILTER_ZFP, H5Z_FLAG_MANDATORY,
                                  cd_nelmts, cd_values);
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

    dset_g = H5Dopen(file_g, DATASET_NAME, H5P_DEFAULT);
    H5_ASSERT(dset_g);
}