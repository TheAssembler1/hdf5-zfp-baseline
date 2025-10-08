#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <mpi.h>
#include <string.h>

#include "config.h"
#include "common.h"
#include "log.h"
#include "json-c/json.h"

/**
 * Example configuration JSON:
 * {
 *     "workloads": [
 *         {
 *             "name": "HDF5",
 *             "implementation": "hdf5",
 *             "io_participations": [
 *                 "collective",
 *                 "independent"
 *             ],
 *             "io_type": "read",
 *             "filter": "raw",
 *             "params": "none"
 *         }
 *     ]
 *     "chunk_size_bytes": 64000,
 *     "chunks_per_rank": 1,
 * }
 */

#define CONFIG_ERROR_PREFIX "[CONFIG_ERROR] "

static struct array_list *validate_json_array(struct json_object *json_obj,
                                              char *arr_name,
                                              uint32_t min_length,
                                              uint32_t max_length) {
    struct json_object *arr;

    ASSERT(json_object_object_get_ex(json_obj, arr_name, &arr),
           CONFIG_ERROR_PREFIX "Failed to find %s in JSON config\n", arr_name);
    ASSERT(json_object_get_type(arr) == json_type_array,
           CONFIG_ERROR_PREFIX "%s must be an array\n", arr_name);
    uint32_t workloads_length = json_object_array_length(arr);
    ASSERT(workloads_length >= min_length && workloads_length <= max_length,
           CONFIG_ERROR_PREFIX "%s length must be %d <= length <= %d\n",
           arr_name, min_length, max_length);

    return json_object_get_array(arr);
}

static const char *validate_json_string(struct json_object *json_obj,
                                        char *str_name, uint32_t max_length) {
    struct json_object *str_json_obj;

    ASSERT(json_object_object_get_ex(json_obj, str_name, &str_json_obj),
           CONFIG_ERROR_PREFIX "Failed to find %s in JSON config\n", str_name);
    ASSERT(json_object_get_type(str_json_obj) == json_type_string,
           CONFIG_ERROR_PREFIX "%s must be a string\n", str_name);
    const char *res = json_object_get_string(str_json_obj);
    ASSERT(strlen(res) <= max_length, "Strings must be length <= %u",
           max_length);

    return res;
}

static const char *validate_json_string_raw(struct json_object *json_obj,
                                            uint32_t max_length) {
    ASSERT(json_object_get_type(json_obj) == json_type_string,
           CONFIG_ERROR_PREFIX "Must be a string\n");
    const char *res = json_object_get_string(json_obj);
    ASSERT(strlen(res) <= max_length, "Strings must be length <= %u",
           max_length);

    return res;
}

static int validate_json_number(struct json_object *json_obj, char *num_name) {
    struct json_object *num_json_obj;

    ASSERT(json_object_object_get_ex(json_obj, num_name, &num_json_obj),
           CONFIG_ERROR_PREFIX "Failed to find %s in JSON config\n", num_name);
    ASSERT(json_object_get_type(num_json_obj) == json_type_int,
           CONFIG_ERROR_PREFIX "%s must be a number\n", num_name);

    return json_object_get_int(num_json_obj);
}

static bool validate_json_bool(struct json_object *json_obj, char *bool_name) {
    struct json_object *bool_json_obj;

    ASSERT(json_object_object_get_ex(json_obj, bool_name, &bool_json_obj),
           CONFIG_ERROR_PREFIX "Failed to find %s in JSON config\n", bool_name);
    ASSERT(json_object_get_type(bool_json_obj) == json_type_boolean,
           CONFIG_ERROR_PREFIX "%s must be a number\n", bool_name);

    return json_object_get_boolean(bool_json_obj);
}

