#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include <netinet/in.h>

struct thread_data {
    uint64_t *blocks;
    size_t    start;
    size_t    end;
    key_t     key;
};

int client_run(const char *filename, key_t key, int p, const char *server_ip, uint16_t port);

#endif // CLIENT_H
