#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include "client.h" // Assicurati che utils.h sia incluso tramite client.h o direttamente se necessario
// utils.h definisce BLOCK_SIZE

void ignore_signals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
}

int read_file(const char *filename, uint8_t **buffer, size_t *len) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long filelen = ftell(f);
    if (filelen < 0) { 
        perror("ftell");
        fclose(f);
        return -1;
    }
    if (filelen == 0) {
        // Gestione file vuoto: può essere considerato un errore o meno a seconda dei requisiti.
        // Per ora, lo trattiamo come un caso valido che produce 0 byte di output.
        // Il client invierà orig_len = 0, e cipher_len sarà paddato a BLOCK_SIZE (o 0 se BLOCK_SIZE è 0).
        *len = 0;
        *buffer = NULL; // Nessun buffer allocato per file vuoto
        fclose(f);
        return 0; 
    }
    *len = (size_t)filelen;
    fseek(f, 0, SEEK_SET);
    *buffer = malloc(*len);
    if (!*buffer) {
        perror("malloc for read_file buffer");
        fclose(f);
        return -1;
    }
    if (fread(*buffer, 1, *len, f) != *len) {
        perror("fread");
        free(*buffer);
        *buffer = NULL;
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

typedef struct {
    const uint8_t *input;
    size_t original_input_len; 
    uint8_t *output;
    uint64_t key;
    size_t start_output_idx; 
    size_t end_output_idx;   
} encrypt_args_t;

static void *encrypt_block_thread(void *arg) {
    encrypt_args_t *args = (encrypt_args_t *)arg;
    for (size_t i = args->start_output_idx; i < args->end_output_idx; i += BLOCK_SIZE) {
        uint64_t current_block_data = 0; 
        size_t bytes_to_copy_from_input = 0;

        if (args->input != NULL && i < args->original_input_len) {
            bytes_to_copy_from_input = (args->original_input_len - i < BLOCK_SIZE) ? (args->original_input_len - i) : BLOCK_SIZE;
            memcpy(&current_block_data, args->input + i, bytes_to_copy_from_input);
        }
        
        current_block_data ^= args->key; 
        memcpy(args->output + i, &current_block_data, BLOCK_SIZE); 
    }
    return NULL;
}

int encrypt_file_blocks(const uint8_t *input, size_t len, uint64_t key, uint8_t **output, size_t *outlen, int n_threads) {
    if (n_threads <= 0) {
        fprintf(stderr, "Il numero di thread deve essere positivo.\n");
        return -1;
    }
    if (BLOCK_SIZE == 0) {
        fprintf(stderr, "BLOCK_SIZE non può essere zero.\n");
        return -1;
    }

    // Anche se len è 0, eseguiamo il padding a BLOCK_SIZE per inviare un blocco "vuoto" crittografato.
    // Questo semplifica la logica del server che si aspetta sempre dati in multipli di BLOCK_SIZE.
    size_t padded_len = ((len + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    if (len == 0 && padded_len == 0) { // Se len è 0, padded_len dovrebbe essere BLOCK_SIZE (a meno che BLOCK_SIZE non sia 0)
        padded_len = BLOCK_SIZE;
    }


    *output = calloc(1, padded_len); 
    if (!*output && padded_len > 0) { // calloc(1,0) può restituire NULL legalmente
        perror("calloc for output in encrypt_file_blocks");
        return -1;
    }
    *outlen = padded_len;

    if (padded_len == 0) { // Nessun dato da processare (es. BLOCK_SIZE era 0 e len era 0)
        // Questo non dovrebbe accadere con la logica di padding sopra se BLOCK_SIZE > 0
        return 0; 
    }

    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    if (!threads) {
        perror("malloc for threads");
        if (*output) free(*output);
        *output = NULL;
        return -1;
    }
    encrypt_args_t *args = malloc(n_threads * sizeof(encrypt_args_t));
    if (!args) {
        perror("malloc for thread args");
        free(threads);
        if (*output) free(*output);
        *output = NULL;
        return -1;
    }

    size_t total_blocks = padded_len / BLOCK_SIZE;
    size_t current_block_start_idx = 0; // Indice del blocco di output corrente da assegnare

    for (int t = 0; t < n_threads; ++t) {
        size_t blocks_for_this_thread = total_blocks / n_threads;
        if (t < (int)(total_blocks % n_threads)) { 
            blocks_for_this_thread++;
        }

        args[t].input = input; // input può essere NULL se len è 0
        args[t].original_input_len = len; 
        args[t].output = *output;
        args[t].key = key;
        
        if (blocks_for_this_thread > 0) {
            args[t].start_output_idx = current_block_start_idx * BLOCK_SIZE;
            args[t].end_output_idx = (current_block_start_idx + blocks_for_this_thread) * BLOCK_SIZE;
            current_block_start_idx += blocks_for_this_thread;
        } else {
            // Nessun blocco per questo thread
            args[t].start_output_idx = 0; // Valori sicuri per non creare il thread
            args[t].end_output_idx = 0;
        }
        
        threads[t] = 0; // Inizializza per marcare come non creato/non valido

        if (args[t].start_output_idx < args[t].end_output_idx) { // Crea thread solo se ha lavoro
            if (pthread_create(&threads[t], NULL, encrypt_block_thread, &args[t]) != 0) {
                perror("pthread_create");
                // Gestione errore: join dei thread creati finora e free
                for (int k = 0; k < t; ++k) {
                    if (threads[k] != 0) pthread_join(threads[k], NULL);
                }
                free(args);
                free(threads);
                if (*output) free(*output);
                *output = NULL;
                return -1;
            }
        }
    }

    int result = 0;
    for (int t = 0; t < n_threads; ++t) {
        if (threads[t] != 0) { // Join solo se il thread è stato effettivamente creato
            if (pthread_join(threads[t], NULL) != 0) {
                perror("pthread_join");
                result = -1; // Segnala errore, ma continua a fare join sugli altri se possibile
            }
        }
    }

    free(threads);
    free(args);

    if (result != 0 && *output) { // Se c'è stato un errore nel join, libera l'output
        free(*output);
        *output = NULL;
    }
    
    return result;
}

int connect_to_server(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1; 
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed or invalid IP address");
        close(sockfd);
        return -1; 
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect to server failed");
        close(sockfd);
        return -1; 
    }
    printf("Connesso al server %s:%d\n", ip, port);
    return sockfd;
}

int send_encrypted_message(int sockfd, const uint8_t *ciphertext, size_t cipher_len, uint64_t orig_len, uint64_t key) {
    uint64_t L_net = htobe64(orig_len);
    uint64_t K_net = htobe64(key); 

    if (write(sockfd, &L_net, sizeof(L_net)) != sizeof(L_net)) {
        perror("write L_net failed");
        return -1;
    }
    if (write(sockfd, &K_net, sizeof(K_net)) != sizeof(K_net)) {
        perror("write K_net failed");
        return -1;
    }
    // Invia ciphertext solo se cipher_len > 0
    // Se cipher_len è 0, significa che orig_len era 0 e BLOCK_SIZE era anche 0 (caso anomalo)
    // o che il padding ha prodotto 0 (es. len=0, padded_len=0 se BLOCK_SIZE=0).
    // Con la logica attuale, se len=0, padded_len=BLOCK_SIZE, quindi cipher_len sarà BLOCK_SIZE.
    if (cipher_len > 0) {
        if (write(sockfd, ciphertext, cipher_len) != (ssize_t)cipher_len) {
            perror("write ciphertext failed");
            return -1;
        }
    }
    printf("Messaggio crittografato inviato (%zu bytes, lunghezza originale %lu, chiave %lx)\n", cipher_len, orig_len, key);
    return 0;
}

int wait_ack(int sockfd) {
    char ack_buffer[2]; 
    ssize_t n = read(sockfd, ack_buffer, 1);
    if (n < 0) {
        perror("read ack failed");
        return -1;
    }
    if (n == 0) {
        fprintf(stderr, "Connessione chiusa dal server prima di ricevere ACK\n");
        return -1;
    }
    if (ack_buffer[0] == 'A') { 
        printf("ACK ricevuto dal server.\n");
        return 0;
    } else {
        fprintf(stderr, "ACK non valido ricevuto: %c (valore atteso: A)\n", ack_buffer[0]);
        return -1;
    }
}