#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include <semaphore.h>
#include <netinet/in.h>

/**
 * @brief Nodo per la lista concatenata dei blocchi ricevuti dal client.
 * 
 * data: Dato cifrato/decriptato (uint64_t).
 * next: Puntatore al prossimo nodo della lista.
 */
struct node {
    uint64_t     data;
    struct node *next;
};

/**
 * @brief Struttura dati utilizzata dai thread di decrittazione.
 * 
 * start_node: Nodo iniziale della porzione di lista da decrittare.
 * count:      Numero di nodi da decrittare.
 * key:        Chiave di decrittazione da applicare.
 */
struct decrypt_data {
    struct node *start_node;
    size_t       count;
    uint64_t     key;
};

/**
 * @brief Funzione principale per l'avvio del server.
 * 
 * @param p       Numero di thread di decrittazione per ogni client.
 * @param prefix  Prefisso per i file di output.
 * @param backlog Numero massimo di connessioni concorrenti.
 * @param port    Porta TCP su cui ascoltare.
 * @return 0 in caso di successo, 1 in caso di errore.
 */
int server_run(int p, const char *prefix, int backlog, uint16_t port);

#endif // SERVER_H
