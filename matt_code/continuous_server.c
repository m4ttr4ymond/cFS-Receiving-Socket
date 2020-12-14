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

#define INET_UDP_SERVER_PORT 8082
// Max UDP datagram size
#define SERVER_BUFFER_MAX 65535

// Need to increase these all by one to prevent cycling on 0
#define ID_TBL 0
#define ID_APP 1
#define ID_STATE 2

const int OFFSET_PT = 0;
const int OFFSET_TIME = OFFSET_PT + sizeof(unsigned char);
const int OFFSET_ID = OFFSET_TIME + sizeof(int32_t);
const int OFFSET_PNTR = OFFSET_ID + sizeof(int16_t);
const int OFFSET_LEN = OFFSET_PNTR + sizeof(int16_t);
const int OFFSET_MESSAGE = OFFSET_LEN + sizeof(int32_t);

typedef struct State_Info {
    // unsigned char packet_type;
    int32_t timestamp;
    int16_t app_id;
    int16_t pointer_offset;
    int32_t message_length;
} State_Info;


State_Info *info = NULL;
char *state = NULL, *so_file = NULL, *tbl_file = NULL;

// unsigned int clnt_addrSize = sizeof(clnt_addr);
unsigned int text_end;
int buffer_size;

int initialize_socket(int *socket_fd, struct sockaddr_in *srv_addr, struct sockaddr_in *clnt_addr);

int check_buffer(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr);

int process_incoming(char *buffer);

void uninitialize_socket(int *socket_fd);

void socket_run(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr);

int main() {
    int socket_fd; // socket file descriptor
    char buffer[SERVER_BUFFER_MAX * 3];
    struct sockaddr_in srv_addr, clnt_addr;

    if (initialize_socket(&socket_fd, &srv_addr, &clnt_addr) != 0)
    {
        printf("There was an issue initializing the socket\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        socket_run(&socket_fd, buffer, &clnt_addr);

        sleep(1);
    }

    uninitialize_socket(&socket_fd);
    return EXIT_SUCCESS; 
}

void socket_run(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr)
{
    do
    {
        printf("toc\n");
        if (buffer_size != -1)
        {
            usleep(10);
        }
        buffer_size = check_buffer(socket_fd, buffer, clnt_addr);
    } while (process_incoming(buffer));

    printf("tic\n");
}

void process_tbl(char *datagram)
{
    tbl_file = datagram + 1;
    printf("Test Table:\n");
}

void process_app(char *datagram)
{
    so_file = datagram + 1;
    printf("Test App:\n");
}

void process_state(char *datagram)
{
    // preventing memory leaks
    if(info != NULL) free(info);
    
    info = (State_Info *)malloc(sizeof(State_Info));
    info->timestamp = ntohl(*((uint32_t *)(datagram + OFFSET_TIME)));
    info->app_id = ntohs(*((uint16_t *)(datagram + OFFSET_ID)));
    info->pointer_offset = ntohs(*((uint16_t *)(datagram + OFFSET_PNTR)));
    info->message_length = ntohl(*((uint32_t *)(datagram + OFFSET_LEN)));
    
    // Need to switch the byte order? I don't think so, because the message seemed to come through fine
    state = datagram + OFFSET_MESSAGE;

    printf("Test State: %x, %x, %x, %x\n", info->timestamp, info->app_id, info->pointer_offset, info->message_length);
}

int try_send()
{
    if (state != NULL && so_file != NULL && tbl_file != NULL)
    {
        printf("SO MUCH FUCKING POWER\n");
        free(info);
        info = NULL;
        state = so_file = tbl_file = NULL;

        return 1;
    }
    else
    {
        return 0;
    }
    
}

int process_incoming(char *buffer)
{
    switch (buffer[OFFSET_PT])
    {
    case ID_TBL:
        process_tbl(buffer);
        break;
    case ID_APP:
        process_app(buffer);
        break;
    case ID_STATE:
        process_state(buffer);
        break;
    default:
        printf("None\n");
        break;
    }

    return try_send();
}

void uninitialize_socket(int *socket_fd)
{
    close(*socket_fd);
}

int initialize_socket(int *socket_fd, struct sockaddr_in *srv_addr, struct sockaddr_in *clnt_addr)
{
    memset(srv_addr, 0, sizeof(*srv_addr));
    memset(clnt_addr, 0, sizeof(*clnt_addr));

    if ((*socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("Socket Creation Failure");
        return EXIT_FAILURE;
    }

    srv_addr->sin_family = PF_INET;
    srv_addr->sin_addr.s_addr = INADDR_ANY;
    srv_addr->sin_port = htons(INET_UDP_SERVER_PORT);

    if (bind(*socket_fd, (const struct sockaddr *)srv_addr, sizeof(*srv_addr)) < 0)
    {
        perror("Binding Failure");
        return EXIT_FAILURE;
    }

    return 0;
}

int check_buffer(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr)
{
    unsigned int clnt_addrSize = sizeof(*clnt_addr);
    return recvfrom(*socket_fd, (char *)buffer, SERVER_BUFFER_MAX, MSG_DONTWAIT, (struct sockaddr *)clnt_addr, &clnt_addrSize);
}