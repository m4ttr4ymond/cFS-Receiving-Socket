// UDP Client: starting later + ending earlier; sending the request and then waiting for the response
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define INET_UDP_SERVER_PORT 8081
#define CLIENT_BUFFER_MAXLINE 1024

int main(int argc, char *argv[]) {
    int socketFD, clientPort;
    unsigned int clientAddrSize;
    char buffer[CLIENT_BUFFER_MAXLINE];
    char *greetingPrefix = "A greeting from C Client Socket";
    struct sockaddr_in serverAddr, clientAddr;

    if (argc > 3)  {
        fprintf(stderr, "Wrong Client Usage: exe [client msg] [client port]\n");
        exit(EXIT_FAILURE);
    }
    // Filling server information
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(INET_UDP_SERVER_PORT);

    // Creating socket file descriptor
    if (( socketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0) {
        perror("Client Socket Creation Failure");
        exit(EXIT_FAILURE);
    }

    // Bind the socket with the client address: implicit bind if port 0 
    if (argc == 3) clientPort = strtol(argv[2], NULL, 10);
    else clientPort = 0;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(clientPort);

    if (bind(socketFD, (const struct sockaddr *) &clientAddr, sizeof(clientAddr)) < 0) {
        perror("Client Binding Failure");
        exit(EXIT_FAILURE);
    }
    if (argc < 3) {
        clientAddrSize = sizeof(clientAddr);
        getsockname(socketFD, (struct sockaddr *) &clientAddr, &clientAddrSize);
    }
    printf("Expected Client Port: %d\n", ntohs(clientAddr.sin_port));

    unsigned int textEnd, serverAddrSize;
    serverAddrSize = sizeof(serverAddr);

    memset(buffer, 0, CLIENT_BUFFER_MAXLINE);
    if (argc == 1) snprintf(buffer, CLIENT_BUFFER_MAXLINE,
        "%s - Port %d", greetingPrefix, ntohs(clientAddr.sin_port));
    else snprintf(buffer, CLIENT_BUFFER_MAXLINE,
        "%s - Port %d", argv[1], ntohs(clientAddr.sin_port));

    sendto(socketFD, (const char *) buffer, strlen(buffer), 0,
        (const struct sockaddr *) &serverAddr, serverAddrSize);
    printf("Client: the message \'%s\' sent.\n", buffer);

    textEnd = recvfrom(socketFD, (char *) buffer, CLIENT_BUFFER_MAXLINE, MSG_WAITALL,
        (struct sockaddr *) &serverAddr, &serverAddrSize);
    buffer[textEnd] = '\0';
    printf("Server: %s\n", buffer);

    close(socketFD);
    return 0;
}
