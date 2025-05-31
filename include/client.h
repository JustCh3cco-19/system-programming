#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include <netinet/in.h>

/**
 * @brief Struttura dati utilizzata dai thread di cifratura.
 * 
 * blocks: Puntatore all'array dei blocchi dati da cifrare.
 * start:  Indice iniziale (inclusivo) dei blocchi da cifrare.
 * end:    Indice finale (esclusivo) dei blocchi da cifrare.
 * key:    Chiave di cifratura da applicare ai blocchi.
 */
struct thread_data {
    uint64_t *blocks;
    size_t    start;
    size_t    end;
    uint64_t  key;
};

/**
 * @brief Funzione principale per l'esecuzione del client.
 * 
 * @param filename  Nome del file da inviare.
 * @param key       Chiave di cifratura.
 * @param p         Numero di thread di cifratura.
 * @param server_ip Indirizzo IP del server.
 * @param port      Porta TCP del server.
 * @return 0 in caso di successo, 1 in caso di errore.
 */
int client_run(const char *filename, uint64_t key, int p, const char *server_ip, uint16_t port);

#endif // CLIENT_H
