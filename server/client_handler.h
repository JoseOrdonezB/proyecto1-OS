#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <netinet/in.h>
#include "../common/protocol.h"

/* ── Argumentos que se le pasan al thread de cada cliente ── */
typedef struct {
    int  socket_fd;
    char ip[INET_ADDRSTRLEN];
} ClientArgs;

// Thread por cliente.
void* handle_client(void* arg);

// Thread para inactividad de los usuarios.
void* inactivity_checker(void* arg);

// Enviar un packet a un socket.
int send_packet(int sockfd, uint8_t command, const char* sender,
                const char* target, const char* payload);

// Anunciar a todos los participantes.
void broadcast_packet(uint8_t command, const char* sender,
                      const char* target, const char* payload,
                      int exclude_fd);

#endif