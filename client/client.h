#ifndef CLIENT_H
#define CLIENT_H
#include <stdint.h>
#include "protocol.h"
extern int sockfd;
extern char my_username[32];
extern int running;
void send_packet_client(uint8_t command, const char* target, const char* payload);
#endif