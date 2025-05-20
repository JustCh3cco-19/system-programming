#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // Per sockaddr_in, INADDR_ANY, htons, accept, htobe64, be64toh
#include <pthread.h>
#include <signal.h>
#include <errno.h>    // Per perror
#include "server.h"   // Contiene le dichiarazioni delle funzioni del server e utils.h
#include "server_utils.h" // Contiene decrypt_blocks e la sua struct

// Definizione del mutex (già presente nel tuo file originale)
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
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(sockfd);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Accetta connessioni su qualsiasi interfaccia
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, max_conn) < 0) {
        perror("listen failed");
        close(sockfd);
        return -1;
    }
    printf("Server in ascolto sulla porta %d...\n", port);
    return sockfd;
}

int accept_client(int listenfd) {
    struct sockaddr_in cli_addr;
    socklen_t len = sizeof(cli_addr);
    int clientfd = accept(listenfd, (struct sockaddr*)&cli_addr, &len);
    if (clientfd < 0) {
        perror("accept client failed");
        return -1;
    }
    // Stampa l'IP del client connesso (opzionale)
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cli_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Client connesso: %s\n", client_ip);
    return clientfd;
}

int receive_encrypted_message(int clientfd, uint8_t **cipher, size_t *cipher_len, uint64_t *orig_len, uint64_t *key) {
    uint64_t L_net, K_net;
    ssize_t n_read;

    n_read = read(clientfd, &L_net, sizeof(L_net));
    if (n_read != sizeof(L_net)) {
        if (n_read < 0) perror("read L_net failed"); else fprintf(stderr, "read L_net: short read\n");
        return -1;
    }
    n_read = read(clientfd, &K_net, sizeof(K_net));
    if (n_read != sizeof(K_net)) {
         if (n_read < 0) perror("read K_net failed"); else fprintf(stderr, "read K_net: short read\n");
        return -1;
    }

    *orig_len = be64toh(L_net);
    *key = be64toh(K_net); // Il server riceve la chiave, assicurati sia il comportamento desiderato

    if (*orig_len == 0 && BLOCK_SIZE > 0) { // Se il file originale è vuoto, la lunghezza paddata è BLOCK_SIZE
         *cipher_len = BLOCK_SIZE; // Anche un file vuoto viene "paddato" a BLOCK_SIZE per la crittografia/decrittografia
    } else {
        *cipher_len = ((*orig_len + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    }
    
    if (*cipher_len == 0 && *orig_len > 0) { // caso anomalo se blocksize è 0 o orig_len molto grande
        fprintf(stderr, "Cipher length calcolata a 0 per orig_len > 0. Controllare BLOCK_SIZE.\n");
        return -1;
    }
    if (*cipher_len == 0 && *orig_len == 0) { // Se il file è vuoto e blocksize è 0 o non definito
         // Potrebbe essere valido se si vuole inviare 0 bytes per un file vuoto e non fare padding.
         // Ma la logica di padding attuale implica che cipher_len sia almeno BLOCK_SIZE se orig_len > 0 (o anche =0)
         // Se si permette orig_len = 0 e cipher_len = 0, allocare 0 bytes con calloc è ok ma read leggerà 0.
         // Per coerenza con encrypt, se orig_len=0, cipher_len dovrebbe essere BLOCK_SIZE (o 0 se non si fa padding di file vuoti)
         // Attualmente, se orig_len=0, cipher_len = BLOCK_SIZE.
    }


    *cipher = calloc(1, *cipher_len); // calloc è sicuro per cipher_len = 0 (restituisce NULL o un puntatore valido deallocabile)
    if (!*cipher && *cipher_len > 0) { // Controlla solo se cipher_len > 0, perché calloc(1,0) può restituire NULL
        perror("calloc for cipher failed");
        return -1;
    }

    if (*cipher_len > 0) { // Leggi solo se c'è qualcosa da leggere
        n_read = read(clientfd, *cipher, *cipher_len);
        if (n_read != (ssize_t)*cipher_len) {
            if (n_read < 0) perror("read ciphertext failed"); else fprintf(stderr, "read ciphertext: short read (expected %zu, got %zd)\n", *cipher_len, n_read);
            free(*cipher);
            *cipher = NULL;
            return -1;
        }
    }
    printf("Messaggio crittografato ricevuto (%zu bytes, lunghezza originale %lu, chiave %lx)\n", *cipher_len, *orig_len, *key);
    return 0;
}

int send_ack(int clientfd) {
    char ack = 'A'; // ACK standard
    if (write(clientfd, &ack, 1) != 1) {
        perror("send ack failed");
        return -1;
    }
    printf("ACK inviato al client.\n");
    return 0;
}

int write_file_with_prefix(const char *prefix, const uint8_t *data, size_t len) {
    // La gestione del contatore statico con mutex è già nel tuo codice originale ed è corretta
    // per accessi multithread a questa funzione (se il server gestisce più client concorrentemente).
    static int counter = 0;
    char filename[256]; // Assicurati che sia sufficientemente grande

    pthread_mutex_lock(&file_counter_mutex);
    snprintf(filename, sizeof(filename), "%s_%03d.out", prefix, counter++);
    pthread_mutex_unlock(&file_counter_mutex);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen for output file failed");
        return -1;
    }

    if (len > 0) { // Scrivi solo se ci sono dati da scrivere
        if (fwrite(data, 1, len, f) != len) {
            perror("fwrite to output file failed");
            fclose(f);
            // Potresti voler rimuovere il file parzialmente scritto qui
            // remove(filename);
            return -1;
        }
    }

    fclose(f);
    printf("Dati salvati in: %s (%zu bytes)\n", filename, len);
    return 0;
}