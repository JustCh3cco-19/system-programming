#include "client.h"
#include "utils.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>

static void block_signals(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

void *encrypt_thread(void *arg) {
    struct thread_data *td = arg;
    block_signals();
    for (size_t i = td->start; i < td->end; ++i) {
        td->blocks[i] ^= td->key;
    }
    return NULL;
}

int client_run(const char *filename, key_t key, int p, const char *server_ip, uint16_t port) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    size_t L = file_size;

    size_t n = (L + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint64_t *blocks = calloc(n, sizeof(uint64_t));
    read_n(fd, blocks, L);
    size_t rem = L % BLOCK_SIZE;
    if (rem) memset(((char*)blocks) + L, 0, BLOCK_SIZE - rem);
    close(fd);

    block_signals();

    pthread_t *threads = malloc(p * sizeof(pthread_t));
    struct thread_data *td = malloc(p * sizeof(struct thread_data));
    size_t chunk = (n + p - 1) / p;
    for (int i = 0; i < p; ++i) {
        size_t start = i * chunk, end = start + chunk;
        if (end > n) end = n;
        td[i] = (struct thread_data){ blocks, start, end, key };
        pthread_create(&threads[i], NULL, encrypt_thread, &td[i]);
    }
    for (int i = 0; i < p; ++i) pthread_join(threads[i], NULL);

    signal(SIGPIPE, SIG_IGN);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return 1; }

    uint64_t net_L   = htonll(L);
    uint64_t net_key = htonll(key);
    write_n(sock, &net_L,   sizeof net_L);
    write_n(sock, &net_key, sizeof net_key);
    write_n(sock, blocks, n * sizeof(uint64_t));

    char ack[4] = {0};
    if (read_n(sock, ack, 3) <= 0) fprintf(stderr, "Connection closed by server\n");
    else if (strncmp(ack, "ACK", 3) != 0) fprintf(stderr, "Invalid ACK\n");

    close(sock);
    free(blocks);
    free(threads);
    free(td);
    return 0;
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    int p; key_t key; char *file,*ip; uint16_t port;
    if (getenv("CLI_P"))      p    = atoi(getenv("CLI_P"));
    else if (argc > 3)        p    = atoi(argv[3]);
    else { fprintf(stderr,"Usage...\n"); return 1; }
    if (getenv("CLI_KEY"))    key  = strtoull(getenv("CLI_KEY"),NULL,0);
    else if (argc > 2)        key  = strtoull(argv[2],NULL,0);
    else { fprintf(stderr,"Usage...\n"); return 1; }
    if (argc > 1)             file = argv[1];
    else { fprintf(stderr,"Usage...\n"); return 1; }
    if (getenv("CLI_IP"))     ip   = getenv("CLI_IP");
    else if (argc > 4)        ip   = argv[4];
    else { fprintf(stderr,"Usage...\n"); return 1; }
    if (getenv("CLI_PORT"))   port = atoi(getenv("CLI_PORT"));
    else if (argc > 5)        port = atoi(argv[5]);
    else { fprintf(stderr,"Usage...\n"); return 1; }
    return client_run(file, key, p, ip, port);
}
