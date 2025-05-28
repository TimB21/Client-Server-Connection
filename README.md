# Server-Client Connection Project

## Overview
The goal of this project was to implement multiple client and server programs to facilitate communication across a network. We developed four key programs:

1. **echoServer.c**: Responds with an "echo" of each client's message, both in the server console and back to the client. Receives the command `kill` to terminate.
2. **forkSessionServer.c**: Forks a new process to handle each client connection. Child processes manage clients, while the parent process listens for new connections. Commands include `kill` (terminate a child process) and `killserver` (terminate the parent process).
3. **threadSessionServer.c**: Launches new threads using POSIX threads for each client connection. Commands `kill` and `killserver` work similarly to the fork-based server.
4. **sessionClient.c**: Connects to servers, displays startup messages for user commands, and communicates with servers until the user enters `exit`. This client works with both fork-based and thread-based servers but not with simpler echo or default servers.

## Networking Concepts

- **Ports**: 16-bit unsigned integers identifying applications on a device for internet communication.
- **IP Addresses**: Unique numeric identifiers for devices connected to the internet.
- **Sockets**: Communication endpoints defined by an IP address and a port.

Together, IP addresses route data to devices, ports direct data to the correct application, and sockets establish the communication endpoints.

## defaultServer.c & defaultClient.c

These programs demonstrate basic server-client interaction:
- The server listens for a client connection, prints incoming messages, and sends a response.
- The client prompts the user for a message, sends it to the server, and prints the server's response.
- **Similarities:** Shared buffer logic, read/write operations.
- **Differences:** The server opens a socket on a specified port and waits for clients; the client actively connects.

## echoServer.c

This server extends `defaultServer.c` by adding an infinite loop for continuous communication. It prints incoming messages and echoes them back to the client. A `kill` command terminates the server.

## sessionClient.c

Compared to `defaultClient.c`, this client program expects two initial messages from the server:
1. Instructions for kill commands.
2. The process/thread ID of the server connection.

It then enters a communication loop with the server. The client fails with simpler servers that lack these startup messages.

**Potential Application:** A file management server providing commands to access, read, modify, and delete files.

## forkSessionServer.c

- The parent process listens for clients and forks a child process for each connection.
- **`killserver` command:** Sent from a child to terminate the parent process.
- **Process vs. Thread:** Processes are independent program instances with separate memory, while threads share resources within a process.

## threadSessionServer.c

This program uses threads instead of processes:
- A new thread handles each client connection.
- A shared boolean variable tracks the `killserver` command.
- **Thread Synchronization:** Mutex locks were considered but not implemented.

**Pros and Cons:**
- Processes: Better isolation but higher overhead.
- Threads: More efficient communication but require careful synchronization.

Threads were more challenging due to unfamiliarity and synchronization requirements.

## Key C Functions Learned

- `strcmp()`: Compares two strings.
- `setsid()`: Detaches child processes from parents.
- `kill()`: Terminates a process by ID.
- `pthread_create()`: Launches new threads.
- `pthread_self()`: Retrieves the current thread's ID.

## Open Questions

- How are complex server applications architected?
- How can we establish connections without predefined host information?
- How might we implement input-dependent responses?

## Acknowledgments

Assistance was sourced from various online resources and discussions with peers regarding thread synchronization and socket programming fundamentals. Specific references are available upon request.

This project provided hands-on experience with network communication, interprocess communication, and multi-threaded server design—skills applicable to real-world applications in distributed systems and network programming.
