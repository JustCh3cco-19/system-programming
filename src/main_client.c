#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h> // <-- AGGIUNTO: per la funzione close()
#include "client.h" // Include client functions and utils.h

// client.out <server_ip> <server_port> <num_threads> <key_hex> <filename>
int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Utilizzo: %s <server_ip> <server_port> <num_threads> <key_hex> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    uint64_t key = strtoull(argv[4], NULL, 16); // Interpreta la chiave come esadecimale
    const char *filename = argv[5];

    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Porta del server non valida.\n");
        return EXIT_FAILURE;
    }
    if (num_threads <= 0) {
        fprintf(stderr, "Il numero di thread deve essere positivo.\n");
        return EXIT_FAILURE;
    }

    ignore_signals(); // Ignora i segnali come definito in client.c

    uint8_t *file_buffer = NULL;
    size_t file_len = 0;
    printf("Lettura file: %s\n", filename);
    if (read_file(filename, &file_buffer, &file_len) != 0) {
        fprintf(stderr, "Errore nella lettura del file %s\n", filename);
        return EXIT_FAILURE;
    }
    printf("File letto: %zu bytes\n", file_len);

    uint8_t *encrypted_buffer = NULL;
    size_t encrypted_len = 0;
    printf("Crittografia del file con %d thread(s) e chiave 0x%lx...\n", num_threads, key);
    if (encrypt_file_blocks(file_buffer, file_len, key, &encrypted_buffer, &encrypted_len, num_threads) != 0) {
        fprintf(stderr, "Errore durante la crittografia del file.\n");
        free(file_buffer);
        return EXIT_FAILURE;
    }
    printf("File crittografato: %zu bytes (dimensione paddata)\n", encrypted_len);

    int sockfd = connect_to_server(server_ip, server_port);
    if (sockfd < 0) {
        fprintf(stderr, "Impossibile connettersi al server %s:%d.\n", server_ip, server_port);
        free(file_buffer);
        free(encrypted_buffer);
        return EXIT_FAILURE;
    }

    printf("Invio messaggio crittografato al server...\n");
    if (send_encrypted_message(sockfd, encrypted_buffer, encrypted_len, file_len, key) != 0) {
        fprintf(stderr, "Errore durante l'invio del messaggio crittografato.\n");
        close(sockfd); // Chiudi il socket in caso di errore
        free(file_buffer);
        free(encrypted_buffer);
        return EXIT_FAILURE;
    }

    printf("In attesa di ACK dal server...\n");
    if (wait_ack(sockfd) != 0) {
        fprintf(stderr, "Errore durante l'attesa dell'ACK o ACK non valido.\n");
        // Nonostante l'errore nell'ACK, il server potrebbe aver ricevuto i dati.
        // A seconda della logica richiesta, potresti voler considerare questo un fallimento parziale o totale.
    }

    printf("Operazione client completata.\n");

    close(sockfd); // Chiudi il socket dopo l'uso
    free(file_buffer);
    free(encrypted_buffer);

    return EXIT_SUCCESS;
}
