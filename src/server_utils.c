#include "server.h"
#include <signal.h>
#include <pthread.h>

// Implementazione delle funzioni principali (senza main)

void error_exit(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void parse_arguments(int argc, char* argv[], server_config_t* config) {
    if (argc != 9) {
        fprintf(stderr, "Usage: %s -p <parallelism> -s <prefix> -l <max_connections> -P <port>\n", 
                argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i += 2) {
        if (!strcmp(argv[i], "-p")) {
            config->parallelism = atoi(argv[i+1]);
        } else if (!strcmp(argv[i], "-s")) {
            config->output_prefix = argv[i+1];
        } else if (!strcmp(argv[i], "-l")) {
            config->max_connections = atoi(argv[i+1]);
        } else if (!strcmp(argv[i], "-P")) {
            config->server_port = atoi(argv[i+1]);
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    if (config->parallelism <= 0 || config->max_connections <= 0 || config->server_port <= 0) {
        fprintf(stderr, "Invalid parameters: values must be positive\n");
        exit(EXIT_FAILURE);
    }
}

int create_server_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        error_exit("Socket creation failed: %s\n", strerror(errno));
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        error_exit("Bind failed: %s\n", strerror(errno));
    }

    if (listen(sock, SOMAXCONN) < 0) {
        error_exit("Listen failed: %s\n", strerror(errno));
    }

    return sock;
}

int accept_connection(int server_sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_sock < 0) {
        error_exit("Accept failed: %s\n", strerror(errno));
    }
    
    return client_sock;
}

void decrypt_blocks(const uint8_t* ciphertext, const uint8_t* key, 
                   uint8_t* plaintext, int start_idx, int end_idx) {
    for (int i = start_idx; i < end_idx; i++) {
        const uint8_t* ct_pos = ciphertext + i * BLOCK_SIZE;
        uint8_t* pt_pos = plaintext + i * BLOCK_SIZE;
        
        for (int j = 0; j < BLOCK_SIZE; j++) {
            pt_pos[j] = ct_pos[j] ^ key[j];
        }
    }
}

int remove_padding(uint8_t* data, int len) {
    if (len <= 0) return 0;
    
    uint8_t pad_len = data[len - 1];
    if (pad_len > BLOCK_SIZE || pad_len == 0) {
        return len;
    }
    
    for (int i = 0; i < pad_len; i++) {
        if (data[len - 1 - i] != pad_len) {
            return len;
        }
    }
    
    return len - pad_len;
}

char* generate_filename(const char* prefix, const char* extension) {
    char* filename = malloc(MAX_FILENAME_LEN);
    if (!filename) {
        error_exit("Memory allocation failed for filename\n");
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);
    
    snprintf(filename, MAX_FILENAME_LEN, "%s_%s.%s", prefix, timestamp, extension);
    return filename;
}

int write_decrypted_data(const char* filename, const uint8_t* data, int length) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        perror("File open failed");
        return -1;
    }

    fwrite(data, 1, length, fp);
    fclose(fp);
    return 0;
}

