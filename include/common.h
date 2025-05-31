#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BLOCK_SIZE 8  // Dimensione di ogni blocco dati in byte

/**
 * @brief Converte un intero a 64 bit da host a network byte order (big-endian).
 * @param value Valore a 64 bit da convertire.
 * @return Valore convertito in network byte order.
 */
uint64_t htonll(uint64_t value);

/**
 * @brief Converte un intero a 64 bit da network a host byte order.
 * @param value Valore a 64 bit da convertire.
 * @return Valore convertito in host byte order.
 */
uint64_t ntohll(uint64_t value);

/**
 * @brief Legge esattamente n byte da un file descriptor.
 * @param fd File descriptor da cui leggere.
 * @param buffer Buffer di destinazione.
 * @param n Numero di byte da leggere.
 * @return Numero di byte effettivamente letti, oppure -1 in caso di errore.
 */
ssize_t read_n(int fd, void *buffer, size_t n);

/**
 * @brief Scrive esattamente n byte su un file descriptor.
 * @param fd File descriptor su cui scrivere.
 * @param buffer Buffer sorgente.
 * @param n Numero di byte da scrivere.
 * @return Numero di byte effettivamente scritti, oppure -1 in caso di errore.
 */
ssize_t write_n(int fd, const void *buffer, size_t n);

#endif // COMMON_H
