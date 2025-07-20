#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"
// Include all H5Z-ZFP headers you have
#include "H5Zzfp.h"
#include "H5Zzfp_lib.h"
#include "H5Zzfp_plugin.h"
#include "H5Zzfp_props.h"
#include "H5Zzfp_version.h"

#define H5Z_FILTER_ZFP 32013
#define DATASET_NAME "dataset"

int main() {
    // Dataset parameters
    const hsize_t data_size = 4096;
    const hsize_t chunk_size = 4096;

    // Allocate data buffer and fill with some data
    float *data = (float*)malloc(data_size * sizeof(float));
    for (hsize_t i = 0; i < data_size; i++) {
        data[i] = (float)i;
    }

    // Initialize HDF5
    H5open();
    H5Z_zfp_initialize();

    // Check if ZFP filter is available
    if (!H5Zfilter_avail(H5Z_FILTER_ZFP)) {
        fprintf(stderr, "ZFP filter not available\n");
        free(data);
        return 1;
    }

    // Create a new file
    hid_t file = H5Fcreate("zfp_example.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0) {
        fprintf(stderr, "Failed to create file\n");
        free(data);
        return 1;
    }

    // Create dataspace
    hid_t space = H5Screate_simple(1, &data_size, NULL);

    // Create dataset creation property list and set chunking
    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(dcpl, 1, &chunk_size);

    unsigned int cd_values[6];
    size_t cd_nelmts = 6;
    double rate = 8.0; // compression rate (bits/value)

    H5Pset_zfp_rate_cdata(rate, cd_nelmts, cd_values);

    herr_t ret = H5Pset_filter(dcpl, H5Z_FILTER_ZFP, H5Z_FLAG_MANDATORY, cd_nelmts, cd_values);
    if (ret < 0) {
        fprintf(stderr, "Failed to set ZFP filter\n");
        H5Pclose(dcpl);
        H5Sclose(space);
        H5Fclose(file);
        free(data);
        return 1;
    }

    // Create the dataset with compression
    hid_t dset = H5Dcreate(file, DATASET_NAME, H5T_NATIVE_FLOAT, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
    if (dset < 0) {
        fprintf(stderr, "Failed to create dataset\n");
        H5Pclose(dcpl);
        H5Sclose(space);
        H5Fclose(file);
        free(data);
        return 1;
    }

    // Write data
    ret = H5Dwrite(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (ret < 0) {
        fprintf(stderr, "Failed to write data\n");
    }

    // Cleanup
    H5Dclose(dset);
    H5Pclose(dcpl);
    H5Sclose(space);
    H5Fclose(file);
    free(data);
    H5close();

    printf("ZFP compressed dataset created successfully.\n");

    return 0;
}
