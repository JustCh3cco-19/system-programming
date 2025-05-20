#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "server_utils.h"

void *decrypt_block_thread(void *arg) {
    decrypt_args_t *args = arg;
    for (size_t i = args->start; i < args->end; i += BLOCK_SIZE) {
        size_t rem = BLOCK_SIZE;
        if (i + rem > args->end) rem = args->end - i;
        uint64_t block = 0;
        memcpy(&block, args->input + i, rem);
        block ^= args->key;
        memcpy(args->output + i, &block, rem);
    }
    return NULL;
}

int decrypt_blocks(const uint8_t *input, size_t len, uint64_t key, uint8_t *output, int n_threads) {
    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    decrypt_args_t *args = malloc(n_threads * sizeof(decrypt_args_t));

    size_t chunk = len / n_threads;
    for (int t = 0; t < n_threads; ++t) {
        args[t].input = input;
        args[t].output = output;
        args[t].key = key;
        args[t].start = t * chunk;
        args[t].end = (t == n_threads - 1) ? len : (t + 1) * chunk;
        pthread_create(&threads[t], NULL, decrypt_block_thread, &args[t]);
    }

    for (int t = 0; t < n_threads; ++t) pthread_join(threads[t], NULL);

    free(threads); free(args);
    return 0;
}