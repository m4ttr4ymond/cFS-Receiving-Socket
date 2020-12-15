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
#define MAX_BURST_LEN SERVER_BUFFER_MAX * 3

// Need to increase these all by one to prevent cycling on 0
#define ID_STATE 1
#define ID_TBL 2
#define ID_APP 3

const int OFFSET_PT = 0;
const int OFFSET_TIME = OFFSET_PT + sizeof(unsigned char);
const int OFFSET_ID = OFFSET_TIME + sizeof(int32_t);
const int OFFSET_PNTR = OFFSET_ID + sizeof(int16_t);
const int OFFSET_LEN = OFFSET_PNTR + sizeof(int16_t);
const int OFFSET_MESSAGE = OFFSET_LEN + sizeof(int32_t);

typedef struct Data_Len_Pair
{
    char *data;
    int32_t len;
} Data_Len_Pair;

typedef struct State_Info
{
    // unsigned char packet_type;
    int32_t timestamp;
    int16_t app_id;
    int16_t pointer_offset;
    char *data;
    int32_t len;
} State_Info;

typedef struct Incoming_Data
{
    State_Info state;
    Data_Len_Pair table;
    Data_Len_Pair app;
} Incoming_Data;

typedef struct Sockets
{
    int socket_fd; // socket file descriptor
    struct sockaddr_in srv_addr;
    struct sockaddr_in clnt_addr;
} Sockets;

int buffer_size;

int socket_run(Sockets *socket_data, char *buffer, Incoming_Data *incoming_data);
void process_tbl_app(char *datagram, Data_Len_Pair *incoming_data, int32_t data_len, int type);
void process_state(char *datagram, int data_len, State_Info *info);
int is_full(Incoming_Data *incoming_data);
void destroy(Incoming_Data *incoming_data);
int process_incoming(char *buffer, int buffer_size, Incoming_Data *incoming_data);
void uninitialize_socket(Sockets *socket_data);
int initialize_socket(Sockets *socket_data);
int check_buffer(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr);

int socket_run(Sockets *socket_data, char *buffer, Incoming_Data *incoming_data)
{
    int ret_val = 0;
    do
    {
        if (buffer_size > 0)
        {
            usleep(10);
        }
        buffer_size = check_buffer(&socket_data->socket_fd, buffer, &socket_data->clnt_addr);

        if (buffer_size > 0)
        {
            ret_val = process_incoming(buffer, buffer_size, incoming_data);
        }
    } while (buffer_size > 0 && !ret_val);

    return ret_val;
}

void process_tbl_app(char *datagram, Data_Len_Pair *incoming_data, int32_t data_len, int type)
{
    if (incoming_data->len > 0)
        free(incoming_data->data);

    incoming_data->len = ntohs(*((uint16_t *)(datagram + 1)));
    incoming_data->data = (char *)malloc(incoming_data->len);
    memcpy(incoming_data->data, datagram + 3, incoming_data->len);
}

void process_state(char *datagram, int data_len, State_Info *info)
{
    if (info->len > 0)
        free(info->data);

    info->timestamp = ntohl(*((uint32_t *)(datagram + OFFSET_TIME)));
    info->app_id = ntohs(*((uint16_t *)(datagram + OFFSET_ID)));
    info->pointer_offset = ntohs(*((uint16_t *)(datagram + OFFSET_PNTR)));
    info->len = ntohl(*((uint32_t *)(datagram + OFFSET_LEN)));

    // Need to switch the byte order? I don't think so, because the message seemed to come through fine
    info->data = (char *)malloc(info->len);

    memcpy(info->data, datagram + OFFSET_MESSAGE, info->len);
}

int is_full(Incoming_Data *incoming_data)
{
    return incoming_data->state.len && incoming_data->table.len && incoming_data->app.len;
}

void destroy(Incoming_Data *incoming_data)
{
    free(incoming_data->state.data);
    free(incoming_data->table.data);
    free(incoming_data->app.data);
    incoming_data->state.len = incoming_data->table.len = incoming_data->app.len = 0;
}

int process_incoming(char *buffer, int buffer_size, Incoming_Data *incoming_data)
{
    if (buffer_size < 1)
        return 0;

    if (buffer[OFFSET_PT] == ID_STATE)
    {
        process_state(buffer, buffer_size, &incoming_data->state);
    }
    else if (buffer[OFFSET_PT] == ID_APP)
    {
        process_tbl_app(buffer, &incoming_data->app, buffer_size, buffer[OFFSET_PT]);
    }
    else if (buffer[OFFSET_PT] == ID_TBL)
    {
        process_tbl_app(buffer, &incoming_data->table, buffer_size, buffer[OFFSET_PT]);
    }

    return is_full(incoming_data);
}

void uninitialize_socket(Sockets *socket_data)
{
    close(socket_data->socket_fd);
}

int initialize_socket(Sockets *socket_data)
{
    memset(&socket_data->srv_addr, 0, sizeof(socket_data->srv_addr));
    memset(&socket_data->clnt_addr, 0, sizeof(socket_data->clnt_addr));

    if ((socket_data->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("Socket Creation Failure");
        return EXIT_FAILURE;
    }

    (&socket_data->srv_addr)->sin_family = PF_INET;
    (&socket_data->srv_addr)->sin_addr.s_addr = INADDR_ANY;
    (&socket_data->srv_addr)->sin_port = htons(INET_UDP_SERVER_PORT);

    if (bind(socket_data->socket_fd, (const struct sockaddr *)(&socket_data->srv_addr), sizeof(socket_data->srv_addr)) < 0)
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