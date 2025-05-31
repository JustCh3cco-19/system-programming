#include "common.h"
#include "utils.h"
#include <errno.h>
#include <string.h>

/**
 * @brief Legge esattamente `n` byte da un file descriptor `fd` nel buffer `buffer`.
 *        Continua a leggere anche se viene interrotto da un segnale (EINTR).
 * @param fd File descriptor da cui leggere.
 * @param buffer Buffer di destinazione.
 * @param n Numero di byte da leggere.
 * @return Numero di byte effettivamente letti, oppure -1 in caso di errore.
 */
ssize_t read_n(int fd, void *buffer, size_t n) {
    size_t to_read = n;
    char *buf = buffer;
    while (to_read > 0) {
        ssize_t r = read(fd, buf, to_read);
        if (r < 0) {
            if (errno == EINTR) continue; // Se interrotto da segnale, riprova
            return -1;
        }
        if (r == 0) break; // EOF
        to_read -= r;
        buf += r;
    }
    return n - to_read;
}

/**
 * @brief Scrive esattamente `n` byte dal buffer `buffer` nel file descriptor `fd`.
 *        Continua a scrivere anche se viene interrotto da un segnale (EINTR).
 * @param fd File descriptor su cui scrivere.
 * @param buffer Buffer sorgente.
 * @param n Numero di byte da scrivere.
 * @return Numero di byte effettivamente scritti, oppure -1 in caso di errore.
 */
ssize_t write_n(int fd, const void *buffer, size_t n) {
    size_t to_write = n;
    const char *buf = buffer;
    while (to_write > 0) {
        ssize_t w = write(fd, buf, to_write);
        if (w < 0) {
            if (errno == EINTR) continue; // Se interrotto da segnale, riprova
            return -1;
        }
        to_write -= w;
        buf += w;
    }
    return n;
}

/**
 * @brief Converte un intero 64 bit da host a network byte order (big-endian).
 * @param value Valore a 64 bit da convertire.
 * @return Valore convertito in network byte order.
 */
uint64_t htonll(uint64_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
#else
    return value;
#endif
}

/**
 * @brief Converte un intero 64 bit da network a host byte order.
 *        Esegue la conversione solo su sistemi little-endian.
 * @param value Valore a 64 bit da convertire.
 * @return Valore convertito in host byte order.
 */
uint64_t ntohll(uint64_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((uint64_t)ntohl(value & 0xFFFFFFFF) << 32) | ntohl(value >> 32);
#else
    return value;
#endif
}
