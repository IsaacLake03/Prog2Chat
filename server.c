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
void AssignHandle(int clientSocket, char* handle);
void SendHandleList(int clientSocket);
void broadcastMessage(char* message, int senderSocket);
void getHandleFromMessage(char* message, char* handle);
void sendMessage(char* message, int senderSocket, char* handle);

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

    // Assumes First message sent is the clients handle
    Client* client = getClientBySocket(clientSocket);
    if (client->handle[0] == '\0' || strcmp(client->handle, "Null") == 0) {
        AssignHandle(clientSocket, (char*)(dataBuffer + 2));
        return;
    }

	// Handle Flags
	if(*(dataBuffer + 2) == '%' || *(dataBuffer + 2) == '%') {
		// Get current handle list
		if (*(dataBuffer + 3) == 'l' || *(dataBuffer + 3) == 'L') {
			SendHandleList(clientSocket);
			printf("Handle list sent to client %s\n", client->handle);
			return;
		}

		// Broadcast message
		if(*(dataBuffer + 3) == 'b' || *(dataBuffer + 3) == 'B') {
			broadcastMessage((char*)(dataBuffer + 5), clientSocket);
			printf("Broadcast message sent from client %s\n", client->handle);
			return;
		}

		if(*(dataBuffer + 3) == 'm' || *(dataBuffer + 3) == 'M') {
			char* destHandle = malloc(MAX_HANDLE_LEN);
			getHandleFromMessage((char*)(dataBuffer + 5), (char*)destHandle);
			sendMessage((char*)(dataBuffer + 4 + strlen(destHandle) + 2), clientSocket, destHandle);
			free(destHandle);
			printf("Message sent from client %s\n", client->handle);
			return;
		}

	if (*(dataBuffer + 3) == 'c' || *(dataBuffer + 3) == 'C') {
		
		uint8_t numHandles = dataBuffer[5]-'0';
		char* handles[numHandles];
		int offset = 7;
		for (int i = 0; i < numHandles; i++) {
			handles[i] = malloc(MAX_HANDLE_LEN);
			getHandleFromMessage((char*)(dataBuffer + offset), handles[i]);
			offset += strlen(handles[i]) + 1;
		}
		printf("Num handles: %d\n", numHandles);
		for (int i = 0; i < numHandles; i++) {
			sendMessage((char*)(dataBuffer + offset), clientSocket, handles[i]);
		}
		for (int i = 0; i < numHandles; i++) {
			free(handles[i]);
		}
		printf("Message sent from client %s to multiple handles\n", client->handle);
		return;
	}
	}

    /// Print data from the client_socket ///
    if (messageLen > 0) {                        //Message in buffer
        printf("Message received from Client: %s, Length: %d, Data: %s\n", client->handle, messageLen, dataBuffer + 2);

    } else if (messageLen == 0){                //Client connection closed
        printf("Client has closed their connection\n");
        removeFromPollSet(clientSocket);
        close(clientSocket);
		AssignHandle(clientSocket, "Null");
		clientCount--;

    }
}

void AssignHandle(int clientSocket, char* handle) {
    // Assigns handle to client

    Client* client = getClientBySocket(clientSocket);
    strncpy(client->handle, handle, MAX_HANDLE_LEN - 1);
    client->handle[MAX_HANDLE_LEN - 1] = '\0'; // Ensure null-termination
    printf("Handle assigned to client %d: %s\n", client->socketNumber, client->handle);
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
		if (clients[i].handle[0] != '\0' && clients[i].socketNumber != senderSocket) {
			sendPDU(clients[i].socketNumber, (uint8_t*)message, strlen(message));
		}
	}

}

void sendMessage(char* message, int senderSocket, char* handle) {
	// Sends message to specific client
	Client* client = getClientByHandle(handle);
	printf("message: %s\n", message);
	char* newMessage = malloc((strlen(message) + strlen(getClientBySocket(senderSocket)->handle) + 10) * sizeof(char));
	sprintf(newMessage, "%s: %s", getClientBySocket(senderSocket)->handle, message);
	if (client != NULL) {
		sendPDU(client->socketNumber, (uint8_t*)newMessage, strlen(newMessage));
	}
	free(newMessage);
	
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
