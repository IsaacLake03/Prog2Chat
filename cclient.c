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
	clientSocket = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	/// Set up polling ///
	setupPollSet();
	addToPollSet(clientSocket);
	addToPollSet(STDIN_FILENO);

	// Send the client's handle
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
    int bytesSent = 0;
    int bufferLength = 0;
    uint8_t flag = 0; // Change u_char to uint8_t

    bufferLength = readFromStdin(inputBuffer); // Read from Stdin

    // Handle Flags
    if (*(inputBuffer) == '%') {
        // Get current handle list
        if (*(inputBuffer + 1) == 'l' || *(inputBuffer + 1) == 'L') {
            printf("Requesting handle list\n");
            flag = 10;
        }

        // Broadcast message
        if (*(inputBuffer + 1) == 'b' || *(inputBuffer + 1) == 'B') {
            printf("Broadcasting message: %s\n", inputBuffer + 3);
            flag = 4;
        }

        if (*(inputBuffer + 1) == 'm' || *(inputBuffer + 1) == 'M') {
            printf("Sending message to %s: %s\n", inputBuffer + 3, inputBuffer + 3);
            flag = 5;
        }

        if (*(inputBuffer + 1) == 'c' || *(inputBuffer + 1) == 'C') {
            printf("Sending to multiple\n");
            flag = 6;
        }
    }

    sendBuffer[0] = flag;
    memcpy(sendBuffer + 1, inputBuffer+3, bufferLength-3);
	for(int i = 0; i < bufferLength-3; i++) {
		printf(".%02x", sendBuffer[i]);
	}

    // Send the buffer
    bytesSent = send(clientSocket, sendBuffer, bufferLength - 2, 0);
    if (bytesSent < 0) {
        perror("send");
    }
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
	int messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	if(dataBuffer[2]==3) {
		printf("Handle already exists\n");
		exit(1);
	}else if(dataBuffer[2]==2) {
		printf("Handle assigned\n");
		return;
	}

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
