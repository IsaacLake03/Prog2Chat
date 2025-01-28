#include "PDU.h"

int sendPDU(int clientSocket, uint8_t * dataBuffer, int payloadLength) {
    int netOrder_PDULength = htons(payloadLength + 2);
    uint8_t PDU[payloadLength + 2];                             //PDU size = Payload + header (2 bytes)
    
    memcpy(PDU, &netOrder_PDULength, 2);                        //Copy PDU length into PDU's header
    memcpy(PDU + 2, dataBuffer, payloadLength);                 //Copy data into the PDU's payload

    return safeSend(clientSocket, PDU, payloadLength + 2, 0);   //Returns the # of bytes sent
}

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize){
    int PDULength, hostOrder_payloadLength, 
        bytesReceived = safeRecv(socketNumber, dataBuffer, 2, MSG_WAITALL);

    /// Parse PDU ///
    if (bytesReceived == 0) {                               //Connection is closed
        return 0;

    } else if (bytesReceived > 0) {                         //Parse payload length within dataBuffer
        memcpy(&PDULength, dataBuffer, 2);                  //Copy header into PDULength

        if (bufferSize < ntohs(PDULength)) {                //ERROR DETECTION: Buffer size too small for PDU
            perror("Buffer size < PDU length");
            return -1;

        }
        
        hostOrder_payloadLength = ntohs(PDULength) - 2;                                         //Payload length = PDU length - header
        
        return safeRecv(socketNumber, dataBuffer + 2, hostOrder_payloadLength, MSG_WAITALL);    //Returns the # of bytes received excluding header
    } else {                                                //ERROR DETECTION: In case bytesReceived < 0
        perror("Reached default value for recvPDU()");
        return -1;

    }
}
