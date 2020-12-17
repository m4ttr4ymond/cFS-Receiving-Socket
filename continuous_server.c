/*
 * Matt Raymond, December 2020
 * cFS Receiving Socket Library Example
 * 
 * EECS 587, Advanced Operating Systems
 * Final Project
 */

#include <stdio.h>
#include "receiving_socket.h"

// This is an example application of how to use the cFS receiving socket library
int main()
{
    // allocate buffer, sockets, and incoming data. These will be controlled by the user beause they will likely
    // want to access them at some point.
    char buffer[MAX_BURST_LEN];
    Sockets socket_data;
    Incoming_Data incoming_data;

    // Initialize the socket, just like any other application
    if (initialize_socket(&socket_data, &incoming_data) != 0)
    {
        printf("There was an issue initializing the socket\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {   
        // The socket is considered to fail every time it doesn't find all fo the packets, therefore returning 1
        // every single time except for the times that it receives packets
        if (socket_run(&socket_data, buffer, &incoming_data) != 1)
        {
            // Print out the data just becase
            printf("State_len: %d, Table_len: %d, App_len: %d, Msg_len: %d\n", incoming_data.state.len, incoming_data.sch.len, incoming_data.app.len, incoming_data.msg.len);

            printf("%s\n", incoming_data.state.app_name.data);
            printf("%s\n", incoming_data.state.entrypoint.data);

            // Free all of the mallocs and reset all of the variables so we don't end up with memory leaks
            destroy(&incoming_data);
        }

        sleep(1);
    }

    // get ready to shut down
    uninitialize_socket(&socket_data);
    return EXIT_SUCCESS;
}
