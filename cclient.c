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

#include "cclient.h"


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
	setName((uint8_t**)argv, clientSocket);

	while (1) {
		clientControl(clientSocket);
	}
	close(clientSocket);
	return 0;
}

void setName(uint8_t * argv[], int clientSocket) {
	// Send the client's handle
	userHandle = (u_char*)argv[1];
	uint8_t handleBuffer[MAXBUF];
	uint8_t handlelen = strlen((char *)argv[1]);
	if(handlelen > 100) {
		printf("Handle too long\n");
		exit(1);
	}
	handleBuffer[0] = 1;
	handleBuffer[1] = handlelen;
	memcpy(handleBuffer+2, argv[1], handlelen);

	sendPDU(clientSocket, (uint8_t*)handleBuffer, strlen((char*)handleBuffer));
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
    //uint8_t sendBuffer[MAXBUF];
    //uint8_t bytesSent = 0;
    uint16_t bufferLength = 0;
    uint8_t flag = 0; // Change u_char to uint8_t

    bufferLength = readFromStdin(inputBuffer); // Read from Stdin
	if(bufferLength >=1400){
		printf("Message too long\n");
		return;
	}

    // Handle Flags
    if (*(inputBuffer) == '%') {
		flag = getFlag(inputBuffer, &bufferLength, clientSocket);

		if (flag == 0) {
			printf("Invalid command\n");
			return;
		}
    }else{
		printf("Invalid command\n");
	}
}

uint8_t getFlag(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket) {
		// Broadcast message
	if (*(buffer + 1) == 'b' || *(buffer + 1) == 'B') {
		sendBroadcast(buffer, bufferLength, clientSocket);
		return 4;
	}

	// Send message to specific client
	if (*(buffer + 1) == 'm' || *(buffer + 1) == 'M') {
		//printf("Sending message\n");
		sendMessage(buffer, bufferLength, clientSocket);
		return 5;
	}

	// Send message to multiple clients
	if (*(buffer + 1) == 'c' || *(buffer + 1) == 'C') {
		sendMessageMany(buffer, bufferLength, clientSocket);
		//printf("Sending to multiple\n");
		return 6;
	}

	// Get current handle list
	if (*(buffer + 1) == 'l' || *(buffer + 1) == 'L') {
		//printf("Requesting handle list\n");
		getHandleList(clientSocket);
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
	// for(uint8_t i = 0; i < messageLen; i++) {
	// 	printf("%02x.", dataBuffer[i+2]);
	// }
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
		flag7read(dataBuffer);
		return;
	}else if(flag == 4) {
		u_char senderHandle[MAXBUF];
		memcpy(senderHandle, dataBuffer+4, dataBuffer[3]);
		senderHandle[dataBuffer[3]] = '\0';
		printf("%s: %s\n", senderHandle, dataBuffer+4+dataBuffer[3]);
		return;
	}else if(flag == 5 || flag == 6){
		readText(dataBuffer, senderHandle);
		return;
	}else if(flag == 11){
		readClientList(clientSocket, dataBuffer);
		return;
	}else if(flag == 12){
		return;
	}else if(flag == 13){
		return;
	}else{
		if(messageLen == 0) {
			printf("Server has terminated\n");
			close(clientSocket);					//Close client socket
			exit(1);
		}
	}
}

void readClientList(int clientSocket, uint8_t * dataBuffer) {
	uint32_t handleCount;
	uint8_t messageLen;
	memcpy(&handleCount, dataBuffer+3, 4);
	handleCount = ntohl(handleCount);
	printf("Handle list (%d Clients Online):\n", handleCount);
	while(handleCount > 0){
		messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
		if(messageLen == 0) {
			printf("Server has terminated\n");
			close(clientSocket);					//Close client socket
			exit(1);
		}
		if(dataBuffer[2] == 13){
			printf("------------------------------\n");
			break;
		}
		u_char handle[MAXBUF];
		uint8_t handleLen = dataBuffer[3];
		memcpy(handle, dataBuffer+4, handleLen);
		handle[handleLen] = '\0';
		printf("%s\n", handle);
		handleCount--;
	}
	messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	if(messageLen == 0) {
		printf("Server has terminated\n");
		close(clientSocket);					//Close client socket
		exit(1);
	}
	if(dataBuffer[2] == 13){
		printf("------------------------------\n");
	}
}

