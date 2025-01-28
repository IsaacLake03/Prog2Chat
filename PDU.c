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
    uint16_t packetLen = 0;
    packetLen = htons(lengthOfData);
    memcpy(dataBuffer, &packetLen, 2);
    memcpy(dataBuffer+2, dataBuffer, lengthOfData);
    uint8_t sent =  send(clientSocket, dataBuffer, packetLen+2, 0);
	if (sent < 0)
	{
		perror("send fail");
		exit(-1);
	}

    return 0;
}

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize){
    // Grab the packet length
    uint16_t packetLen = recv(socketNumber, dataBuffer, 2, MSG_WAITALL);
    if (packetLen < 0)
    {
        perror("recv call");
        exit(1);
    }

    packetLen = ntohs(packetLen);

    // Grab the Message
    uint8_t received = recv(socketNumber, dataBuffer, packetLen - 2, MSG_WAITALL);
    if (received < 0)
    {
        perror("Not enough data");
        exit(-1);
    }
    return 0;
}