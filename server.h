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

#define MAXBUF 1400
#define DEBUG_FLAG 1
#define MAX_HANDLE_LEN 100

typedef struct {
    int socketNumber;
    char handle[MAX_HANDLE_LEN];
} Client;

int checkArgs(int argc, char *argv[]);
void serverControl(int mainServerSocket, Client* clients);
void addNewSocket(int mainServerSocket, Client* clients);
void processClient(int clientSocket, Client* clients);
Client* getClientBySocket(int socket, Client* clients);
Client* getClientByHandle(char* handle, Client* clients);
void AssignHandle(int clientSocket, char* handle, uint8_t handleLen, Client* clients);
void SendHandleList(int clientSocket, Client* clients);
void broadcastMessage(char* message, int senderSocket, Client* clients);
void getHandleFromMessage(char* message, char* handle);
void sendMessage(char* message, int senderSocket, uint8_t messageLen, Client* clients);
