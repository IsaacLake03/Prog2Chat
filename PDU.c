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


int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData){
    int sent = 0;
    int packetLen = 0;
    packetLen = htons(lengthOfData);
    u_char* packetBuf = (u_char*)malloc(lengthOfData+2);
    memcpy(packetBuf, &packetLen, 2);
    memcpy(packetBuf+2, dataBuffer, lengthOfData);
    sent =  safeSend(clientSocket, packetBuf, packetLen+2, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
    free(packetBuf);

    return 0;
}

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize){
    int received = 0;
    int packetLen = 0;

    // Grab the packet length
    u_char* packetBuf = (u_char*)malloc(2);
    received = safeRecv(socketNumber, packetBuf, 2, 0);
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
    received = safeRecv(socketNumber, packetBuf, packetLen, 0);
    if (received < 0)
    {
        perror("recv call");
        exit(-1);
    }
    memcpy(dataBuffer, packetBuf, packetLen);
    return 0;
}