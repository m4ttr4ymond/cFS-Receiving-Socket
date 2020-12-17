# cFS Receiving Socket Documentation

## Intro

The cFS receiving socket is an importable header file that provides a simple interface for handling the receiving of UDP datagrams from the network switch. There are 4 datagrams received:

1. The new state and metadata
   1. The app name as a character array
   2. The entrypoint for the app as a character array
   3. The stack size
   4. The priority of the application
   5. The state of the app that will be started on the given instance of cFS. The exact format is not important, as it is not handled by this library
2. The new schedule table
3. The new `.so` file for the given app
4. The new message table

As datagrams are received, they are assigned to a struct that holds all of the data together in one place.

### Format

All datagrams contain an ID that distinguishes the type of data they contain (state = 1, schedule table = 2, app `.so` = 3, message table = 4), which occupies the first byte of the datagram. All other data follows this initial byte. The schedule table, app `.so`, and message table only contain their associated files. The data in the state datagram has a special format, which is as follows:

| Data                                         | Size                         |
| -------------------------------------------- | ---------------------------- |
| Length of app name                           | 1 byte                       |
| App name (null-terminated char array)        | Variable                     |
| Length of app entry point                    | 1 byte                       |
| App entry point (null-terminated char array) | Variable                     |
| Stack size                                   | 4 bytes                      |
| Priority                                     | 2 bytes                      |
| State                                        | Variable (length calculated) |

## Assumptions

During development, we make several assumptions about how the UDP datagrams will behave.

1. They may arrive in any order
2. There will be no dropped datagrams
3. There will be no duplicated datagrams
4. Datagrams for one state update will not be interleaved with datagrams for another state update. 
6. States, tables, and the `.so` file fit into one datagram each.
7. No more than one set of datagrams will be in the buffer at once.
7. Some datagrams may arrive much later than others

Assumptions 1-6 simplify the programming for our proof-of-concept, as these are not novel problems and have well-established solutions. Therefore, we do not believe that they are a good use of time in this research project.

Assumption 7 is a contingency that we planned for, as this one is trivial to implement.

## Use

To illustrate how to use the cFS receiving socket, we will give a coding example, then explain the various parts:

```c
#include <stdio.h>
#include "receiving_socket.h"

// This is an example application of how to use the cFS receiving socket library
int main()
{
    // allocate buffer, sockets, and incoming data. These will be controlled by the user beause they will likely want to access them at some point.
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
        // The socket is considered to fail every time it doesn't find all fo the packets, therefore returning 1 every single time except for the times that it receives packets
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
```

###  Declare Data Structures
```c
char buffer[MAX_BURST_LEN];
Sockets socket_data;
Incoming_Data incoming_data;
```

In order to do anything with the socket library, we're going to need to declare three data structures: the buffer, the socket data, and the incoming data. The buffer is used to store the incoming UDP packets, the socket_data holds all of the info for receiving sockets, and the incoming data holds all of the data that has been receiving in the state transfer.

We declare the data manually so that we can access it at any time simply by accessing the structs.

###  Initialize Sockets

```c
initialize_socket(&socket_data, &incoming_data)
```

In order to begin receiving data, we must initialize the sockets, which we do using the `initialize_socket` function. Must pass the socket data and incoming data to this function, where the socket data is populated with socket information, and the incoming data has a few initialization functions performed on it.

This functions returns 0 if the sockets were successfully initialized, and a non-zero code if not.

###  Get Data

```c
socket_run(&socket_data, buffer, &incoming_data)
```

To get incoming data, we use `socket_run`. This function will check the buffer for any data, load it if there is any, and return a value indicating the status of the data. If there is incomplete data (not all data has arrived yet in the buffer) it returns a non-zero exit code. If all the data has arrived, then the function returns 0, and all data is now available for use. It is non-blocking, and intended to be used in a loop of some kind.

###  Accessing Data

```c
printf("State_len: %d, Table_len: %d, App_len: %d, Msg_len: %d\n", incoming_data.state.len, incoming_data.sch.len, incoming_data.app.len, incoming_data.msg.len);
```


```c
printf("%s\n", incoming_data.state.app_name.data);
printf("%s\n", incoming_data.state.entrypoint.data);
```

Since all data is stored in a struct, all data is easily accessible using simple dot operators. The data has the following format:

```c
// A struct to act as a pseudo vector
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
```

###  Freeing Data

```c
destroy(&incoming_data);
```

In order to make sure that data consistency is maintained, as well as to avoid memory leaks, it is necessary to free the malloc-ed data data after use. Since the data structures are so complex, there is a function (`destroy`) that implements this for us. This function should be called before grabbing more data from the buffer.

###  Uninitializing sockets

```c
uninitialize_socket(&socket_data);
```

The `uninitialize_socket` function will release the sockets. This should be performed when we no longer want to use the sockets. After doing this, the socket will need to be reinitialized before being used again.
