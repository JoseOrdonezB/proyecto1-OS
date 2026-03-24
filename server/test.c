#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common/protocol.h"

/* ── Colores para la terminal ── */
#define GREEN  "\033[0;32m"
#define RED    "\033[0;31m"
#define YELLOW "\033[0;33m"
#define RESET  "\033[0m"

/* ────────────────────────────────────────────────────────────────────────── */

int connect_to_server(const char* ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error al crear socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error al conectar");
        return -1;
    }

    return sock;
}

/* ────────────────────────────────────────────────────────────────────────── */

int send_packet(int sock, uint8_t command, const char* sender,
                const char* target, const char* payload) {
    ChatPacket p;
    memset(&p, 0, sizeof(ChatPacket));

    p.command = command;
    if (sender)  strncpy(p.sender,  sender,  sizeof(p.sender)  - 1);
    if (target)  strncpy(p.target,  target,  sizeof(p.target)  - 1);
    if (payload) {
        strncpy(p.payload, payload, sizeof(p.payload) - 1);
        p.payload_len = htons((uint16_t)strlen(payload));
    }

    return send(sock, &p, sizeof(ChatPacket), 0);
}

/* ────────────────────────────────────────────────────────────────────────── */

int recv_response(int sock, ChatPacket* resp) {
    memset(resp, 0, sizeof(ChatPacket));
    int bytes = recv(sock, resp, sizeof(ChatPacket), 0);
    return bytes;
}

/* ────────────────────────────────────────────────────────────────────────── */

void print_result(const char* test, ChatPacket* resp, uint8_t expected_cmd) {
    if (resp->command == expected_cmd) {
        printf(GREEN "[PASS]" RESET " %s → command=%d payload=%s\n",
               test, resp->command, resp->payload);
    } else {
        printf(RED "[FAIL]" RESET " %s → esperado=%d recibido=%d payload=%s\n",
               test, expected_cmd, resp->command, resp->payload);
    }
}

/* ────────────────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IP servidor> <puerto>\n", argv[0]);
        return 1;
    }

    const char* ip   = argv[1];
    int         port = atoi(argv[2]);

    printf(YELLOW "\n=== TEST DEL SERVIDOR ===\n\n" RESET);

    /* ── 1. Conectar al servidor ── */
    printf("Conectando a %s:%d...\n", ip, port);
    int sock = connect_to_server(ip, port);
    if (sock < 0) return 1;
    printf(GREEN "[OK]" RESET " Conexión establecida\n\n");

    ChatPacket resp;

    /* ── 2. Registro exitoso ── */
    printf("--- Prueba 1: Registro exitoso ---\n");
    send_packet(sock, CMD_REGISTER, "testuser", "", "");
    recv_response(sock, &resp);
    print_result("CMD_REGISTER 'testuser'", &resp, CMD_OK);

    /* ── 3. Registro duplicado (mismo socket = misma IP) ── */
    printf("\n--- Prueba 2: Registro con nombre duplicado ---\n");
    int sock2 = connect_to_server(ip, port);
    if (sock2 >= 0) {
        send_packet(sock2, CMD_REGISTER, "testuser", "", "");
        recv_response(sock2, &resp);
        print_result("CMD_REGISTER duplicado", &resp, CMD_ERROR);
        close(sock2);
    }

    /* ── 4. Lista de usuarios ── */
    printf("\n--- Prueba 3: Lista de usuarios ---\n");
    send_packet(sock, CMD_LIST, "testuser", "", "");
    recv_response(sock, &resp);
    print_result("CMD_LIST", &resp, CMD_USER_LIST);
    printf("       Usuarios: %s\n", resp.payload);

    /* ── 5. Info de usuario ── */
    printf("\n--- Prueba 4: Info de usuario ---\n");
    send_packet(sock, CMD_INFO, "testuser", "testuser", "");
    recv_response(sock, &resp);
    print_result("CMD_INFO 'testuser'", &resp, CMD_USER_INFO);
    printf("       Info: %s\n", resp.payload);

    /* ── 6. Info de usuario inexistente ── */
    printf("\n--- Prueba 5: Info de usuario inexistente ---\n");
    send_packet(sock, CMD_INFO, "testuser", "noexiste", "");
    recv_response(sock, &resp);
    print_result("CMD_INFO 'noexiste'", &resp, CMD_ERROR);

    /* ── 7. Cambio de status a BUSY ── */
    printf("\n--- Prueba 6: Cambio de status a BUSY ---\n");
    send_packet(sock, CMD_STATUS, "testuser", "", STATUS_OCUPADO);
    recv_response(sock, &resp);
    print_result("CMD_STATUS BUSY", &resp, CMD_OK);

    /* ── 8. Cambio de status a INACTIVE ── */
    printf("\n--- Prueba 7: Cambio de status a INACTIVE ---\n");
    send_packet(sock, CMD_STATUS, "testuser", "", STATUS_INACTIVO);
    recv_response(sock, &resp);
    print_result("CMD_STATUS INACTIVE", &resp, CMD_OK);

    /* ── 9. Cambio de status inválido ── */
    printf("\n--- Prueba 8: Status inválido ---\n");
    send_packet(sock, CMD_STATUS, "testuser", "", "INVALIDO");
    recv_response(sock, &resp);
    print_result("CMD_STATUS inválido", &resp, CMD_ERROR);

    /* ── 10. Broadcast ── */
    printf("\n--- Prueba 9: Broadcast ---\n");
    send_packet(sock, CMD_BROADCAST, "testuser", "", "Hola a todos!");
    /* No hay otros usuarios, pero no debe crashear */
    printf(GREEN "[OK]" RESET " CMD_BROADCAST enviado sin crash\n");

    /* ── 11. Mensaje directo a usuario inexistente ── */
    printf("\n--- Prueba 10: Mensaje directo a usuario inexistente ---\n");
    send_packet(sock, CMD_DIRECT, "testuser", "noexiste", "Hola!");
    recv_response(sock, &resp);
    print_result("CMD_DIRECT a inexistente", &resp, CMD_ERROR);

    /* ── 12. Logout ── */
    printf("\n--- Prueba 11: Logout ---\n");
    send_packet(sock, CMD_LOGOUT, "testuser", "", "");
    close(sock);
    printf(GREEN "[OK]" RESET " CMD_LOGOUT enviado y socket cerrado\n");

    printf(YELLOW "\n=== FIN DE PRUEBAS ===\n\n" RESET);
    return 0;
}