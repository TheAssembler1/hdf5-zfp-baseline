#include "pdc_io_impl.h"

#include "pdc.h"

pdcid_t pdc_id_g;

void pdc_io_init() {
    pdc_id_g = PDCinit("pdc");
    PDC_ZERO_ASSERT(pdc_id_g);
}

void pdc_io_deinit() {
    PDC_NEG_ASSERT(PDCclose(pdc_id_g));
}

void pdc_io_init_dataset(int nprocs, int chunks_per_rank) {

}

void pdc_io_create_dataset() {

}

void pdc_io_enable_compression_on_dataset() {

}

void pdc_io_write_chunk(float* buffer, bool collective_io, int rank, int chunks_per_rank, int chunk) {

}

void pdc_io_read_chunk(float* buffer, bool collective_io, int rank, int chunks_per_rank, int chunk) {

}

void pdc_io_flush() {

}

void pdc_io_close_dataset() {

}

void pdc_io_reopen_dataset() {

}