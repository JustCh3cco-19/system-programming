#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "client.h"

// Funzione eseguita dai thread per cifrare i blocchi
void* encrypt_blocks(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    for (int i = args->start_idx; i < args->end_idx; i++) {
        args->blocks[i] ^= args->key;  // XOR tra blocco e chiave
    }
    return NULL;
}

// Funzione per gestire la conversione in network byte order (64-bit)
u64 htonll(u64 val) {
    #if __BYTE_ORDER == __BIG_ENDIAN
        return val;
    #else
        return ((u64)(htonl(val & 0xFFFFFFFF)) << 32) | htonl(val >> 32);
    #endif
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <filename> <key> <parallelism> <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    u64 key = strtoull(argv[2], NULL, 10);
    int p = atoi(argv[3]);
    char *server_ip = argv[4];
    int port = atoi(argv[5]);

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Errore nell'apertura del file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    int block_count = (file_size + 7) / 8;
    u64 *blocks = calloc(block_count, sizeof(u64));
    if (!blocks) {
        perror("Errore nell'allocazione della memoria");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(blocks, 1, file_size, fp);
    fclose(fp);

    if (bytes_read < (block_count * 8)) {
        char *last_block = (char *)&blocks[block_count - 1];
        for (size_t i = bytes_read % 8; i < 8; i++) {
            last_block[i] = '\0';  // Padding con carattere nullo
        }
    }

    pthread_t *threads = malloc(p * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(p * sizeof(ThreadArgs));

    if (!threads || !thread_args) {
        perror("Errore nell'allocazione dei thread");
        free(blocks);
        exit(EXIT_FAILURE);
    }

    int blocks_per_thread = block_count / p;
    int remainder = block_count % p;
    int offset = 0;

    for (int i = 0; i < p; i++) {
        thread_args[i].blocks = blocks;
        thread_args[i].key = key;
        thread_args[i].start_idx = offset;
        offset += blocks_per_thread + (i < remainder ? 1 : 0);
        thread_args[i].end_idx = offset;

        if (pthread_create(&threads[i], NULL, encrypt_blocks, &thread_args[i]) != 0) {
            perror("Errore nella creazione del thread");
            for (int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
            }
            free(threads);
            free(thread_args);
            free(blocks);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < p; i++) {
        pthread_join(threads[i], NULL);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore nella creazione del socket");
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Errore nella connessione al server");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    size_t encrypted_size = block_count * sizeof(u64);
    u64 net_key = htonll(key);
    uint64_t net_file_size = htonll(file_size);

    if (send(sockfd, blocks, encrypted_size, 0) < 0) {
        perror("Errore nell'invio dei dati");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    if (send(sockfd, &net_file_size, sizeof(net_file_size), 0) < 0) {
        perror("Errore nell'invio della dimensione");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    if (send(sockfd, &net_key, sizeof(net_key), 0) < 0) {
        perror("Errore nell'invio della chiave");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    char ack[4];
    int bytes_received = recv(sockfd, ack, sizeof(ack), 0);
    if (bytes_received < 0) {
        perror("Errore nella ricezione dell'acknowledgment");
    } else if (bytes_received == 0) {
        printf("Connessione chiusa dal server\n");
    } else {
        printf("Ricevuto acknowledgment: %.*s\n", bytes_received, ack);
    }

    close(sockfd);
    free(threads);
    free(thread_args);
    free(blocks);

    return 0;
}
