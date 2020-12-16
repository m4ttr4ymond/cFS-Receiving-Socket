/*
 * Matt Raymond, December 2020
 * cFS Receiving Socket Library
 * 
 * EECS 587, Advanced Operating Systems
 * Final Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// The UDP port to listen on
#define INET_UDP_SERVER_PORT 8082
// Max UDP datagram size
#define SERVER_BUFFER_MAX 65535
// 4 times the max UDP size, allowing for four packets
#define MAX_BURST_LEN SERVER_BUFFER_MAX * 4

// ID for the state packet
#define ID_STATE 1
// ID for the schedule packet (schedule table)
#define ID_SCH 2
// ID for the app packet (.so file)
#define ID_APP 3
// ID for the message packet (message table)
#define ID_MSG 4

// A struct to act as a pseudo struct
typedef struct Data_Len_Pair
{
    // The data to be held, in bytes
    char *data;
    // the length of the data being held
    int32_t len;
} Data_Len_Pair;

// A struct that holds the state info that we want to pass on to the developer
typedef struct State_Info
{
    // The time that the state was sent
    int32_t timestamp;
    // The state being sent, in bytes
    char *data;
    // the length of the data being held
    int32_t len;
    // Name of the application
    Data_Len_Pair app_name;
    Data_Len_Pair entrypoint;
    uint32_t stack_size;
    uint16_t priority;
} State_Info;

// All od the data that's being sent. There is one variable per field
typedef struct Incoming_Data
{
    // The application state that has been sent
    State_Info state;
    // the schedule table
    Data_Len_Pair sch;
    // the application .so
    Data_Len_Pair app;
    // the message table
    Data_Len_Pair msg;
} Incoming_Data;

// Holds socket data so that developers don't have to worry about it
typedef struct Sockets
{
    // socket file descriptor
    int socket_fd;
    // server ip address
    struct sockaddr_in srv_addr;
    // client ip address
    struct sockaddr_in clnt_addr;
} Sockets;

int socket_run(Sockets *socket_data, char *buffer, Incoming_Data *incoming_data);
void process_tbl_app(char *datagram, Data_Len_Pair *incoming_data, int32_t data_len);
void process_state(char *datagram, int data_len, State_Info *info);
int is_full(Incoming_Data *incoming_data);
void destroy(Incoming_Data *incoming_data);
int process_incoming(char *buffer, int buffer_size, Incoming_Data *incoming_data);
void uninitialize_socket(Sockets *socket_data);
int initialize_socket(Sockets *socket_data, Incoming_Data *incoming_data);
int check_buffer(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr);

// Performs a single socketcheck and data grab (if there's any data in the buffer)
int socket_run(Sockets *socket_data, char *buffer, Incoming_Data *incoming_data)
{
    // declare variables to hold the return value and the buffer size for later
    int ret_val = 0;
    int buffer_size = 0;

    // This do loop allows the library to grab packets one at a time if they arive far apart,
    // but also allows them to be loaded quickly if there are multiple in the buffer at the same time.
    // This helps keep the buffer from overflowing
    do
    {
        // If there was anything in the buffer (only happens when looping)
        if (buffer_size > 0)
        {
            // Sleep for a short amount of time
            usleep(10);
        }

        // Check to see how much data is in the buffer
        buffer_size = check_buffer(&socket_data->socket_fd, buffer, &socket_data->clnt_addr);

        // If there's anything in the buffer
        if (buffer_size > 0)
        {
            // Process one packet and save the value to ret_val
            // 0 = success (all were loaded)
            // 1 = failure (not all were loaded)
            ret_val = process_incoming(buffer, buffer_size, incoming_data);
        }

        // if there is something in the buffer and if it hasn't gotten all of the aailable packets
        // WARN: This does not get rid of excess packets and will not handle dropped packets
        // However, Andrew said to assume that we should operate under the assumtion that no packets
        // can be dropped
    } while (buffer_size > 0 && !ret_val);

    // Return the result
    return !ret_val;
}


// Loads the message table, schedule table, and app .so
void process_tbl_app(char *datagram, Data_Len_Pair *incoming_data, int32_t data_len)
{
    // Ignores small, malformed packets
    if (data_len <= 1)
        return; 

    // If there is any data, reset free the old data
    if (incoming_data->len > 0)
        free(incoming_data->data);

    // Calculate the appropriate length
    incoming_data->len = data_len - 1;//ntohs(*((uint16_t *)(datagram + 1)));
    // malloc space for the data
    incoming_data->data = (char *)malloc(incoming_data->len);
    // memcpy data from the buffer to the new malloced array
    memcpy(incoming_data->data, datagram + 1, incoming_data->len);
}


// Loads the state from a datagram
void process_state(char *datagram, int data_len, State_Info *info)
{
    // Ignores small, malformed packets
    if (data_len <= 1)
        return;

    // If there is any data, free it
    if (info->len > 0)
        free(info->data);

    if (info->app_name.len > 0)
        free(info->app_name.data);

    if (info->entrypoint.len > 0)
        free(info->entrypoint.data);

    // Start a temp offset to be used to offset everything
    int temp_offset = 1;

    // Get the length of the app name variable
    info->app_name.len = datagram[temp_offset++];

    // malloc space for the app name variable
    info->app_name.data = (char *)malloc(info->app_name.len);
    // Copy thr string to the char array
    // TODO: need to add null character at the end
    memcpy(info->app_name.data, datagram + temp_offset, info->app_name.len);
    // Update the offset for the amount of data taken
    temp_offset += info->app_name.len;
    
    // get the length of the entrypoint name
    info->entrypoint.len = datagram[temp_offset++];
    // malloc the space for the entrypoint data
    info->entrypoint.data = (char *)malloc(info->entrypoint.len);
    // copy data to the correct location
    memcpy(info->entrypoint.data, datagram + temp_offset, info->entrypoint.len);
    temp_offset += info->entrypoint.len;
    
    // get the stack size
    info->stack_size = ntohl(*((uint32_t *)(datagram + temp_offset)));
    temp_offset += 4;
    
    // get the priority number
    info->priority = ntohs(*((uint16_t *)(datagram + temp_offset)));
    temp_offset += 2;
    

    // Gets the timestamp from the data
    info->timestamp = ntohl(*((uint32_t *)(datagram + temp_offset)));
    // 4 bytes plus skipping the app id
    temp_offset += 6;

    // Calculates the length of the data
    info->len = data_len - temp_offset;
    
    // mallocs space for the data
    info->data = (char *)malloc(info->len);
    // copy's data from the buffer to the malloced array
    memcpy(info->data, datagram + temp_offset, info->len);
}


// Checks to see if all of the data is loaded or not
int is_full(Incoming_Data *incoming_data)
{
    // Checks to see if all of the lengths are greater than 0
    return incoming_data->state.len > 0 && incoming_data->sch.len > 0 && incoming_data->app.len > 0 && incoming_data->msg.len > 0;
}


// Frees all of the data and sets values back to default
void destroy(Incoming_Data *incoming_data)
{
    // Free everything
    free(incoming_data->state.data);
    free(incoming_data->sch.data);
    free(incoming_data->app.data);
    free(incoming_data->msg.data);
    free(incoming_data->state.app_name.data);
    free(incoming_data->state.entrypoint.data);

    // Reset lengths to 0 to show that there's nothing in there
    incoming_data->state.len = 0;
    incoming_data->sch.len = 0;
    incoming_data->msg.len = 0;
    incoming_data->app.len = 0;
    incoming_data->state.app_name.len = 0;
    incoming_data->state.entrypoint.len = 0;

    // Setting pointers to NULL so that free does't throw an error if accidentaly called again
    incoming_data->state.data = NULL;
    incoming_data->sch.data = NULL;
    incoming_data->msg.data = NULL;
    incoming_data->app.data = NULL;
    incoming_data->state.app_name.data = NULL;
    incoming_data->state.entrypoint.data = NULL;
}


// Determines which type of message a packet is and retreives the data accordingly
int process_incoming(char *buffer, int buffer_size, Incoming_Data *incoming_data)
{
    // Packet is for the state
    if (buffer[0] == ID_STATE)
    {
        process_state(buffer, buffer_size, &incoming_data->state);
    }
    // Packet is for the app .so
    else if (buffer[0] == ID_APP)
    {
        process_tbl_app(buffer, &incoming_data->app, buffer_size);
    }
    // Packet is for the schedule table
    else if (buffer[0] == ID_SCH)
    {
        process_tbl_app(buffer, &incoming_data->sch, buffer_size);
    }
    // Packet is for the message table
    else if (buffer[0] == ID_MSG)
    {
        process_tbl_app(buffer, &incoming_data->msg, buffer_size);
    }

    // If it was full, then the method was successful and returns a bool
    return is_full(incoming_data);
}


// Close the socket
void uninitialize_socket(Sockets *socket_data)
{
    close(socket_data->socket_fd);
}


// Set up the socket and some basic data for receiving
int initialize_socket(Sockets *socket_data, Incoming_Data *incoming_data)
{
    // Set to 0 to initialize
    incoming_data->state.len = 0;
    incoming_data->sch.len = 0;
    incoming_data->msg.len = 0;
    incoming_data->app.len = 0;
    incoming_data->state.app_name.len = 0;
    incoming_data->state.entrypoint.len = 0;
    memset(&socket_data->srv_addr, 0, sizeof(socket_data->srv_addr));
    memset(&socket_data->clnt_addr, 0, sizeof(socket_data->clnt_addr));

    // initializes the socket
    if ((socket_data->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("Socket Creation Failure");
        return EXIT_FAILURE;
    }

    // Set up the socket data
    (&socket_data->srv_addr)->sin_family = PF_INET;
    (&socket_data->srv_addr)->sin_addr.s_addr = INADDR_ANY;
    (&socket_data->srv_addr)->sin_port = htons(INET_UDP_SERVER_PORT);

    // bind the socket to a part
    if (bind(socket_data->socket_fd, (const struct sockaddr *)(&socket_data->srv_addr), sizeof(socket_data->srv_addr)) < 0)
    {
        perror("Binding Failure");
        return EXIT_FAILURE;
    }

    // Return success
    return 0;
}

// Check to see if there's anything in the buffer
int check_buffer(int *socket_fd, char *buffer, struct sockaddr_in *clnt_addr)
{
    // variable to hold data that we don't need
    unsigned int clnt_addrSize = sizeof(*clnt_addr);
    // receive data from the socket and return the length
    return recvfrom(*socket_fd, (char *)buffer, SERVER_BUFFER_MAX, MSG_DONTWAIT, (struct sockaddr *)clnt_addr, &clnt_addrSize);
}