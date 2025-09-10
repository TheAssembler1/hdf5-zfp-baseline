#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define CSV_FILENAME "output.csv"

#define MAX_CONFIG_WORKLOADS 255
#define MAX_CONFIG_IO_PARTICIPATIONS 2
#define MAX_CONFIG_STRING_SIZE 256

typedef struct config_workload_t {
    char name[MAX_CONFIG_STRING_SIZE];
    char params[MAX_CONFIG_STRING_SIZE];
    char implementation[MAX_CONFIG_STRING_SIZE];
    char io_filter[MAX_CONFIG_STRING_SIZE];
    uint32_t num_io_participations;
    char io_participations[MAX_CONFIG_IO_PARTICIPATIONS]
                          [MAX_CONFIG_STRING_SIZE];
} config_workload_t;

// this should mirror JSON
typedef struct config_t {
    uint32_t num_workloads;
    config_workload_t workloads[MAX_CONFIG_WORKLOADS];
    uint64_t chunk_size_bytes;
    uint64_t chunks_per_rank;
    bool validate_read;
} config_t;

config_t *init_config(char *config_path);

#endif