void* handle_client(void* arg) {
    client_params_t* params = (client_params_t*)arg;
    int client_sock = params->client_sock;
    server_config_t config = params->config;
    sem_t* conn_sem = params->conn_sem;
    free(params);
    
    // Blocca tutti i segnali nel thread
    sigset_t sigset;
    sigfillset(&sigset);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    // Leggi la chiave (8 byte)
    uint8_t key[BLOCK_SIZE];
    ssize_t bytes_read = 0;
    while (bytes_read < BLOCK_SIZE) {
        ssize_t res = read(client_sock, key + bytes_read, BLOCK_SIZE - bytes_read);
        if (res <= 0) {
            send_ack(client_sock, 0);
            close(client_sock);
            sem_post(conn_sem);
            return NULL;
        }
        bytes_read += res;
    }

    // Leggi il numero di blocchi (4 byte)
    uint32_t n_blocks_net;
    bytes_read = 0;
    while (bytes_read < HEADER_SIZE) {
        ssize_t res = read(client_sock, ((char*)&n_blocks_net) + bytes_read, HEADER_SIZE - bytes_read);
        if (res <= 0) {
            send_ack(client_sock, 0);
            close(client_sock);
            sem_post(conn_sem);
            return NULL;
        }
        bytes_read += res;
    }
    int n_blocks = ntohl(n_blocks_net);

    if (n_blocks <= 0) {
        send_ack(client_sock, 0);
        close(client_sock);
        sem_post(conn_sem);
        return NULL;
    }

    // Leggi dati cifrati
    size_t ct_size = n_blocks * BLOCK_SIZE;
    uint8_t* ciphertext = malloc(ct_size);
    if (!ciphertext) {
        send_ack(client_sock, 0);
        close(client_sock);
        sem_post(conn_sem);
        return NULL;
    }

    bytes_read = 0;
    while (bytes_read < ct_size) {
        ssize_t res = read(client_sock, ciphertext + bytes_read, ct_size - bytes_read);
        if (res <= 0) {
            free(ciphertext);
            send_ack(client_sock, 0);
            close(client_sock);
            sem_post(conn_sem);
            return NULL;
        }
        bytes_read += res;
    }

    // Alloca buffer per dati decrittografati
    uint8_t* pt_buffer = malloc(ct_size);
    if (!pt_buffer) {
        free(ciphertext);
        send_ack(client_sock, 0);
        close(client_sock);
        sem_post(conn_sem);
        return NULL;
    }

    // Calcola numero effettivo di thread da usare
    int num_threads = (config.parallelism > n_blocks) ? n_blocks : config.parallelism;
    pthread_t workers[num_threads];
    int blocks_per_thread = n_blocks / num_threads;
    int extra_blocks = n_blocks % num_threads;
    int offset = 0;

    // Crea thread worker
    for (int i = 0; i < num_threads; i++) {
        int start_idx = offset;
        int end_idx = offset + blocks_per_thread + (i < extra_blocks ? 1 : 0);
        offset = end_idx;

        decrypt_task_t* task = malloc(sizeof(decrypt_task_t));
        task->ciphertext = ciphertext;
        task->key = key;
        task->plaintext = pt_buffer;
        task->start_idx = start_idx;
        task->end_idx = end_idx;

        if (pthread_create(&workers[i], NULL, (void* (*)(void*))decrypt_blocks, task) != 0) {
            // Gestione errore nella creazione del thread
            for (int j = 0; j < i; j++) {
                pthread_cancel(workers[j]);
                pthread_join(workers[j], NULL);
            }
            free(ciphertext);
            free(pt_buffer);
            send_ack(client_sock, 0);
            close(client_sock);
            sem_post(conn_sem);
            return NULL;
        }
    }

    // Attende la fine di tutti i thread
    for (int i = 0; i < num_threads; i++) {
        pthread_join(workers[i], NULL);
    }

    // Rimuove il padding
    int pt_len = ct_size;
    pt_len = remove_padding(pt_buffer, pt_len);

    // Genera nome file e scrive dati
    char* filename = generate_filename(config.output_prefix, "dec");
    if (write_decrypted_data(filename, pt_buffer, pt_len) != 0) {
        free(filename);
        free(ciphertext);
        free(pt_buffer);
        send_ack(client_sock, 0);
        close(client_sock);
        sem_post(conn_sem);
        return NULL;
    }

    // Invia ACK al client
    send_ack(client_sock, 1);

    // Cleanup
    free(filename);
    free(ciphertext);
    free(pt_buffer);
    close(client_sock);
    sem_post(conn_sem);
    return NULL;
}

void send_ack(int sock, int success) {
    uint8_t response = success ? 1 : 0;
    write(sock, &response, 1);
}