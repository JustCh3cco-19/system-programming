#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "server.h"
#include "server_utils.h"

pthread_mutex_t file_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void ignore_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
}

int start_server_socket(int port, int max_conn) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, max_conn);
    return sockfd;
}

int accept_client(int listenfd) {
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    return accept(listenfd, (struct sockaddr*)&cli_addr, &len);
}

int receive_encrypted_message(int clientfd, uint8_t **cipher, size_t *cipher_len, uint64_t *orig_len, uint64_t *key) {
    uint64_t L_net, K_net;
    if (read(clientfd, &L_net, 8) != 8 || read(clientfd, &K_net, 8) != 8) return -1;
    *orig_len = be64toh(L_net);
    *key = be64toh(K_net);
    *cipher_len = ((*orig_len + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    *cipher = calloc(1, *cipher_len);
    if (read(clientfd, *cipher, *cipher_len) != *cipher_len) return -1;
    return 0;
}

int send_ack(int clientfd) {
    char ack = 'A';
    return write(clientfd, &ack, 1) == 1 ? 0 : -1;
}

int write_file_with_prefix(const char *prefix, const uint8_t *data, size_t len) {
    static int counter = 0;
    char filename[256];

    pthread_mutex_lock(&file_counter_mutex);
    snprintf(filename, sizeof(filename), "%s_%03d.out", prefix, counter++);
    pthread_mutex_unlock(&file_counter_mutex);

    FILE *f = fopen(filename, "wb");
    fwrite(data, 1, len, f);
    fclose(f);
    printf("Salvato: %s\n", filename);
    return 0;
}