#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

uint8_t sendPDU(int socket, u_char* packet);
uint8_t recvPDU(int socket, u_char* packet);