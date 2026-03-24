#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "client_handler.h"
#include "user_list.h"
#include "../common/protocol.h"

// Tiempo para verificar la inactividad de un usuario.
#define INACTIVITY_CHECK_INTERVAL 10

int send_packet(int sockfd, uint8_t command, const char* sender,
                const char* target, const char* payload) {
    ChatPacket packet;
    memset(&packet, 0, sizeof(ChatPacket));

    packet.command = command;

    if (sender)  strncpy(packet.sender,  sender,  sizeof(packet.sender)  - 1);
    if (target)  strncpy(packet.target,  target,  sizeof(packet.target)  - 1);
    if (payload) {
        strncpy(packet.payload, payload, sizeof(packet.payload) - 1);
        packet.payload_len = htons((uint16_t)strlen(payload));
    }

    int result = send(sockfd, &packet, sizeof(ChatPacket), 0);
    if (result < 0) {
        perror("[handler] Error al enviar packet");
    }
    return result;
}

void broadcast_packet(uint8_t command, const char* sender,
                      const char* target, const char* payload,
                      int exclude_fd) {

    char buffer[957];
    int count = list_users(buffer, sizeof(buffer));
    if (count == 0) return;

    // Parsear "user1:STATUS,user2:STATUS"
    char temp[957];
    strncpy(temp, buffer, sizeof(temp) - 1);
    char* token = strtok(temp, ",");

    while (token != NULL) {
        /* Separar nombre del status */
        char username[32];
        strncpy(username, token, sizeof(username) - 1);
        char* colon = strchr(username, ':');
        if (colon) *colon = '\0';

        pthread_mutex_lock(&users_mutex);
        User* user = find_user(username);
        if (user != NULL && user->socket_fd != exclude_fd) {
            int fd = user->socket_fd;
            pthread_mutex_unlock(&users_mutex);
            send_packet(fd, command, sender, target, payload);
        } else {
            pthread_mutex_unlock(&users_mutex);
        }

        token = strtok(NULL, ",");
    }
}

static void handle_register(int sockfd, const char* ip, ChatPacket* packet,
                             char* username_out) {
    const char* username = packet->sender;

    int ok = add_user(username, ip, sockfd);
    if (ok) {
        strncpy(username_out, username, 32);
        send_packet(sockfd, CMD_OK, "server", username, "Registro exitoso");
        printf("[server] Usuario '%s' registrado desde %s\n", username, ip);

        /* Notificar a todos que hay un nuevo usuario */
        char msg[64];
        snprintf(msg, sizeof(msg), "%s se ha conectado", username);
        broadcast_packet(CMD_MSG, "server", "", msg, sockfd);
    } else {
        send_packet(sockfd, CMD_ERROR, "server", "", "Nombre o IP ya en uso");
        printf("[server] Registro rechazado para '%s' desde %s\n", username, ip);
    }
}

static void handle_broadcast(ChatPacket* packet) {
    const char* sender  = packet->sender;
    const char* payload = packet->payload;

    // Actualizar la actividad del usuario.
    update_activity(sender);
    set_status(sender, STATUS_ACTIVO);

    // Reenviar a todos menos al remitente.
    pthread_mutex_lock(&users_mutex);
    User* user = find_user(sender);
    int sender_fd = user ? user->socket_fd : -1;
    pthread_mutex_unlock(&users_mutex);

    broadcast_packet(CMD_MSG, sender, "", payload, sender_fd);
}

static void handle_direct(int sockfd, ChatPacket* packet) {
    const char* sender  = packet->sender;
    const char* target  = packet->target;
    const char* payload = packet->payload;

    // Actualizar actividad del usuario.
    update_activity(sender);
    set_status(sender, STATUS_ACTIVO);

    pthread_mutex_lock(&users_mutex);
    User* dest = find_user(target);
    if (dest == NULL) {
        pthread_mutex_unlock(&users_mutex);
        send_packet(sockfd, CMD_ERROR, "server", sender, "Usuario no encontrado");
        return;
    }
    int dest_fd = dest->socket_fd;
    pthread_mutex_unlock(&users_mutex);

    /* Enviar al destinatario */
    send_packet(dest_fd, CMD_MSG, sender, target, payload);

    /* Confirmar al remitente */
    send_packet(sockfd, CMD_OK, "server", sender, "Mensaje enviado");
}

static void handle_list(int sockfd) {
    char buffer[957];
    int count = list_users(buffer, sizeof(buffer));

    if (count == 0) {
        send_packet(sockfd, CMD_USER_LIST, "server", "", "No hay usuarios conectados");
    } else {
        send_packet(sockfd, CMD_USER_LIST, "server", "", buffer);
    }
}

