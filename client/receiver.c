#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "receiver.h"
#include "client.h"

void* receive_handler(void* arg) {
    (void)arg;
    ChatPacket packet;
    while (running) {
        int bytes = recv(sockfd, &packet, sizeof(ChatPacket), 0);
        if (bytes <= 0) {
            if (running) printf("\n[Sistema] Conexión perdida.\n");
            running = 0;
            exit(0);
        }

        switch (packet.command) {
            case CMD_MSG:
                printf("\n[%s]: %s\n> ", packet.sender, packet.payload);
                break;
            case CMD_USER_LIST:
                printf("\n[Lista] %s\n> ", packet.payload);
                break;
            case CMD_USER_INFO:
                printf("\n[Info] %s\n> ", packet.payload);
                break;
            case CMD_STATUS:
                printf("\n[Status] Tu estado actual: %s\n> ", packet.payload);
                break;
            case CMD_ERROR:
                printf("\n[ERROR] %s\n> ", packet.payload);
                break;
            case CMD_OK:
                printf("\n[OK] %s\n> ", packet.payload);
                break;
        }
        fflush(stdout);
    }
    return NULL;
}