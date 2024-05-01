#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 256

void error(char *msg) {
    perror(msg);
    exit(1);
}

// https://stackoverflow.com/questions/19201689/naming-threads-using-typedef-struct 
// I used this source to get an understanding of how to create different threads and store variables for each of them
typedef struct {
    int client_socket;
} ThreadArgs;

bool running = true; 

void *client_handler(void *arg) {
    // get the thread arguments provided by the pthread_create call
    ThreadArgs *threadArgs = (ThreadArgs *)arg;
    int client_socket = threadArgs->client_socket;
    char buffer[BUFFER_SIZE];

    // gets the current thread id
    pthread_t tid = pthread_self();

    // send welcome message to the client
    sprintf(buffer, "Server session started.\nUse \"kill\" to exit session, \"killserver\" to kill server \n");
    int n = write(client_socket, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    // this loop runs until it is broken by the kill command
    while (1) {
        // https://cplusplus.com/reference/cstdio/snprintf/ 
        // this source helped us understand how to write the thread id number into a variable
        int num = snprintf(NULL, 0, "%ld", tid);
        // creates a array which will fit the number
        char charID[num + 2];
        // prints the thread number with $ onto the charID buffer
        sprintf(charID, "%ld$", tid);

        // write the child process id each time
        n = write(client_socket, charID, num + 2);
        if (n < 0)
            error("ERROR writing to socket");
        
        // clears the buffer and reads the user's input
        bzero(buffer, BUFFER_SIZE);
        n = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (n < 0)
            error("ERROR reading from socket");
        
        // prints out what the client's thread id along with the message sent 
        printf("Client ID %ld sent: %s", pthread_self(), buffer);

        // Check if the received message is "kill" to terminate the server
        if (strcmp(buffer, "kill\n") == 0) {
            // kill the child thread
            printf("Received 'kill' command. Terminating client connection.\n");
            
            // send kill back to the client to ensure that it terminates when it wants to leave
            sprintf(buffer, "kill\n");
            n = write(client_socket, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            break;
        }

        // Check if the received message is "killserver" to terminate the server
        if (strcmp(buffer, "killserver\n") == 0) {
            printf("Received 'killserver' command. Terminating server.\n");
            // kill the parent but leave the child process running until the child receives the kill command
            running = false;
        }

        // write the message back to the client if it was not a kill or killserver message 
        n = write(client_socket, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");
        
        // including this sleep ensured that the thread id is repeadtly sent back to the user until the kill command
        // if this was not included, it would take like 5 minutes to send the thread id back after recieving the first write
        sleep(1);
    }
    // close the client socket when finished and exit thread
    close(client_socket);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (running) {
        // accept new connection while server is running
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        
        // create a new thread and arguments for the thread
        pthread_t tid;
        ThreadArgs args;
        args.client_socket = newsockfd;

        // if we cannot create a thread indicate that a thread cannot be created
        // if a thread is created, it is handled in the client handler method which is the thread function
        if (pthread_create(&tid, NULL, client_handler, (void *)&args) != 0)
            error("ERROR creating thread");
    }

    close(sockfd);
    return 0;
} 