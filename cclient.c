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

int main(int argc, char * argv[]) {
	int clientSocket = 0;         //Socket descriptor

	checkArgs(argc, argv);

	/// Set up the TCP Client socket ///
	clientSocket = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);

	/// Set up polling ///
	setupPollSet();
	addToPollSet(clientSocket);
	addToPollSet(STDIN_FILENO);
	
	while (1) {
		clientControl(clientSocket);

	}
	
	close(clientSocket);
	return 0;
}

/* Esnure the correct # of parameters were passed. Terminate otherwise. */
void checkArgs(int argc, char * argv[]) {
	/// Check command line arguments  ///
	if (argc != 3) {
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);

	}
}

/* Client control logic: Read/send from stdin or handle server disconnect */
void clientControl(int clientSocket) {
	int pollResult;

	pollResult = pollCall(3500);

	if (pollResult == STDIN_FILENO) {			//Available message in stdin
		processStdin(clientSocket);				//Read client's messages

	} else if (pollResult == clientSocket) {	//Server connection closed
		processMsgFromServer(clientSocket);		//Terminate client socket

	}
}

/* Reads input from stdin, converts into a PDU and sends to the server */
void processStdin(int clientSocket) {
	uint8_t inputBuffer[MAXBUF];
	int bytesSent = 0;
	int bufferLength = 0;
	
	bufferLength = readFromStdin(inputBuffer);						//Read from Stdin
	printf("Reading: %s\nString length: %d (including null)\n", inputBuffer, bufferLength);
	
	bytesSent = sendPDU(clientSocket, inputBuffer, bufferLength);	//Send message to server

	if (bytesSent < 0) {											//ERROR DETECTION
		perror("send call");
		exit(-1);

	}
	printf("Amount of data bytesSent is: %d\n\n", bytesSent);
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
	printf("Server has terminated\n");
	close(clientSocket);					//Close client socket
	exit(1);
}
