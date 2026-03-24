#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_list.h"

/* ── Variables internas ── */
static User* head = NULL;
static int   count = 0;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

void user_list_init() {
    head  = NULL;
    count = 0;
    pthread_mutex_init(&users_mutex, NULL);
    printf("[user_list] Lista inicializada. Máximo %d usuarios.\n", MAX_USERS);
}

int add_user(const char* username, const char* ip, int sockfd) {
    pthread_mutex_lock(&users_mutex);

    /* Verificar límite de usuarios */
    if (count >= MAX_USERS) {
        printf("[user_list] Límite de usuarios alcanzado (%d).\n", MAX_USERS);
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }

    /* Verificar que no exista el mismo nombre o IP */
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            printf("[user_list] Nombre '%s' ya existe.\n", username);
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
        if (strcmp(current->ip, ip) == 0) {
            printf("[user_list] IP '%s' ya está conectada.\n", ip);
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
        current = current->next;
    }

    /* Crear nuevo usuario */
    User* new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        printf("[user_list] Error al allocar memoria.\n");
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }

    strncpy(new_user->username, username, sizeof(new_user->username) - 1);
    new_user->username[sizeof(new_user->username) - 1] = '\0';

    strncpy(new_user->ip, ip, sizeof(new_user->ip) - 1);
    new_user->ip[sizeof(new_user->ip) - 1] = '\0';

    new_user->socket_fd    = sockfd;
    new_user->last_activity = time(NULL);
    strncpy(new_user->status, STATUS_ACTIVO, sizeof(new_user->status) - 1);
    new_user->status[sizeof(new_user->status) - 1] = '\0';

    /* Insertar al inicio de la lista */
    new_user->next = head;
    head  = new_user;
    count++;

    printf("[user_list] Usuario '%s' (%s) agregado. Total: %d\n",
           username, ip, count);

    pthread_mutex_unlock(&users_mutex);
    return 1;
}

void remove_user(const char* username) {
    pthread_mutex_lock(&users_mutex);

    User* current  = head;
    User* previous = NULL;

    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            if (previous == NULL) {
                head = current->next;
            } else {
                previous->next = current->next;
            }
            printf("[user_list] Usuario '%s' eliminado. Total: %d\n",
                   username, count - 1);
            free(current);
            count--;
            pthread_mutex_unlock(&users_mutex);
            return;
        }
        previous = current;
        current  = current->next;
    }

    printf("[user_list] Usuario '%s' no encontrado para eliminar.\n", username);
    pthread_mutex_unlock(&users_mutex);
}

/* IMPORTANTE: Esta función NO bloquea el mutex.
   Quien la llame debe hacer lock/unlock si es necesario. */
User* find_user(const char* username) {
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

User* find_user_by_ip(const char* ip) {
    pthread_mutex_lock(&users_mutex);
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->ip, ip) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return current;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&users_mutex);
    return NULL;
}

int set_status(const char* username, const char* status) {
    pthread_mutex_lock(&users_mutex);

    User* user = find_user(username);
    if (user == NULL) {
        printf("[user_list] set_status: '%s' no encontrado.\n", username);
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }

    strncpy(user->status, status, sizeof(user->status) - 1);
    user->status[sizeof(user->status) - 1] = '\0';
    printf("[user_list] Status de '%s' cambiado a '%s'.\n", username, status);

    pthread_mutex_unlock(&users_mutex);
    return 1;
}

void update_activity(const char* username) {
    pthread_mutex_lock(&users_mutex);

    User* user = find_user(username);
    if (user != NULL) {
        user->last_activity = time(NULL);
    }

    pthread_mutex_unlock(&users_mutex);
}

int list_users(char* buffer, size_t bufsize) {
    pthread_mutex_lock(&users_mutex);

    buffer[0] = '\0';
    int found = 0;
    User* current = head;

    while (current != NULL) {
        /* formato: "username:status," */
        char entry[64];
        snprintf(entry, sizeof(entry), "%s:%s,", current->username, current->status);

        if (strlen(buffer) + strlen(entry) < bufsize - 1) {
            strncat(buffer, entry, bufsize - strlen(buffer) - 1);
            found++;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&users_mutex);
    return found;
}

int user_count() {
    pthread_mutex_lock(&users_mutex);
    int c = count;
    pthread_mutex_unlock(&users_mutex);
    return c;
}