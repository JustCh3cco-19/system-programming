#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>

// =================== CONFIGURATION ===================
#define BLOCK_SIZE 8
#define HEADER_SIZE 4
#define MAX_FILENAME_LEN 256

// =================== DATA STRUCTURES =================
typedef struct {
    int parallelism;      // Grado di parallelismo (p)
    char* output_prefix;  // Prefisso file output (s)
    int max_connections;  // Connessioni simultanee (l)
    int server_port;      // Porta di ascolto (-P)
} server_config_t;

typedef struct {
    int client_sock;
    server_config_t config;
    sem_t* conn_sem;
} client_params_t;

typedef struct {
    const uint8_t* ciphertext;
    const uint8_t* key;
    uint8_t* plaintext;
    int start_idx;
    int end_idx;
} decrypt_task_t;

// =================== FUNCTION DECLARATIONS =================
void parse_arguments(int argc, char* argv[], server_config_t* config);
int create_server_socket(int port);
int accept_connection(int server_sock);
void decrypt_blocks(const uint8_t* ciphertext, const uint8_t* key, 
                   uint8_t* plaintext, int start_idx, int end_idx);
int remove_padding(uint8_t* data, int len);
char* generate_filename(const char* prefix, const char* extension);
int write_decrypted_data(const char* filename, const uint8_t* data, int length);
void* handle_client(void* arg);
void error_exit(const char* format, ...);
void send_ack(int sock, int success);