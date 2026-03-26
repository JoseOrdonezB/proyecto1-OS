#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "client.h"
#include "receiver.h"
#include "ui.h"

int sockfd;
char my_username[32];
int running = 1;

void send_packet_client(uint8_t command, const char* target, const char* payload) {
    ChatPacket packet;
    memset(&packet, 0, sizeof(ChatPacket));
    packet.command = command;
    strncpy(packet.sender, my_username, sizeof(packet.sender) - 1);
    if (target) strncpy(packet.target, target, sizeof(packet.target) - 1);
    if (payload) {
        strncpy(packet.payload, payload, sizeof(packet.payload) - 1);
        packet.payload_len = htons((uint16_t)strlen(payload));
    }
    send(sockfd, &packet, sizeof(ChatPacket), 0);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <username> <IP_servidor> <puerto>\n", argv[0]);
        return 1;
    }

    strncpy(my_username, argv[1], 31);
    char* server_ip = argv[2];
    int port = atoi(argv[3]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error al conectar");
        return 1;
    }

    // Registro inicial
    send_packet_client(CMD_REGISTER, "", "");
    
    printf("Conectado exitosamente como %s\n", my_username);
    print_help();

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_handler, NULL);
    pthread_detach(recv_tid);

    ui_loop();

    close(sockfd);
    return 0;
}