void readText(uint8_t * dataBuffer, uint8_t * senderHandle){
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
}

void flag7read(uint8_t * dataBuffer) {
	uint8_t handleLen = dataBuffer[3];
	u_char Incorrect[MAXBUF];
	memcpy(Incorrect, dataBuffer+4, handleLen);
	Incorrect[handleLen] = '\0';
	printf("Error Handle not found: %s\n", Incorrect);
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

void sendBroadcast(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket){
	uint8_t preHeader[MAXBUF];
	uint8_t handleLen = strlen((char *)userHandle);
	uint8_t Offset = 2 + handleLen;
	uint8_t strOffset = 3;
	preHeader[0] = 4;
	preHeader[1] = handleLen;
	memcpy(preHeader+2, userHandle, handleLen);
	bufferLength[0] = strlen((char *)buffer+strOffset) + Offset;

	sendTxt(bufferLength[0], preHeader, buffer, bufferLength[0], clientSocket, Offset, strOffset);
}

void sendMessage(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket){
	uint8_t preHeader[MAXBUF];
	uint8_t handleLen = strlen((char *)userHandle);
	uint8_t numDest = 1;
	uint8_t destHandles[1][MAXBUF];
	uint8_t destHandleLens[1];
	uint8_t offset = 2 + handleLen + 1;
	uint8_t strOffset = 3;
	preHeader[0] = 5;
	preHeader[1] = handleLen;
	memcpy(preHeader+2, userHandle, handleLen);
	memcpy(preHeader+2+handleLen, &numDest, 1);

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

	sendTxt(bufferLength[0], preHeader, buffer, bufferLength[0], clientSocket, offset, strOffset);
}

void sendMessageMany(uint8_t * buffer, uint16_t * bufferLength, uint8_t clientSocket){
	const uint8_t numDest = (uint8_t)buffer[3]-'0';

	if(numDest < 2 || numDest > 9) {
		printf("Invalid number of handles\n");
		return;
	}
	
	uint8_t handleOffset = 0;
	uint8_t preHeader[MAXBUF];
	uint8_t HandleList[MAXBUF];
	const uint8_t handleLen = strlen((char *)userHandle);
	uint8_t destHandles[MAXBUF][1];
	uint8_t destHandleLens[MAXBUF];
	uint8_t offset = 2 + handleLen + 1;
	uint8_t strOffset = 5;
	
	preHeader[0] = 6;
	preHeader[1] = handleLen;
	memcpy(preHeader+2, userHandle, handleLen);
	preHeader[handleLen+2] = numDest;

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
	}

	bufferLength[0] = offset + strlen((char *)buffer+strOffset);
	memcpy(preHeader+handleLen+3, HandleList, handleOffset);

	sendTxt(bufferLength[0], preHeader, buffer, bufferLength[0], clientSocket, offset, strOffset);
}

void sendTxt(int16_t remainingLength, uint8_t * sendBuff,uint8_t * buffer, uint16_t bufferLength, uint8_t clientSocket, const uint8_t headerLen, uint16_t txtOffset){
	uint8_t txtSize = 0;
	uint8_t sendLength = 0;

	do{
		if(remainingLength>MAX_TXT_SIZE){
			txtSize = MAX_TXT_SIZE-1;
		}else{
			txtSize = remainingLength+1;
		}
		
		memcpy(sendBuff+headerLen, buffer+txtOffset, txtSize);

		sendLength = txtSize + headerLen;
		sendBuff[sendLength] = '\0';
		sendLength++;
		// for(uint8_t j=0; j<sendLength; j++) {
		// 	printf(".%02x", sendBuff[j]);
		// }
		// printf("\n");
		int8_t bytesSent = sendPDU(clientSocket, sendBuff, sendLength+1);
		if (bytesSent < 0) {
			perror("send");
		}
		txtOffset += txtSize;
		remainingLength -= txtSize;
	}while(txtOffset < bufferLength);
}

void getHandleList(int clientSocket){
	uint8_t handleList[1];
	handleList[0] = 10;
	sendPDU(clientSocket, handleList, 1);
}