config_t *init_config(char *config_path) {
    config_t *config = malloc(sizeof(config_t));

    // make sure we can access file
    if (access(config_path, R_OK) != 0)
        ASSERT(false, "Failed to access config file %s\n", config_path);

    // open file
    int fd = open(config_path, O_RDONLY);
    ASSERT(fd != -1, "Failed to open config file\n");

    // get file size
    off_t file_size_bytes = lseek(fd, 0, SEEK_END);
    ASSERT(file_size_bytes != -1, "Failed to lseek config file\n");
    off_t lseek_stat = lseek(fd, 0, SEEK_SET);
    ASSERT(lseek_stat != -1, "Failed to lseek reset config file\n");

    // read file into buf
    char *file_buf = malloc(file_size_bytes + 1);
    ssize_t read_stat = read(fd, file_buf, file_size_bytes);
    ASSERT(read_stat != -1, "Failed to read config file\n");
    if (read_stat != file_size_bytes)
        ASSERT(false,
               "Failed to read entire file, received %ld, expected %ld\n",
               read_stat, file_size_bytes);
    file_buf[file_size_bytes] = '\0';

    // close file
    ASSERT(close(fd) == 0, "Failed to close config file\n");

    // parse json string
    struct json_object *json_obj = json_tokener_parse(file_buf);
    ASSERT(json_obj != NULL, "Failed to parse json string\n");
    PRINT_RANK0("%s\n", json_object_to_json_string_ext(
                            json_obj, JSON_C_TO_STRING_PRETTY));

    /**
     * json_object_object_get_ex(obj, key, &value) — lookup by key.
     * json_object_get_type(obj) — check if it’s an array.
     * json_object_array_length(array) — get length.
     * json_object_array_get_idx(array, index) — get element.
     */

    // get workloads array
    struct array_list *workloads_json_obj =
        validate_json_array(json_obj, "workloads", 1, MAX_CONFIG_WORKLOADS);
    uint32_t workloads_length = array_list_length(workloads_json_obj);
    config->num_workloads = workloads_length;
    for (uint32_t i = 0; i < workloads_length; i++) {
        struct json_object *workload =
            array_list_get_idx(workloads_json_obj, i);

        // validate name & implementation
        const char *workload_name =
            validate_json_string(workload, "name", MAX_CONFIG_STRING_SIZE);
        strcpy(config->workloads[i].name, workload_name);
        const char *workload_implementation = validate_json_string(
            workload, "implementation", MAX_CONFIG_STRING_SIZE);
        strcpy(config->workloads[i].implementation, workload_implementation);
        const char *workload_io_filter =
            validate_json_string(workload, "filter", MAX_CONFIG_STRING_SIZE);
        const char *workload_io_type =
            validate_json_string(workload, "io_type", MAX_CONFIG_STRING_SIZE);
        strcpy(config->workloads[i].io_type, workload_io_type);
        const char *params =
            validate_json_string(workload, "params", MAX_CONFIG_STRING_SIZE);
        strcpy(config->workloads[i].params, params);

        // validate and pull out io participations
        struct array_list *workload_io_participations = validate_json_array(
            workload, "io_participations", 1, MAX_CONFIG_IO_PARTICIPATIONS);
        uint32_t workload_io_participations_length =
            array_list_length(workload_io_participations);
        config->workloads[i].num_io_participations =
            workload_io_participations_length;
        for (uint32_t j = 0; j < workload_io_participations_length; j++) {
            struct json_object *workload_io_participation_json_obj =
                array_list_get_idx(workload_io_participations, j);
            const char *workload_io_participation = validate_json_string_raw(
                workload_io_participation_json_obj, MAX_CONFIG_STRING_SIZE);
            strcpy(config->workloads[i].io_participations[j],
                   workload_io_participation);
        }
    }
    // get total bytes per chunk & validate read & chunks_per_rank
    config->chunk_size_bytes =
        validate_json_number(json_obj, "chunk_size_bytes");
    config->chunks_per_rank = validate_json_number(json_obj, "chunks_per_rank");

    /**
     * FIXME: at this point we have pulled out correct values from json
     * we no need to verify things such as workload names being unique
     * and string values being valid.
     */
    json_object_put(json_obj);

    // Default compression to false

    return config;
}
