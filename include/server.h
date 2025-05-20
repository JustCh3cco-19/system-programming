#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include <semaphore.h>
#include <netinet/in.h>

// Nodo per buffer a lista concatenata
struct node {
    uint64_t     data;
    struct node *next;
};

struct decrypt_data {
    struct node *start_node;
    size_t       count;
    key_t        key;
};

int server_run(int p, const char *prefix, int backlog, uint16_t port);

#endif // SERVER_H
