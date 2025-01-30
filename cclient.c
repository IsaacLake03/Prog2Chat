/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/
/* 
 * Terminal Cmmds: cd - Enter file; cd .. - Exit file; ls - List all files; rm fileName - Remove file
 * Relevant Cmmds: ./client localhost PORTNUMBER
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

void checkArgs(int argc, char * argv[]);
void clientControl(int clientSocket);
void processStdin(int clientSocket);
int readFromStdin(uint8_t * buffer);
void processMsgFromServer(int serverSocket);
uint8_t getFlag(uint8_t * buffer, uint8_t * bufferLength);
uint8_t getHandleFromStr(uint8_t * buffer, uint8_t * handle);


u_char* userHandle;


int main(int argc, char * argv[]) {
	int clientSocket = 0;         //Socket descriptor

	checkArgs(argc, argv);

	/// Set up the TCP Client socket ///
	clientSocket = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	/// Set up polling ///
	setupPollSet();
	addToPollSet(clientSocket);
	addToPollSet(STDIN_FILENO);

	// Send the client's handle
	userHandle = (u_char*)argv[1];
	uint8_t handleBuffer[MAXBUF];
	uint8_t handlelen = strlen(argv[1]);
	handleBuffer[0] = 1;
	handleBuffer[1] = handlelen;
	memcpy(handleBuffer+2, argv[1], strlen(argv[1]));

	sendPDU(clientSocket, (uint8_t*)handleBuffer, strlen((char*)handleBuffer));

	
	while (1) {
		clientControl(clientSocket);

	}
	
	close(clientSocket);
	return 0;
}

/* Esnure the correct # of parameters were passed. Terminate otherwise. */
void checkArgs(int argc, char * argv[]) {
	/// Check command line arguments  ///
	if (argc != 4) {
		printf("usage: %s handle host-name port-number \n", argv[0]);
		exit(1);

	}
}

/* Client control logic: Read/send from stdin or handle server disconnect */
void clientControl(int clientSocket) {
	int pollResult;

	printf("$: ");
	fflush(stdout);

	pollResult = pollCall(-1);

	if (pollResult == STDIN_FILENO) {			//Available message in stdin
		processStdin(clientSocket);				//Read client's messages

	} else if (pollResult == clientSocket) {	//Server connection closed
		processMsgFromServer(clientSocket);		//Terminate client socket

	}
}

/* Reads input from stdin, converts into a PDU and sends to the server */
void processStdin(int clientSocket) {
    uint8_t inputBuffer[MAXBUF];
    uint8_t sendBuffer[MAXBUF];
    uint8_t bytesSent = 0;
    uint8_t bufferLength = 0;
    uint8_t flag = 0; // Change u_char to uint8_t

    bufferLength = readFromStdin(inputBuffer); // Read from Stdin

    // Handle Flags
    if (*(inputBuffer) == '%') {
		flag = getFlag(inputBuffer, &bufferLength);

		if (flag == 0) {
			printf("Invalid command\n");
			return;
		}

		sendBuffer[0] = flag;
		memcpy(sendBuffer + 1, inputBuffer, bufferLength+1);
		// for(int i = 0; i < bufferLength+1; i++) {
		// 	printf(".%02x", sendBuffer[i]);
		// }

		// Send the buffer
		bytesSent = sendPDU(clientSocket, sendBuffer, bufferLength+1);
		if (bytesSent < 0) {
			perror("send");
		}
    }
}

