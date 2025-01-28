/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "pollLib.h"
#include "networks.h"
#include "safeUtil.h"
#include "PDU.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void serverControl(int mainServerSocket);
void addNewSocket(int readySocket);
void processClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	//int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);
	setupPollSet();
	addToPollSet(mainServerSocket);
	while(1){
		serverControl(mainServerSocket);
	}
	/* close the sockets */
	close(mainServerSocket);

	return 0;
}

void serverControl(int mainServerSocket){
    int readySocket = pollCall(-1);
    if(readySocket == mainServerSocket){ 
		addNewSocket(readySocket);
    }else if(readySocket < 0){
        perror("poll timeout");
        exit(1);
    }
    else{processClient(readySocket);}
}

void addNewSocket(int readySocket){
    int newSocket = tcpAccept(readySocket, DEBUG_FLAG);
    addToPollSet(newSocket);
}

void processClient(int clientSocket){
	int messageLen = 0;
    printf("getting here guys\n");
    uint8_t dataBuffer[MAXBUF];
	//now get the data from the client_socket
	printf("getting here guys still\n");
    messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	printf("%d\n", messageLen);
	if(messageLen > 0){
		printf("Message received on socket: %d, length: %d Data: %s\n", clientSocket, messageLen, dataBuffer);
        // uint8_t repeatMSG[messageLen];
        // memcpy(repeatMSG, dataBuffer, messageLen);
        // int sent = sendPDU(clientSocket, repeatMSG, messageLen);
        // if (sent > 0)
        // {
        //     printf("Amount of data sent is: %d\n", sent);
        // } 
        // else {
        //     printf("Client has terminated\n");
        //     exit(1);
        // }
	}
	else
	{
        printf("Connection closed by other side\n");
        close(clientSocket);
        removeFromPollSet(clientSocket);
        //remove socket from handle table in P2
	}
}


int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}
