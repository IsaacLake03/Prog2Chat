



#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc,char* argv[]){ 

    // Create a socket
    int ServerSock = socket(AF_INET6, SOCK_STREAM, 0);
    if(ServerSock < 0){
        perror("Error creating socket\n");
        return -1;
    }

    // Bind the socket
    struct sockaddr_in6 address = {0};
    address.sin6_family = AF_INET6; // IPv4/IPv6
    address.sin6_addr = in6addr_any; // This machines IP
    address.sin6_port = htons(0); // Random port

    if(bind(ServerSock, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("Error binding socket\n");
        return -1;
    }

    // Listen for incoming connections
    listen(ServerSock, 5);

    // Accept a connection
    struct sockaddr_in6 client_address = {0};
    socklen_t client_address_len = sizeof(client_address);
    int client_sock = accept(ServerSock, (struct sockaddr *)&client_address, &client_address_len);
}