#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BLOCK_SIZE 8

typedef uint64_t key_t;

uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);
ssize_t read_n(int fd, void *buffer, size_t n);
ssize_t write_n(int fd, const void *buffer, size_t n);

#endif // COMMON_H
