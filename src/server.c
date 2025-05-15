#include "server.h"

int main(int argc, char* argv[]) {
    server_config_t config;
    parse_arguments(argc, argv, &config);
    
    sem_t conn_sem;
    sem_init(&conn_sem, 0, config.max_connections);
    
    int server_sock = create_server_socket(config.server_port);
    printf("Server listening on port %d\n", config.server_port);

    while (1) {
        int client_sock = accept_connection(server_sock);
        
        if (sem_trywait(&conn_sem) != 0) {
            fprintf(stderr, "Max connections reached\n");
            close(client_sock);
            continue;
        }

        client_params_t* params = malloc(sizeof(client_params_t));
        params->client_sock = client_sock;
        params->config = config;
        params->conn_sem = &conn_sem;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, params) != 0) {
            fprintf(stderr, "Thread creation failed\n");
            close(client_sock);
            sem_post(&conn_sem);
            free(params);
        } else {
            pthread_detach(tid);
        }
    }

    close(server_sock);
    sem_destroy(&conn_sem);
    return 0;
}