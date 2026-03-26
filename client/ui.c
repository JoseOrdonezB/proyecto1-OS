#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "client.h"

void print_help() {
    printf("\n╔══════════════════════════════════════╗\n");
    printf("║         MENÚ DE OPCIONES             ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  1. Enviar mensaje a todos           ║\n");
    printf("║  2. Enviar mensaje privado           ║\n");
    printf("║  3. Cambiar estado                   ║\n");
    printf("║  4. Ver usuarios conectados          ║\n");
    printf("║  5. Ver info de un usuario           ║\n");
    printf("║  6. Ayuda                            ║\n");
    printf("║  7. Salir                            ║\n");
    printf("╚══════════════════════════════════════╝\n");
}

static void read_input(const char* prompt, char* buffer, int size) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buffer, size, stdin)) {
        buffer[0] = '\0';
        return;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

static void handle_broadcast() {
    char msg[957];
    read_input("  Mensaje para todos: ", msg, sizeof(msg));
    if (strlen(msg) == 0) {
        printf("[!] Mensaje vacío, cancelado.\n");
        return;
    }
    send_packet_client(CMD_BROADCAST, "", msg);
}

static void handle_direct() {
    char target[32];
    char msg[957];

    read_input("  Usuario destinatario: ", target, sizeof(target));
    if (strlen(target) == 0) {
        printf("[!] Usuario vacío, cancelado.\n");
        return;
    }

    read_input("  Mensaje: ", msg, sizeof(msg));
    if (strlen(msg) == 0) {
        printf("[!] Mensaje vacío, cancelado.\n");
        return;
    }

    send_packet_client(CMD_DIRECT, target, msg);
}

static void handle_status() {
    printf("\n  Estados disponibles:\n");
    printf("    1. ACTIVE\n");
    printf("    2. BUSY\n");
    printf("    3. INACTIVE\n");

    char opt[8];
    read_input("  Elige una opción: ", opt, sizeof(opt));

    const char* status = NULL;
    if (strcmp(opt, "1") == 0)      status = "ACTIVE";
    else if (strcmp(opt, "2") == 0) status = "BUSY";
    else if (strcmp(opt, "3") == 0) status = "INACTIVE";
    else {
        printf("[!] Opción inválida.\n");
        return;
    }

    send_packet_client(CMD_STATUS, "", status);
}

static void handle_info() {
    char target[32];
    read_input("  Nombre de usuario: ", target, sizeof(target));
    if (strlen(target) == 0) {
        printf("[!] Usuario vacío, cancelado.\n");
        return;
    }
    send_packet_client(CMD_INFO, target, "");
}

void ui_loop() {
    char opt[8];

    while (running) {
        print_help();
        read_input("  Elige una opción: ", opt, sizeof(opt));

        if (strcmp(opt, "1") == 0) {
            handle_broadcast();
        } else if (strcmp(opt, "2") == 0) {
            handle_direct();
        } else if (strcmp(opt, "3") == 0) {
            handle_status();
        } else if (strcmp(opt, "4") == 0) {
            send_packet_client(CMD_LIST, "", "");
            /* pequeña pausa para que el receiver imprima la respuesta
               antes de que aparezca el menú de nuevo */
            usleep(200000);
        } else if (strcmp(opt, "5") == 0) {
            handle_info();
            usleep(200000);
        } else if (strcmp(opt, "6") == 0) {
            /* el menú ya se muestra en el siguiente ciclo */
        } else if (strcmp(opt, "7") == 0) {
            send_packet_client(CMD_LOGOUT, "", "");
            running = 0;
            printf("  Hasta luego!\n");
        } else {
            printf("[!] Opción inválida, intenta de nuevo.\n");
        }
    }
}