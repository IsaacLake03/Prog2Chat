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

//Max buf was 1024 but we allow for messages of up to 1400 bytes
#define MAXBUF 1400
#define DEBUG_FLAG 1
#define maxPacketSize 200
#define MAX_TXT_SIZE (200-1-headerLen)

void checkArgs(int argc, char * argv[]);
void clientControl(int clientSocket);
void processStdin(int clientSocket);
int readFromStdin(uint8_t * buffer);
void processMsgFromServer(int serverSocket);
uint8_t getFlag(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket);
uint8_t getHandleFromStr(uint8_t * buffer, uint8_t * handle);
void setName(uint8_t * argv[], int clientSocket);
void sendBroadcast(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket);
void sendTxt(int16_t remainingLength, uint8_t * sendBuff,uint8_t * buffer, 
			uint16_t bufferLength, uint8_t clientSocket, uint8_t headerLen, uint16_t txtOffset);
void sendMessage(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket);
void sendMessageMany(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket);
void getHandleList(int clientSocket);
void readClientList(int clientSocket, uint8_t * dataBuffer);
void readText(uint8_t * dataBuffer, uint8_t * senderHandle);
void flag7read(uint8_t * dataBuffer);

