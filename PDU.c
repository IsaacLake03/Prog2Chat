#include "PDU.h"

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData){
    int bytesSent = 0;
    //calculate PDU length and make big enough PDU buffer
    uint16_t lengthOfPDU = lengthOfData + 2;
    uint8_t PDU[lengthOfPDU];
    //build PDU
    uint16_t lenPDUNetOrder = htons(lengthOfPDU);
    memcpy(PDU, &lenPDUNetOrder, 2);
    memcpy(PDU + 2, dataBuffer, lengthOfData);
    //send PDU
    bytesSent = safeSend(clientSocket, PDU, lengthOfPDU, 0);
    return bytesSent; //return num bytes sent (length of PDU)
}

/**
 * recv() the PDU and pass back the dataBuffer
 * @param socketNumber server socket number
 * @param dataBuffer buffer for PDU
 * @param bufferSize size of buffer for PDU
 * @return data bytes received
 */
int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize){
    int16_t PDUlenNetOrder, PDUlenHostOrder;
    //get message length
    int bytesReceived = safeRecv(socketNumber, dataBuffer, 2, MSG_WAITALL);
    if (bytesReceived == 0) //check for closed connection
    {
        return bytesReceived;
    }
    memcpy(&PDUlenNetOrder, dataBuffer, 2);
    PDUlenHostOrder = ntohs(PDUlenNetOrder);
    //get message
    if (bufferSize < PDUlenHostOrder) {  //Exit if buffer is not large enough to receive PDU
        printf("Buffer Error\n");
        exit(-1);
    }
    bytesReceived = safeRecv(socketNumber, dataBuffer + 2, PDUlenHostOrder - 2, MSG_WAITALL);
    //return message length (== 0 if connection closed)
    return bytesReceived ;
}
