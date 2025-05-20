#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Per close()
#include <stdint.h>
#include "server.h"       // Include server functions, utils.h
#include "server_utils.h" // Include decrypt_blocks

// server.out <port> <num_threads_decrypt> <output_prefix>
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Utilizzo: %s <port> <num_threads_decrypt> <output_prefix>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int num_threads_decrypt = atoi(argv[2]);
    const char *output_prefix = argv[3];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Porta non valida.\n");
        return EXIT_FAILURE;
    }
    if (num_threads_decrypt <= 0) {
        fprintf(stderr, "Il numero di thread per la decrittografia deve essere positivo.\n");
        return EXIT_FAILURE;
    }

    ignore_signals(); // Ignora segnali come definito in server.c

    int listenfd = start_server_socket(port, 10); // Max 10 connessioni in coda
    if (listenfd < 0) {
        fprintf(stderr, "Impossibile avviare il server socket sulla porta %d.\n", port);
        return EXIT_FAILURE;
    }

    // Loop principale per accettare client (gestisce un client alla volta per semplicità)
    // Per gestire client concorrenti, dovresti usare thread o processi figli.
    while (1) {
        printf("In attesa di una nuova connessione client...\n");
        int clientfd = accept_client(listenfd);
        if (clientfd < 0) {
            fprintf(stderr, "Errore nell'accettare un client, continuo...\n");
            continue; // Continua ad ascoltare per altri client
        }

        uint8_t *cipher_buffer = NULL;
        size_t cipher_len = 0;
        uint64_t original_file_len = 0;
        uint64_t key = 0;

        if (receive_encrypted_message(clientfd, &cipher_buffer, &cipher_len, &original_file_len, &key) != 0) {
            fprintf(stderr, "Errore nella ricezione del messaggio crittografato.\n");
            free(cipher_buffer); // cipher_buffer potrebbe essere stato allocato parzialmente
            close(clientfd);
            continue;
        }

        uint8_t *decrypted_buffer = NULL;
        if (cipher_len > 0) { // Decrittografa solo se c'è qualcosa
            decrypted_buffer = malloc(cipher_len); // Il buffer decrittografato avrà la stessa dimensione paddata
            if (!decrypted_buffer) {
                perror("malloc for decrypted_buffer");
                free(cipher_buffer);
                close(clientfd);
                // Potresti voler inviare un NACK o semplicemente chiudere
                continue;
            }

            printf("Decrittografia dei dati (%zu bytes) con %d thread(s)...\n", cipher_len, num_threads_decrypt);
            if (decrypt_blocks(cipher_buffer, cipher_len, key, decrypted_buffer, num_threads_decrypt) != 0) {
                fprintf(stderr, "Errore durante la decrittografia.\n");
                free(cipher_buffer);
                free(decrypted_buffer);
                close(clientfd);
                // Potresti voler inviare un NACK o semplicemente chiudere
                continue;
            }
        } else if (original_file_len == 0 && cipher_len == 0) { // File originale vuoto, nessun dato cifrato
            printf("Ricevuto file vuoto.\n");
            // decrypted_buffer rimane NULL
        }


        // Scrive i dati decrittografati (solo la lunghezza originale)
        // Nota: decrypted_buffer contiene dati fino a cipher_len (paddata).
        // Devi scrivere solo original_file_len byte nel file di output.
        printf("Scrittura del file decrittografato...\n");
        if (write_file_with_prefix(output_prefix, decrypted_buffer, original_file_len) != 0) {
            fprintf(stderr, "Errore durante la scrittura del file decrittografato.\n");
            // Nonostante l'errore di scrittura, l'ACK viene comunque inviato.
            // Valuta se questo è il comportamento desiderato.
        }

        free(cipher_buffer);
        free(decrypted_buffer); // Libera anche se era NULL (sicuro)

        printf("Invio ACK al client...\n");
        if (send_ack(clientfd) != 0) {
            fprintf(stderr, "Errore durante l'invio dell'ACK.\n");
        }

        close(clientfd);
        printf("Connessione con il client chiusa.\n\n");
    }

    close(listenfd); // Teoricamente irraggiungibile nel loop while(1)
    return EXIT_SUCCESS; // Teoricamente irraggiungibile
}