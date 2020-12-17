# cFS Receiving Socket Documentation

## Intro

The cFS receiving socket is an importable header file that provides a simple interface for handling the receiving of UDP datagrams from the network switch. There are 4 datagrams received:

1. The new state and metadata
   1. The app name as a character array
   2. The entrypoint for the app as a character array
   3. The stack size
   4. The priority of the application
   5. The state of the app that will be started on the given instance of cFS. The exact format is not important, as it is not handled by this section
2. The new schedule table
3. The new `.so` file for the given app
4. The new message table

As datagrams are received, they are assigned to a struct that holds all of the data together in one place.

### Format

The state has a special format (more than just a single file/datas structure), which is as follows:

| Data                         | Size                         |
| ---------------------------- | ---------------------------- |
| Length of app name           | 1 byte                       |
| App name (char array)        | Variable                     |
| Length of app entry point    | 1 byte                       |
| App entry point (char array) | Variable                     |
| Stack size                   | 4 bytes                      |
| Priority                     | 2 bytes                      |
| State                        | Variable (length calculated) |

## Assumptions

During development, we make several assumptions about how the UDP datagrams will behave.

1. They may arrive in any order
2. There will be no dropped datagrams
3. There will be no duplicated datagrams
4. Some datagrams may arrive much later than others
5. Datagrams for one state update will not be interleaved with datagrams for another state update. This 
6. States, tables, and the `.so` file each fit into one datagram.
7. No more than one set of datagrams will be in the buffer at once. This assumption is to reduce memory overhead, so the number of datagrams that can be held can be increased by setting the buffer length to some multiple of `MAX_BURST_LEN` (the maximum length that 4 UDP packets could have)

Assumptions 1, 2, 3, and 5 simplify the programming for our proof-of-concept, as these are not novel problems and have well-established solutions. Therefore, we do not believe that they are a good use of time in this research project.

## Use



## Customizing