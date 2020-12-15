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

#include "receiving_socket.h"

int main()
{
    char buffer[MAX_BURST_LEN];
    Sockets socket_data;
    Incoming_Data incoming_data;

    if (initialize_socket(&socket_data, &incoming_data) != 0)
    {
        printf("There was an issue initializing the socket\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        int val = socket_run(&socket_data, buffer, &incoming_data);

        if(val)
        {
            printf("State_len: %d, Table_len: %d, App_len: %d, Msg_len: %d\n", incoming_data.state.len, incoming_data.sch.len, incoming_data.app.len, incoming_data.msg.len);
            printf("Test State: %x, %x, %x, %x\n", incoming_data.state.timestamp, incoming_data.state.app_id, incoming_data.state.pointer_offset, incoming_data.state.len);
            destroy(&incoming_data);
        }

        sleep(1);
    }

    uninitialize_socket(&socket_data);
    return EXIT_SUCCESS;
}