static void handle_info(int sockfd, ChatPacket* packet) {
    const char* target = packet->target;

    pthread_mutex_lock(&users_mutex);
    User* user = find_user(target);
    if (user == NULL) {
        pthread_mutex_unlock(&users_mutex);
        send_packet(sockfd, CMD_ERROR, "server", "", "Usuario no encontrado");
        return;
    }

    char info[128];
    snprintf(info, sizeof(info), "%s:%s:%s", user->username, user->ip, user->status);
    pthread_mutex_unlock(&users_mutex);

    send_packet(sockfd, CMD_USER_INFO, "server", "", info);
}

static void handle_status(int sockfd, ChatPacket* packet) {
    const char* sender  = packet->sender;
    const char* payload = packet->payload;

    if (strcmp(payload, STATUS_ACTIVO)   != 0 &&
        strcmp(payload, STATUS_OCUPADO)  != 0 &&
        strcmp(payload, STATUS_INACTIVO) != 0) {
        send_packet(sockfd, CMD_ERROR, "server", sender, "Status inválido");
        return;
    }

    int ok = set_status(sender, payload);
    if (ok) {
        send_packet(sockfd, CMD_OK, "server", sender, payload);
    } else {
        send_packet(sockfd, CMD_ERROR, "server", sender, "Error al cambiar status");
    }
}

static void handle_logout(int sockfd, const char* username) {
    if (strlen(username) == 0) return;

    // Notificar a los usuarios que un usuario abandonó el servidor.
    char msg[64];
    snprintf(msg, sizeof(msg), "%s se ha desconectado", username);
    broadcast_packet(CMD_MSG, "server", "", msg, sockfd);

    remove_user(username);
    printf("[server] Usuario '%s' desconectado\n", username);
}

void* handle_client(void* arg) {
    ClientArgs* args = (ClientArgs*)arg;
    int  sockfd = args->socket_fd;
    char ip[INET_ADDRSTRLEN];
    strncpy(ip, args->ip, sizeof(ip) - 1);
    free(args);

    char username[32];
    memset(username, 0, sizeof(username));

    ChatPacket packet;
    int registered = 0;

    while (1) {
        memset(&packet, 0, sizeof(ChatPacket));
        int bytes = recv(sockfd, &packet, sizeof(ChatPacket), 0);

        if (bytes <= 0) {
            if (registered) {
                printf("[server] Conexión perdida con '%s'\n", username);
                handle_logout(sockfd, username);
            } else {
                printf("[server] Conexión perdida con %s (sin registrar)\n", ip);
            }
            break;
        }

        // Registrarse antes de interactuar.
        if (!registered) {
            if (packet.command == CMD_REGISTER) {
                handle_register(sockfd, ip, &packet, username);
                if (strlen(username) > 0) registered = 1;
            } else {
                send_packet(sockfd, CMD_ERROR, "server", "",
                            "Debes registrarte primero");
            }
            continue;
        }

        // Comandos.
        switch (packet.command) {
            case CMD_BROADCAST: handle_broadcast(&packet);             break;
            case CMD_DIRECT:    handle_direct(sockfd, &packet);        break;
            case CMD_LIST:      handle_list(sockfd);                   break;
            case CMD_INFO:      handle_info(sockfd, &packet);          break;
            case CMD_STATUS:    handle_status(sockfd, &packet);        break;
            case CMD_LOGOUT:
                handle_logout(sockfd, username);
                close(sockfd);
                return NULL;
            default:
                send_packet(sockfd, CMD_ERROR, "server", "", "Comando desconocido");
                break;
        }
    }

    close(sockfd);
    return NULL;
}

void* inactivity_checker(void* arg) {
    (void)arg;

    while (1) {
        sleep(INACTIVITY_CHECK_INTERVAL);

        time_t now = time(NULL);

        char buffer[957];
        int count = list_users(buffer, sizeof(buffer));
        if (count == 0) continue;

        char temp[957];
        strncpy(temp, buffer, sizeof(temp) - 1);
        char* token = strtok(temp, ",");

        while (token != NULL) {
            char username[32];
            strncpy(username, token, sizeof(username) - 1);
            char* colon = strchr(username, ':');
            if (colon) *colon = '\0';

            pthread_mutex_lock(&users_mutex);
            User* user = find_user(username);
            if (user != NULL) {
                double elapsed = difftime(now, user->last_activity);

                // Establecer a un usuario como inactivo.
                if (elapsed >= INACTIVITY_TIMEOUT &&
                    strcmp(user->status, STATUS_INACTIVO) != 0) {

                    int fd = user->socket_fd;
                    strncpy(user->status, STATUS_INACTIVO, sizeof(user->status) - 1);
                    pthread_mutex_unlock(&users_mutex);

                    printf("[server] Usuario '%s' marcado como INACTIVE\n", username);

                    send_packet(fd, CMD_STATUS, "server", username, STATUS_INACTIVO);
                } else {
                    pthread_mutex_unlock(&users_mutex);
                }
            } else {
                pthread_mutex_unlock(&users_mutex);
            }

            token = strtok(NULL, ",");
        }
    }

    return NULL;
}