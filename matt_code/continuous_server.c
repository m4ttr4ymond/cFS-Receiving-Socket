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

#define INET_UDP_SERVER_PORT 8081
// Max UDP datagram size
#define SERVER_BUFFER_MAXLINE 65535

int socketFD; // socket file descriptor
char IPContainer[16] = "";
char buffer[SERVER_BUFFER_MAXLINE]; // to maintain or manipulate the text contents
char *greetingPrefix = "a greeting from ";
struct sockaddr_in serverAddr, clientAddr;

int16_t AppID1;
int32_t AppStateTime1;

unsigned int clientAddrSize = sizeof(clientAddr);
unsigned int textEnd;

int initializeSocket();

int checkBuffer();

void processIncoming();

void uninitializeSocket();

int main() {

    if(initializeSocket() != 0)
    {
        printf("There was an issue initializing the socket\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if (checkBuffer() != -1) processIncoming();
        else printf("There was no message\n");

        sleep(1);
    }

    uninitializeSocket();
    return EXIT_SUCCESS; 
}

void processIncoming()
{
    buffer[textEnd] = '\0';
    textEnd += 1;

    AppID1 = ntohs(*((uint16_t *)buffer));
    AppStateTime1 = ntohl(*((uint32_t *)(buffer + 2)));
    printf("Client 1: ID%04hX T%d > %s\n", AppID1, AppStateTime1, buffer + 6);

    sendto(socketFD, (const char *)(buffer + 0), (textEnd - 1), 0,
           (const struct sockaddr *)&clientAddr, clientAddrSize);
    inet_ntop(AF_INET, &clientAddr.sin_addr, IPContainer, 16);
    printf("Client 1's Addr: %s:%d\n", IPContainer, ntohs(clientAddr.sin_port));
}

void uninitializeSocket()
{
    close(socketFD);
}

int initializeSocket()
{
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    if ((socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("Socket Creation Failure");
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(INET_UDP_SERVER_PORT);

    if (bind(socketFD, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Binding Failure");
        return EXIT_FAILURE;
    }

    return 0;
}

int checkBuffer()
{
    return recvfrom(socketFD, (char *)buffer, SERVER_BUFFER_MAXLINE,
                    MSG_DONTWAIT, (struct sockaddr *)&clientAddr, &clientAddrSize);
}