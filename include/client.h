#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

// Tipo per i valori a 64 bit
typedef unsigned long long u64;

// Struttura per passare gli argomenti ai thread
typedef struct {
    u64 *blocks;        // Array di blocchi da elaborare
    u64 key;            // Chiave di cifratura
    int start_idx;      // Indice iniziale nel blocco
    int end_idx;        // Indice finale (esclusivo)
} ThreadArgs;

// Funzione eseguita dai thread per cifrare i blocchi
void* encrypt_blocks(void* arg);

// Funzione per gestire la conversione in network byte order (64-bit)
u64 htonll(u64 val);

#endif // CLIENT_H
