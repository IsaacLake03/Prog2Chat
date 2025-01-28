/******************************************************************************
* myClient.c
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

void clientControl(int socketNum);
void processMsgFromServer(int socketNum);
void processStdin(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);
	setupPollSet();
    addToPollSet(socketNum);
    addToPollSet(STDIN_FILENO);
	while(1){
    	clientControl(socketNum);
	}
	close(socketNum);
	
	return 0;
}

void clientControl(int socketNum){
	int readySocket = pollCall(-1);
	printf("getting here\n");
	if(readySocket == socketNum) processMsgFromServer(readySocket);
	else if(readySocket == STDIN_FILENO) processStdin(socketNum);
	else{
		perror("poll timeout");
		exit(1);
	}
}

void processMsgFromServer(int socketNum){
    uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;

	
	//now get the data from the server socket
	if((messageLen = recvPDU(socketNum, dataBuffer, MAXBUF)) > 0){
		printf("Message received on socket: %d, length: %d Data: %s\n", socketNum, messageLen, dataBuffer);
	}
	else
	{
        close(socketNum);
        removeFromPollSet(socketNum);
        //remove socket from handle table in P2
		printf("Server terminated\n");
        exit(1);
	}
}

void processStdin(int socketNum){
    uint8_t sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
	sendLen = readFromStdin(sendBuf);
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	sent = sendPDU(socketNum, sendBuf, sendLen);
	if (sent > 0)
	{
		printf("Amount of data sent is: %d\n", sent);
	} 
    else{
        printf("Server terminated\n");
        exit(1);
    }
}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	//printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}