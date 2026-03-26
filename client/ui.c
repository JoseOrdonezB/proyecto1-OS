#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "client.h"

void print_help() {
    printf("\n--- Comandos Disponibles ---\n");
    printf("/list            : Ver lista de usuarios conectados [cite: 29, 55]\n");
    printf("/info <usuario>  : Ver información de un usuario [cite: 36, 56]\n");
    printf("/status <estado> : Cambiar estado (ACTIVE, BUSY, INACTIVE) [cite: 38, 54, 60]\n");
    printf("/exit            : Desconectarse y salir [cite: 27, 58]\n");
    printf("<usuario> <msg>  : Enviar mensaje privado [cite: 43, 53, 59]\n");
    printf("<mensaje>        : Enviar mensaje a todos (Broadcast) [cite: 43, 52]\n");
    printf("----------------------------\n");
}

void ui_loop() {
    char input[1024];
    while (running) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;

        if (strncmp(input, "/exit", 5) == 0) {
            send_packet_client(CMD_LOGOUT, "", "");
            running = 0;
        } else if (strncmp(input, "/list", 5) == 0) {
            send_packet_client(CMD_LIST, "", "");
        } else if (strncmp(input, "/info ", 6) == 0) {
            send_packet_client(CMD_INFO, input + 6, "");
        } else if (strncmp(input, "/status ", 8) == 0) {
            send_packet_client(CMD_STATUS, "", input + 8);
        } else {
            // Lógica de Mensaje Directo vs Broadcast [cite: 43, 59]
            char *space = strchr(input, ' ');
            if (space) {
                char target[32];
                int name_len = space - input;
                if (name_len > 31) name_len = 31;
                strncpy(target, input, name_len);
                target[name_len] = '\0';
                send_packet_client(CMD_DIRECT, target, space + 1);
            } else {
                send_packet_client(CMD_BROADCAST, "", input);
            }
        }
    }
}