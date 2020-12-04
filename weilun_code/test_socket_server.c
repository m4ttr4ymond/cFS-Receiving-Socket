// UDP Server: starting earlier + ending later; waiting for request and then sending the response
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// #define INET_UDP_SERVER_PORT 8080 // Wei-Lun's convention
#define INET_UDP_SERVER_PORT 8081 // Matt's convention
#define SERVER_BUFFER_MAXLINE 1024

int main() {
    int socketFD; // socket file descriptor
    char IPContainer[16] = "";
    char buffer[SERVER_BUFFER_MAXLINE]; // to maintain or manipulate the text contents
    char *greetingPrefix = "a greeting from ";
    struct sockaddr_in serverAddr, clientAddr1, clientAddr2;

    int16_t AppID1, AppID2;
    int32_t AppStateTime1, AppStateTime2;

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr1, 0, sizeof(clientAddr1));
    memset(&clientAddr2, 0, sizeof(clientAddr2));

    // Creating socket file descriptor
    if (( socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0) {
        perror("Socket Creation Failure");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    serverAddr.sin_family = PF_INET;
    // INADDR_ANY: if server, bind to all the network interfaces (and their "ports")
    // INADDR_ANY: if client, bind to one default network interface (and its "port")
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(INET_UDP_SERVER_PORT);

    // Bind the socket with the server address
    if (bind(socketFD, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Binding Failure");
        exit(EXIT_FAILURE);
    }

    /* Starting the interaction with clients */
    unsigned int clientAddr1Size, clientAddr2Size, textEnd1, textEnd2;
    clientAddr1Size = sizeof(clientAddr1); // or else recvfrom would behave wrong!!!
    clientAddr2Size = sizeof(clientAddr2); // or else recvfrom would behave wrong!!!

    textEnd1 = recvfrom(socketFD, (char *) buffer, SERVER_BUFFER_MAXLINE,
        MSG_WAITALL, (struct sockaddr *) &clientAddr1, &clientAddr1Size);
    buffer[textEnd1] = '\0';
    textEnd1 += 1;

    textEnd2 = recvfrom(socketFD, (char *) (buffer + textEnd1), (SERVER_BUFFER_MAXLINE - textEnd1),
        MSG_WAITALL, (struct sockaddr *) &clientAddr2, &clientAddr2Size);
    buffer[textEnd1 + textEnd2] = '\0';
    textEnd2 += 1;

    AppID1 = ntohs(*((uint16_t *)buffer));
    AppID2 = ntohs(*((uint16_t *)(buffer + textEnd1)));
    AppStateTime1 = ntohl(*((uint32_t *)(buffer + 2)));
    AppStateTime2 = ntohl(*((uint32_t *)(buffer + textEnd1 + 2)));
    printf("Client 1: ID%04hX T%d > %s\n", AppID1, AppStateTime1, buffer + 6);
    printf("Client 2: ID%04hX T%d > %s\n", AppID2, AppStateTime2, buffer + textEnd1 + 6);

    //sendto(socketFD, (const char *) greetingPrefix, strlen(greetingPrefix), 0,
    //    (const struct sockaddr *) &clientAddr1, clientAddr1Size);
    //sendto(socketFD, (const char *) greetingPrefix, strlen(greetingPrefix), 0,
    //    (const struct sockaddr *) &clientAddr2, clientAddr2Size);

    sendto(socketFD, (const char *) (buffer + 0), (textEnd1 - 1), 0,
        (const struct sockaddr *) &clientAddr2, clientAddr2Size);
    sendto(socketFD, (const char *) (buffer + textEnd1), (textEnd2 - 1), 0,
        (const struct sockaddr *) &clientAddr1, clientAddr1Size);
    inet_ntop(AF_INET, &clientAddr1.sin_addr, IPContainer, 16);
    printf("Client 1's Addr: %s:%d\n", IPContainer, ntohs(clientAddr1.sin_port));
    inet_ntop(AF_INET, &clientAddr2.sin_addr, IPContainer, 16);
    printf("Client 2's Addr: %s:%d\n", IPContainer, ntohs(clientAddr2.sin_port));
    printf("The two messages are exchanged!\n");

    close(socketFD);
    return 0; 
}
