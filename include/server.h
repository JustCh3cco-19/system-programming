#pragma once

#include "utils.h"
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

// Funzioni server
int start_server_socket(int port, int max_conn);
int accept_client(int listenfd);
int receive_encrypted_message(int clientfd, uint8_t **cipher, size_t *cipher_len, uint64_t *orig_len, uint64_t *key);
int send_ack(int clientfd);
int write_file_with_prefix(const char *prefix, const uint8_t *data, size_t len);
void ignore_signals();