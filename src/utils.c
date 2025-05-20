#include "common.h"
#include "utils.h"
#include <errno.h>
#include <string.h>

ssize_t read_n(int fd, void *buffer, size_t n) {
    size_t to_read = n;
    char *buf = buffer;
    while (to_read > 0) {
        ssize_t r = read(fd, buf, to_read);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) break;
        to_read -= r;
        buf += r;
    }
    return n - to_read;
}

ssize_t write_n(int fd, const void *buffer, size_t n) {
    size_t to_write = n;
    const char *buf = buffer;
    while (to_write > 0) {
        ssize_t w = write(fd, buf, to_write);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        to_write -= w;
        buf += w;
    }
    return n;
}

uint64_t htonll(uint64_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
#else
    return value;
#endif
}

uint64_t ntohll(uint64_t value) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((uint64_t)ntohl(value & 0xFFFFFFFF) << 32) | ntohl(value >> 32);
#else
    return value;
#endif
}
