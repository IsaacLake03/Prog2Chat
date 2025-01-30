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
#define MAX_HANDLE_LEN 100

typedef struct {
    int socketNumber;
    char handle[MAX_HANDLE_LEN];
} Client;

int checkArgs(int argc, char *argv[]);
void serverControl(int mainServerSocket);
void addNewSocket(int mainServerSocket);
void processClient(int clientSocket);
Client* getClientBySocket(int socket);
Client* getClientByHandle(char* handle);
void AssignHandle(int clientSocket, char* handle, uint8_t handleLen);
void SendHandleList(int clientSocket);
void broadcastMessage(char* message, int senderSocket);
void getHandleFromMessage(char* message, char* handle);
void sendMessage(char* message, int senderSocket, uint8_t messageLen);

// Array of clients, prelim max of 500 clients testing shouldnt exceed 300?
Client clients[500] = {0}; 
uint16_t clientCount = 0;

int main(int argc, char *argv[]) {
    int mainServerSocket = 0;    //socket descriptor for the server socket
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
        socketNumber = pollCall(-1);

        if (socketNumber == mainServerSocket) {            //Add client to polling table
            addNewSocket(mainServerSocket);

        } else if (socketNumber > mainServerSocket) {    //Print client's message
            processClient(socketNumber);
        }
    }
}

/// Adds client's socket to polling table ///
void addNewSocket(int mainServerSocket) {
    int clientSocket;
    
    clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);        //Accept client
    addToPollSet(clientSocket);                                    //Add client to polling table
    clients[clientCount].socketNumber = clientSocket;            //Add client to client array
    clientCount++;
}

/// Print client's message ///
void processClient(int clientSocket) {
    uint8_t dataBuffer[MAXBUF];
    int messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	int flag = dataBuffer[2];
	printf("Flag: %x\n", flag);
	printf("Message length: %d\n", messageLen);

	for(int i = 0; i < messageLen+2; i++) {
		printf("%02x.", dataBuffer[i]);
	}
	printf("\n");

	if (messageLen < 3){                //Client connection closed
		char * name = "Null";
        printf("Client has closed their connection\n");
		AssignHandle(clientSocket, name, 4);
        removeFromPollSet(clientSocket);
        close(clientSocket);

		return;

    }

    // Gets name from initial message
    Client* client = getClientBySocket(clientSocket);
    if (flag == 1) {
        AssignHandle(clientSocket, (char*)(dataBuffer + 4), dataBuffer[3]);
        return;
    }

	// Handle Flags
	// Get current handle list
	if (flag==10) {
		SendHandleList(clientSocket);
		printf("Handle list sent to client %s\n", client->handle);
		return;
	}

	// Broadcast message
	if(flag == 4) {
		broadcastMessage((char*)(dataBuffer+3), clientSocket);
		printf("Broadcast message sent from client %s\n", client->handle);
		return;
	}

	if(flag == 5||flag == 6) {
		sendMessage((char*)dataBuffer, clientSocket, messageLen);
		printf("Message sent from client %s\n", client->handle);
		return;
	}

    /// Print data from the client_socket ///
    if (messageLen > 0) {                        //Message in buffer
        printf("databuffer[0] = %d\n", dataBuffer[0]);

    }
}

void AssignHandle(int clientSocket, char* handle, uint8_t handleLen) {
    // Assigns handle to client
	
	uint8_t handleExists = 0;
	u_char handleT[handleLen+1];
	memcpy(handleT, handle, handleLen+1);
	
	if(handle[handleLen] != '\0') {
		handle[handleLen] = '\0';
	}

	for (int i = 0; i < 500; i++) {
		if (strcmp(clients[i].handle, (char*)handleT) == 0 && strcmp(clients[i].handle, "Null") != 0) {
			handleExists = 1;
		}
	}
	if (handleExists == 1) {
		u_char flag[1];
		flag[0] = 3;
		sendPDU(clientSocket, flag, 1);
		return;
	}else{
		u_char flag[1];
		flag[0] = 2;
		Client* client = getClientBySocket(clientSocket);
		
		memcpy(client->handle, handle, handleLen+1);
		client->handle[handleLen+1] = '\0'; // Ensure null-termination
		if(strcmp(client->handle, "Null") != 0) {
			printf("Handle assigned to client %d: %s\n", client->socketNumber, client->handle);
		}
		sendPDU(clientSocket, flag, 1);
	}
}

Client* getClientBySocket(int socket) {
    // Returns client by socket number
    for (int i = 0; i < 500; i++) {
        if (clients[i].socketNumber == socket) {
            return &clients[i];
        }
    }
    return NULL;
}

Client* getClientByHandle(char* handle) {
    // Returns client by handle
    for (int i = 0; i < 500; i++) {
        if (strcmp(clients[i].handle, handle) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

void SendHandleList(int clientSocket) {
    // Packs Message into buffer
    for (int i = 0; i <= 500; i++) {
        if (clients[i].handle[0] != '\0') {
			uint8_t dataBuffer[MAXBUF];
    		uint8_t handleLen = strlen(clients[i].handle);
            memcpy(dataBuffer, clients[i].handle, handleLen);
            memcpy(dataBuffer + handleLen, "\n\0", 2);
			sendPDU(clientSocket, dataBuffer, handleLen+2);
        }
    }
}

void broadcastMessage(char* message, int senderSocket) {
	// Broadcasts message to all clients
	printf("Broadcasting message: %s\n", message);
	for (int i = 0; i <= 500; i++) {
		if (clients[i].handle[0] != '\0' && clients[i].socketNumber != senderSocket && strcmp(clients[i].handle, "Null") != 0){
			sendPDU(clients[i].socketNumber, (uint8_t*)message, strlen(message));
		}
	}

}

void sendMessage(char* message, int senderSocket, uint8_t messageLen) {
	u_char senderHandle[MAXBUF];
	uint8_t numHandles = message[4+message[3]];
	uint8_t handles[numHandles][MAXBUF];
	uint8_t offset = 5+message[3];
	memcpy(senderHandle, message+4, message[3]);
	senderHandle[(uint8_t)message[3]] = '\0';

	for(int i = 0; i < numHandles; i++) {
		memcpy(handles[i], message+offset+1, message[offset]);
		handles[i][(uint8_t)message[offset]] = '\0';
		offset += message[offset]+1;
	}
	for(int i = 0; i<numHandles; i++) {
		Client* dest = getClientByHandle((char*)handles[i]);
		if(dest == NULL) {
			u_char HandleNotExist[MAXBUF];
			u_char handleLen = strlen((char*)handles[i]);
			HandleNotExist[0] = 7;
			HandleNotExist[1] = handleLen;
			memcpy(HandleNotExist+2, handles[i], handleLen);
			sendPDU(senderSocket, HandleNotExist, handleLen+2);
		}else{
			printf("Message Sent to %d: %s\n", dest->socketNumber, message+offset);
			sendPDU(dest->socketNumber, (uint8_t*)message+2, messageLen);
			for(int i = 0; i < messageLen; i++) {
				printf("%02x.", message[i+2]);
			}
			printf("\n");
		}
	}
}

void getHandleFromMessage(char* message, char* handle) {
    // Gets handle from message
    int i = 0;
    while (message[i] != ' ' && message[i] != '\0') {
        handle[i] = message[i];
        i++;
    }
    handle[i] = '\0';
    printf("Handle: %s", handle);
}