uint8_t getFlag(uint8_t * buffer, uint8_t * bufferLength) {
		// Broadcast message
	if (*(buffer + 1) == 'b' || *(buffer + 1) == 'B') {
		printf("Broadcasting message: %s\n", buffer + 3);
		return 4;
	}

	// Send message to specific client
	if (*(buffer + 1) == 'm' || *(buffer + 1) == 'M') {
		uint8_t preHeader[MAXBUF];
		uint8_t handleLen = strlen((char *)userHandle);
		uint8_t numDest = 1;
		uint8_t destHandles[1][MAXBUF];
		uint8_t destHandleLens[1];
		uint8_t offset = 1 + handleLen + 1;
		uint8_t strOffset = 3;
		preHeader[0] = handleLen;
		memcpy(preHeader+1, userHandle, handleLen);
		memcpy(preHeader+1+handleLen, &numDest, 1);

		// Get destination handle
		for(int i = 0; i < numDest; i++){
			destHandleLens[i] = getHandleFromStr(buffer+strOffset, destHandles[i]);
			strOffset += destHandleLens[i] + 1;
			memcpy(preHeader+offset, &destHandleLens[i], 1);
			offset++;
			memcpy(preHeader+offset, destHandles[i], destHandleLens[i]);
			offset += destHandleLens[i];
		}
		bufferLength[0] = offset + strlen((char *)buffer+strOffset);
		memcpy(preHeader+offset, buffer+strOffset, bufferLength[0]-offset);
		memcpy(buffer, preHeader, bufferLength[0]);
		buffer[bufferLength[0]] = '\0';
		bufferLength[0]++;
		for(uint8_t j=0; j<bufferLength[0]; j++) {
				printf(".%02x", preHeader[j]);
		}
		printf("\n");
		printf("Sending message\n");
		
		return 5;
	}

	// Send message to multiple clients
	if (*(buffer + 1) == 'c' || *(buffer + 1) == 'C') {
		const uint8_t numDest = (uint8_t)buffer[3]-'0';
		uint8_t handleOffset = 0;
		uint8_t preHeader[MAXBUF];
		uint8_t HandleList[MAXBUF];
		const uint8_t handleLen = strlen((char *)userHandle);
		uint8_t destHandles[MAXBUF][1];
		uint8_t destHandleLens[MAXBUF];
		uint8_t offset = 1 + handleLen + 1;
		uint8_t strOffset = 5;
		
		preHeader[0] = handleLen;
		memcpy(preHeader+1, userHandle, handleLen);
		preHeader[handleLen+1] = numDest;
		printf("\n");
		printf("NumDest: %d\n", numDest);

		// Get destination handle
		for(int i = 0; i < numDest; i++){
			destHandleLens[i] = getHandleFromStr(buffer+strOffset, destHandles[i]);
			strOffset += destHandleLens[i] + 1;
			memcpy(HandleList+handleOffset, &destHandleLens[i], 1);
			handleOffset++;
			offset++;
			memcpy(HandleList+handleOffset, destHandles[i], destHandleLens[i]);
			handleOffset += destHandleLens[i];
			offset += destHandleLens[i];
			printf("DestHandle: %s\n", destHandles[i]);
			printf("DestHandleLen: %d\n", destHandleLens[i]);
			printf("i: %d, NumDest: %d\n", i, numDest);
		}

		for(uint8_t i = 0; i < handleOffset; i++) {
			printf("%02x.", HandleList[i]);
		}
		printf("\n");
		bufferLength[0] = offset + strlen((char *)buffer+strOffset);
		memcpy(preHeader+handleLen+2, HandleList, handleOffset);
		memcpy(preHeader+offset, buffer+strOffset, bufferLength[0]-offset);
		memcpy(buffer, preHeader, bufferLength[0]);
		buffer[bufferLength[0]] = '\0';
		bufferLength[0]++;
		for(uint8_t j=0; j<bufferLength[0]; j++) {
				printf(".%02x", preHeader[j]);
		}
		printf("\n");
		printf("Sending to multiple\n");
		return 6;
	}

	// Get current handle list
	if (*(buffer + 1) == 'l' || *(buffer + 1) == 'L') {
		printf("Requesting handle list\n");
		return 10;
	}
	return 0;
}

/* Reads input from stdin: Ensure the input length < buffer size and null terminates the string */
int readFromStdin(uint8_t * buffer) {
	char inputChar = 0;
	int bufferIndex = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	while (bufferIndex < (MAXBUF - 1) && inputChar != '\n') {
		inputChar = getchar();
		if (inputChar != '\n') {
			buffer[bufferIndex] = inputChar;
			bufferIndex++;
		}
	}
	
	/// Null terminate the string ///
	buffer[bufferIndex] = '\0';
	bufferIndex++;
	
	return bufferIndex;
}

/* Handle server disconnect: Close the client and terminate the program */
void processMsgFromServer(int clientSocket) {
	uint8_t dataBuffer[MAXBUF];
	uint8_t senderHandle[MAXBUF];
	int messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	uint8_t flag = dataBuffer[2];
	for(uint8_t i = 0; i < messageLen+2; i++) {
		printf("%02x.", dataBuffer[i+2]);
	}
	if(messageLen < 1) {
		printf("Server has terminated\n");
		close(clientSocket);
		exit(1);
	}
	printf("\n");

	if(flag == 3) {
		printf("Handle already exists\n");
		exit(1);
	}else if(flag == 2) {
		printf("Handle assigned\n");
		return;
	}else if(flag == 7){
		uint8_t handleLen = dataBuffer[3];
		u_char Incorrect[MAXBUF];
		memcpy(Incorrect, dataBuffer+4, handleLen);
		Incorrect[handleLen] = '\0';
		printf("Error Handle not found: %s\n", Incorrect);
		return;
	}else if(flag == 4) {
		memcpy(senderHandle, dataBuffer+4, dataBuffer[3]);
		senderHandle[dataBuffer[3]] = '\0';
		printf("%s: %s\n", senderHandle, dataBuffer+4+dataBuffer[3]);
		return;
	}else if(flag == 5 || flag == 6){
		uint8_t numHandles = dataBuffer[4+dataBuffer[3]];
		uint8_t handles[numHandles][MAXBUF];
		uint8_t offset = 5+dataBuffer[3];
		memcpy(senderHandle, dataBuffer+4, dataBuffer[3]);
		senderHandle[dataBuffer[3]] = '\0';

		for(int i = 0; i < numHandles; i++) {
			memcpy(handles[i], dataBuffer+offset+1, dataBuffer[offset]);
			handles[i][dataBuffer[offset]] = '\0';
			offset += dataBuffer[offset]+1;
		}
		printf("%s: %s\n", senderHandle, dataBuffer+offset);
	}else{
		if(messageLen == 0) {
			printf("Server has terminated\n");
			close(clientSocket);					//Close client socket
			exit(1);
		} else{
			memcpy(dataBuffer+messageLen+2, "\0", 1);
			printf("%s\n", (char*)(dataBuffer+2));
			fflush(stdout);
		}
	}
}

uint8_t getHandleFromStr(uint8_t * buffer, uint8_t * handle) {
	uint8_t handleLen = 0;
	uint8_t i = 0;
	while(buffer[i] != ' ' && buffer[i] != '\0') {
		handle[i] = buffer[i];
		handleLen++;
		i++;
	}
	handle[handleLen] = '\0';
	return handleLen;
}
