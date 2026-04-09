# Multi-threaded Database Server and Client

This project implements a socket-based database server (`dbserver`) and client (`dbclient`) in C. The server is multi-threaded: it accepts client connections continuously, creates a handler thread for each client, and processes requests concurrently.

## Overview

The project consists of two programs:

- `dbserver` — a multi-threaded database server
- `dbclient` — an interactive client that connects to the server

The server listens on a port, accepts incoming client connections, and spawns a new thread to handle each connected client. Each handler thread processes requests until that client closes the connection.

The client supports three operations:

- `put` — store a record in the database
- `get` — fetch a record by ID
- `quit` — close the connection and exit

The message format and record structure are defined in the provided `msg.h` file.

## Files

- `dbserver.c` — server implementation
- `dbclient.c` — client implementation
- `Makefile` — build automation
- `msg.h` — provided message and record definitions (not provided in repository, but see below example to understand)

## Server behavior

The server:

- creates a listening socket
- waits for client connection requests
- accepts a connection
- creates a handler thread for that client
- continues waiting for the next connection

A handler thread processes these request types:

### PUT
The client sends a record containing a name and ID.

The server:

- appends only the client data record to the database file
- sends `SUCCESS` if the write succeeds
- sends `FAIL` otherwise

### GET
The client sends an ID.

The server:

- searches the database file for a matching record
- sends `SUCCESS` with the matching record if found
- sends `FAIL` if not found

## Important notes

- Opens the database file separately inside each thread.
- Does **not** share one global file descriptor among threads.
- Does **not** need to handle inconsistencies caused by concurrent writes.
- The server is be able to handle many clients at the same time.
- Does not impose user's own fixed limit on the number of simultaneous clients.

## Compilation

### Compile the server

```bash
gcc dbserver.c -o dbserver -Wall -Werror -std=gnu99 -pthread
```

## Build with Makefile

The Makefile should support these targets:

all
clean
dbclient
dbserver

If the Makefile is set up correctly, you should be able to run:
```bash
make
```

and clean build artifacts with:
```bash
make clean
```

## How to run
1. Start the server
Run the server with a port number:
```bash
./dbserver 3648
```
3648 is just an example. You may choose another port number. If the port is already in use, pick a different one.

2. Start the client
In another terminal, connect to the server using the server DNS name and the same port number:
```bash
./dbclient cs2.utdallas.edu 3648
```
Replace cs2.utdallas.edu with the actual server machine name if needed.

## Client interaction

When the client starts, it prompts for one of three choices:

1 - put

2 - get

0 - quit

Example session
```bash
Enter your choice (1 to put, 2 to get, 0 to quit): 1
Enter the name: Abdul Wallace
Enter the id: 47678
Put success.

Enter your choice (1 to put, 2 to get, 0 to quit): 1
Enter the name: Lakshmi Wong
Enter the id: 34980
Put success.

Enter your choice (1 to put, 2 to get, 0 to quit): 2
Enter the id: 34980
name: Lakshmi Wong
id: 34980

Enter your choice (1 to put, 2 to get, 0 to quit): 0
```

To avoid typing client input manually, you can use input redirection:
```bash
./dbclient cs2.utdallas.edu 3648 < input.txt
```