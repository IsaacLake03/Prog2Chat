#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize);