/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/
/* 
 * Terminal Cmmds: cd - Enter file; cd .. - Exit file; ls - List all files; rm fileName - Remove file
 * Relevant Cmmds: ./server
 * 
 */

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

#include "PDU.h"
#include "pollLib.h"
#include "networks.h"
#include "safeUtil.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

int checkArgs(int argc, char *argv[]);
void serverControl(int mainServerSocket);
void addNewSocket(int mainServerSocket);
void processClient(int clientSocket);

int main(int argc, char *argv[]) {
	int mainServerSocket = 0;	//socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	serverControl(mainServerSocket);
	
	/* close the sockets */
	close(mainServerSocket);

	return 0;
}

int checkArgs(int argc, char *argv[]) {
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2) {
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);

	}
	
	if (argc == 2) {
		portNumber = atoi(argv[1]);

	}
	return portNumber;
}

/// Accept clients and prints client messages ///
void serverControl(int mainServerSocket) {
	int socketNumber;

	/// Set up polling ///
	setupPollSet();
	addToPollSet(mainServerSocket);

	while (1) {
		socketNumber = pollCall(3500);

		if (socketNumber == mainServerSocket) {			//Add client to polling table
			addNewSocket(mainServerSocket);

		} else if (socketNumber > mainServerSocket) {	//Print client's message
			processClient(socketNumber);

		}
	}
}

/// Adds client's socket to polling table ///
void addNewSocket(int mainServerSocket) {
	int clientSocket;
	
	clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);		//Accept client
	addToPollSet(clientSocket);									//Add client to polling table
}

/// Print client's message ///
void processClient(int clientSocket) {
	uint8_t dataBuffer[MAXBUF];
	int messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	
	/// Print data from the client_socket ///
	if (messageLen > 0) {						//Message in buffer
		printf("Message received on socket: %d\nLength: %d\nData: %s\n\n", clientSocket, messageLen, dataBuffer + 2);

	} else if (messageLen == 0) {				//Client connection closed
		printf("Client has closed their connection\n");
		removeFromPollSet(clientSocket);
		close(clientSocket);

	} else {									//Error detection: Buffer < PDU length or default condition in recv() was reached
		exit(1);

	}
}