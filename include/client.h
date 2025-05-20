#pragma once

#include "utils.h"
#include <stdint.h>
#include <stddef.h>

// Funzioni client
int read_file(const char *filename, uint8_t **buffer, size_t *len);
int encrypt_file_blocks(const uint8_t *input, size_t len, uint64_t key, uint8_t **output, size_t *outlen, int n_threads);
int connect_to_server(const char *ip, int port);
int send_encrypted_message(int sockfd, const uint8_t *ciphertext, size_t cipher_len, uint64_t orig_len, uint64_t key);
int wait_ack(int sockfd);
void ignore_signals();