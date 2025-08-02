#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

char *random_string(uint32_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789";
    char *buffer = NULL;

    buffer = malloc(length + 1);
    if (!buffer) return NULL;

    for (uint32_t i = 0; i < length; i++) {
        int key =
            rand() % (int) (sizeof(charset) - 1); // -1 to exclude null byte
        buffer[i] = charset[key];
    }

    buffer[length] = '\0';
    return buffer;
}
