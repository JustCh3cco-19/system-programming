#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

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

#endif // UTILS_H
