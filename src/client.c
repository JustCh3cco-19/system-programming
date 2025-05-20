#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include "client.h"

void ignore_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
}

int read_file(const char *filename, uint8_t **buffer, size_t *len) {
    FILE *f = fopen(filename, "rb");
    if (!f) return perror("fopen"), -1;
    fseek(f, 0, SEEK_END);
    long filelen = ftell(f);
    if (filelen <= 0) return fprintf(stderr, "File vuoto o non leggibile\n"), -1;
    *len = (size_t)filelen;
    fseek(f, 0, SEEK_SET);
    *buffer = malloc(*len);
    if (!*buffer) return perror("malloc"), -1;
    if (fread(*buffer, 1, *len, f) != *len) return perror("fread"), -1;
    fclose(f);
    return 0;
}

typedef struct {
    const uint8_t *input;
    uint8_t *output;
    uint64_t key;
    size_t start;
    size_t end;
} encrypt_args_t;

static void *encrypt_block_thread(void *arg) {
    encrypt_args_t *args = arg;
    for (size_t i = args->start; i < args->end; i += BLOCK_SIZE) {
        size_t rem = BLOCK_SIZE;
        if (i + rem > args->end) rem = args->end - i;
        uint64_t block = 0;
        memcpy(&block, args->input + i, rem);
        block ^= args->key;
        memcpy(args->output + i, &block, BLOCK_SIZE);
    }
    return NULL;
}

int encrypt_file_blocks(const uint8_t *input, size_t len, uint64_t key, uint8_t **output, size_t *outlen, int n_threads) {
    size_t padded_len = ((len + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    *output = calloc(1, padded_len);
    *outlen = padded_len;

    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    encrypt_args_t *args = malloc(n_threads * sizeof(encrypt_args_t));
    size_t chunk = padded_len / n_threads;

    for (int t = 0; t < n_threads; ++t) {
        args[t].input = input;
        args[t].output = *output;
        args[t].key = key;
        args[t].start = t * chunk;
        args[t].end = (t == n_threads - 1) ? padded_len : (t + 1) * chunk;
        pthread_create(&threads[t], NULL, encrypt_block_thread, &args[t]);
    }

    for (int t = 0; t < n_threads; ++t) pthread_join(threads[t], NULL);

    free(threads); free(args);
    return 0;
}

int connect_to_server(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); exit(1);
    }

    return sockfd;
}

int send_encrypted_message(int sockfd, const uint8_t *ciphertext, size_t cipher_len, uint64_t orig_len, uint64_t key) {
    uint64_t L_net = htobe64(orig_len), K_net = htobe64(key);
    if (write(sockfd, &L_net, 8) != 8 || write(sockfd, &K_net, 8) != 8) return -1;
    if (write(sockfd, ciphertext, cipher_len) != cipher_len) return -1;
    return 0;
}

int wait_ack(int sockfd) {
    char ack;
    return read(sockfd, &ack, 1) != 1 ? -1 : 0;
}