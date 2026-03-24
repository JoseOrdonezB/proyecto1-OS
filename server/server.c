#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "../common/protocol.h"
#include "user_list.h"
#include "client_handler.h"

static int running = 1;
static int server_fd_global = -1;

void handle_sigint(int sig) {
    (void)sig;
    printf("\n[server] Cerrando servidor...\n");
    running = 0;
    if (server_fd_global != 1) {
        close(server_fd_global);
    }
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "[server] Puerto inválido: %s\n", argv[1]);
        return 1;
    }

    signal(SIGINT, handle_sigint);

    signal(SIGPIPE, SIG_IGN);

    // Inicializar lista de usuarios.
    user_list_init();

    // Crea socket del servidor.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_fd_global = server_fd;
    if (server_fd < 0) {
        perror("[server] Error al crear socket");
        return 1;
    }

    // No bloquear el puerto al cerrar el servidor.
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Configurar dirección */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[server] Error en bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("[server] Error en listen");
        close(server_fd);
        return 1;
    }

    printf("[server] Servidor iniciado en puerto %d\n", port);
    printf("[server] Esperando conexiones...\n");

    // Thread de inactividad.
    pthread_t inactivity_tid;
    pthread_create(&inactivity_tid, NULL, inactivity_checker, NULL);
    pthread_detach(inactivity_tid);
    printf("[server] Esperando conexiones...\n");

    /* ── Loop principal: aceptar conexiones ── */
    while (running) {

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running) {
                perror("[server] Error en accept");
            }
            continue;
        }

        // IP de los clientes.
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[server] Nueva conexión desde %s\n", client_ip);

        ClientArgs* args = malloc(sizeof(ClientArgs));
        if (args == NULL) {
            fprintf(stderr, "[server] Error al allocar memoria para thread\n");
            close(client_fd);
            continue;
        }

        args->socket_fd = client_fd;
        strncpy(args->ip, client_ip, sizeof(args->ip) - 1);
        args->ip[sizeof(args->ip) - 1] = '\0';

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, args) != 0) {
            perror("[server] Error al crear thread");
            free(args);
            close(client_fd);
            continue;
        }

        // Limpiar thread cuando termina con su proceso
        pthread_detach(tid);
    }

    // Cerrar servidor.
    close(server_fd);
    printf("[server] Servidor cerrado.\n");
    return 0;
}