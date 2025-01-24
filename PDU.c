#include "PDU.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


uint8_t sendPDU(int socket, u_char* packet){
    int sent = 0;
    int packetLen = 0;
    packetLen = htons(strlen(packet));
    u_char* packetBuf = (u_char*)malloc(packetLen+2);
    memcpy(packetBuf, &packetLen, 2);
    memcpy(packetBuf+2, packet, packetLen);
    sent =  safeSend(socket, packetBuf, packetLen+2, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
    free(packetBuf);

    return 0;
}

uint8_t recvPDU(int socket, u_char* packet){
    int received = 0;
    int packetLen = 0;

    // Grab the packet length
    u_char* packetBuf = (u_char*)malloc(2);
    received = safeRecv(socket, packetBuf, 2, 0);
    if (received < 0)
    {
        perror("recv call");
        exit(-1);
    }
    memcpy(&packetLen, packetBuf, 2);
    packetLen = ntohs(packetLen);
    free(packetBuf);

    // Grab the Message
    packetBuf = (u_char*)malloc(packetLen);
    received = safeRecv(socket, packetBuf, packetLen, 0);
    if (received < 0)
    {
        perror("recv call");
        exit(-1);
    }
    memcpy(packet, packetBuf, packetLen);
    return 0;
}