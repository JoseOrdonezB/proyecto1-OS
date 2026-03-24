#ifndef USER_LIST_H
#define USER_LIST_H

#include <stdint.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include "../common/protocol.h"

#define MAX_USERS 50

// Estructura del usuario conectado.
typedef struct User {
    char username[32];
    char ip[INET_ADDRSTRLEN];
    int  socket_fd;
    char status[10];
    time_t last_activity;
    struct User* next;
} User;

/* ── Funciones de la lista ── */
// Inicializar listado de usuarios conectados.
void user_list_init();

// Agrega usuario al servidor.
int add_user(const char* username, const char* ip, int sockfd);

// Elimina usuario del servidor.
void remove_user(const char* username);

// Busca usuario por nombre.
User* find_user(const char* username);

// Busca usuario por ip.
User* find_user_by_ip(const char* ip);

// Determina el estado del usuario.
int set_status(const char* username, const char* status);

// Actualiza la ultima actividad del usuario.
void update_activity(const char* username);

// Devuelve la lista de usuarios conectados.
int list_users(char* buffer, size_t bufsize);

// Devuelve la cantidad de usuarios conectados.
int user_count();

// Para operaciones que necesiten consistencia.
extern pthread_mutex_t users_mutex;

#endif