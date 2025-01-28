#ifndef __PDU_H__
#define __PDU_H__

// ---------- Includes ---------- //
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>         //For exit()
#include <string.h>         //For memcpy()
#include <arpa/inet.h>      //For ntohs()

#include "safeUtil.h"

// ---------- Function Prototypes ---------- //
int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize);


#endif /* INC_RNG_H_ */
