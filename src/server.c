#include "server.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

static int server_p;
static const char *file_prefix;
static sem_t conn_sem;

// Blocca i segnali specificati in tutti i thread
static void block_signals(void) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

void *decrypt_thread(void *arg) {
    struct decrypt_data *dd = arg;
    block_signals();
    struct node *curr = dd->start_node;
    for (size_t i = 0; i < dd->count && curr; ++i) {
        curr->data ^= dd->key;
        curr = curr->next;
    }
    return NULL;
}

void *handle_client(void *arg) {
    int client_fd = *(int*)arg;
    free(arg);
    sem_wait(&conn_sem);

    // 1) Ricevi lunghezza e chiave
    uint64_t net_L, net_key;
    if (read_n(client_fd, &net_L,   sizeof net_L) <= 0) goto cleanup;
    if (read_n(client_fd, &net_key, sizeof net_key) <= 0) goto cleanup;
    size_t L   = ntohll(net_L);
    key_t  key = ntohll(net_key);

    // 2) Costruisci la linked-list dei blocchi ricevuti
    size_t n = (L + BLOCK_SIZE - 1) / BLOCK_SIZE;
    struct node *head = NULL, *tail = NULL;
    for (size_t i = 0; i < n; ++i) {
        struct node *nd = malloc(sizeof *nd);
        nd->next = NULL;
        uint64_t block;
        read_n(client_fd, &block, sizeof block);
        nd->data = block;
        if (!head) head = nd; else tail->next = nd;
        tail = nd;
    }

    // 3) Blocca segnali e decifra in parallelo
    block_signals();
    pthread_t *threads = malloc(server_p * sizeof(pthread_t));
    struct decrypt_data *dd = malloc(server_p * sizeof *dd);
    size_t chunk = n / server_p;
    struct node *curr = head;
    for (int i = 0; i < server_p; ++i) {
        dd[i].start_node = curr;
        dd[i].count      = (i == server_p-1) ? (n - i*chunk) : chunk;
        dd[i].key        = key;
        for (size_t j = 0; j < dd[i].count; ++j) curr = curr->next;
        pthread_create(&threads[i], NULL, decrypt_thread, &dd[i]);
    }
    for (int i = 0; i < server_p; ++i) pthread_join(threads[i], NULL);

    // 4) Invia ACK
    signal(SIGPIPE, SIG_IGN);
    write_n(client_fd, "ACK", 3);

    // 5) Scrivi su file, troncando l’ultimo blocco a L byte esatti
    time_t now = time(NULL);
    struct tm tm = *localtime(&now);
    char filename[256];
    snprintf(filename, sizeof filename,
             "%s_%04d%02d%02d%02d%02d%02d_%d.bin",
             file_prefix,
             tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             getpid());
    FILE *fp = fopen(filename, "wb");
    if (fp) {
        struct node *it = head;
        size_t written = 0;
        while (it && written < L) {
            size_t to_write = (L - written) < BLOCK_SIZE
                                ? (L - written)
                                : BLOCK_SIZE;
            fwrite(&it->data, 1, to_write, fp);
            written += to_write;
            it = it->next;
        }
        fclose(fp);
    }

cleanup:
    close(client_fd);
    // libero la linked-list
    struct node *it = head;
    while (it) {
        struct node *nx = it->next;
        free(it);
        it = nx;
    }
    free(threads);
    free(dd);
    sem_post(&conn_sem);
    return NULL;
}

int server_run(int p, const char *prefix, int backlog, uint16_t port) {
    server_p    = p;
    file_prefix = prefix;
    sem_init(&conn_sem, 0, backlog);

    signal(SIGPIPE, SIG_IGN);
    block_signals();

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    if (listen_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof addr) < 0) {
        perror("bind"); return 1;
    }
    listen(listen_fd, backlog);

    while (1) {
        int *fdp = malloc(sizeof *fdp);
        *fdp = accept(listen_fd, NULL, NULL);
        if (*fdp < 0) { perror("accept"); free(fdp); continue; }
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, fdp);
        pthread_detach(tid);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    int p; const char *prefix; int backlog; uint16_t port;
    if (getenv("SRV_P"))       p       = atoi(getenv("SRV_P"));
    else if (argc > 1)         p       = atoi(argv[1]);
    else { fprintf(stderr,"Usage: server <p> <prefix> <backlog> <port>\n"); return 1; }
    if (getenv("SRV_PREFIX"))  prefix  = getenv("SRV_PREFIX");
    else if (argc > 2)         prefix  = argv[2];
    else { fprintf(stderr,"Usage: server <p> <prefix> <backlog> <port>\n"); return 1; }
    if (getenv("SRV_BACKLOG")) backlog = atoi(getenv("SRV_BACKLOG"));
    else if (argc > 3)         backlog = atoi(argv[3]);
    else { fprintf(stderr,"Usage: server <p> <prefix> <backlog> <port>\n"); return 1; }
    if (getenv("SRV_PORT"))    port    = atoi(getenv("SRV_PORT"));
    else if (argc > 4)         port    = atoi(argv[4]);
    else { fprintf(stderr,"Usage: server <p> <prefix> <backlog> <port>\n"); return 1; }
    return server_run(p, prefix, backlog, port);
}
