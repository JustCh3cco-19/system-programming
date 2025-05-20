#pragma once

#include "utils.h"
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

typedef struct {
    const uint8_t *input;
    uint8_t *output;
    uint64_t key;
    size_t start;
    size_t end;
} decrypt_args_t;

extern pthread_mutex_t file_counter_mutex;

int decrypt_blocks(const uint8_t *input, size_t len, uint64_t key, uint8_t *output, int n_threads);