#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h>

// Definizione di un tipo per i valori a 64 bit
typedef unsigned long long u64;

// Struttura per passare gli argomenti ai thread
typedef struct {
    u64 *blocks;        // Array di blocchi da elaborare
    u64 key;            // Chiave di cifratura
    int start_idx;      // Indice iniziale nel blocco
    int end_idx;        // Indice finale (esclusivo)
} ThreadArgs;

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

// Funzione principale
int main(int argc, char *argv[]) {
    // Controllo del numero di argomenti
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <filename> <key> <parallelism> <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parsing degli argomenti
    char *filename = argv[1];
    u64 key = strtoull(argv[2], NULL, 10);
    int p = atoi(argv[3]);
    char *server_ip = argv[4];
    int port = atoi(argv[5]);

    // Ignoriamo i segnali specificati
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Apertura e lettura del file
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Errore nell'apertura del file");
        exit(EXIT_FAILURE);
    }

    // Determinare la dimensione del file
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    // Calcolo del numero di blocchi da 64 bit (8 byte)
    int block_count = (file_size + 7) / 8;
    
    // Allocazione della memoria per i blocchi
    u64 *blocks = calloc(block_count, sizeof(u64));
    if (!blocks) {
        perror("Errore nell'allocazione della memoria");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    // Lettura del file nei blocchi
    size_t bytes_read = fread(blocks, 1, file_size, fp);
    fclose(fp);

    // Applicazione del padding se necessario
    if (bytes_read < (block_count * 8)) {
        char *last_block = (char *)&blocks[block_count - 1];
        for (size_t i = bytes_read % 8; i < 8; i++) {
            last_block[i] = '\0';  // Padding con carattere nullo
        }
    }

    // Creazione dei thread
    pthread_t *threads = malloc(p * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(p * sizeof(ThreadArgs));
    
    if (!threads || !thread_args) {
        perror("Errore nell'allocazione dei thread");
        free(blocks);
        exit(EXIT_FAILURE);
    }

    // Distribuzione dei blocchi tra i thread
    int blocks_per_thread = block_count / p;
    int remainder = block_count % p;
    int offset = 0;

    for (int i = 0; i < p; i++) {
        thread_args[i].blocks = blocks;
        thread_args[i].key = key;
        thread_args[i].start_idx = offset;
        
        // Distribuzione equa dei blocchi con resto
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

    // Attesa del completamento dei thread
    for (int i = 0; i < p; i++) {
        pthread_join(threads[i], NULL);
    }

    // Creazione del socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore nella creazione del socket");
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);

    // Connessione al server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Errore nella connessione al server");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    // Invio dei dati al server
    size_t encrypted_size = block_count * sizeof(u64);
    u64 net_key = htonll(key);
    uint64_t net_file_size = htonll(file_size);

    // Invio dei blocchi cifrati
    if (send(sockfd, blocks, encrypted_size, 0) < 0) {
        perror("Errore nell'invio dei dati");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    // Invio della dimensione originale
    if (send(sockfd, &net_file_size, sizeof(net_file_size), 0) < 0) {
        perror("Errore nell'invio della dimensione");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    // Invio della chiave
    if (send(sockfd, &net_key, sizeof(net_key), 0) < 0) {
        perror("Errore nell'invio della chiave");
        close(sockfd);
        free(threads);
        free(thread_args);
        free(blocks);
        exit(EXIT_FAILURE);
    }

    // Ricezione del messaggio di acknowledgment
    char ack[4];
    int bytes_received = recv(sockfd, ack, sizeof(ack), 0);
    if (bytes_received < 0) {
        perror("Errore nella ricezione dell'acknowledgment");
    } else if (bytes_received == 0) {
        printf("Connessione chiusa dal server\n");
    } else {
        printf("Ricevuto acknowledgment: %.*s\n", bytes_received, ack);
    }

    // Pulizia delle risorse
    close(sockfd);
    free(threads);
    free(thread_args);
    free(blocks);
    
    return 0;
}