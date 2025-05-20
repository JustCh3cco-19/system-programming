#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

ssize_t read_n(int fd, void *buffer, size_t n);
ssize_t write_n(int fd, const void *buffer, size_t n);

#endif // UTILS_H
