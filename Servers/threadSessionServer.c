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
#include <limits.h> 

#define BUFFER_SIZE 256

void error(char *msg) {
    perror(msg);
    exit(1);
}

int getNumChar (int n) {
    if (n < 0) return getNumChar ((n == INT_MIN) ? INT_MAX: -n);
    if (n < 10) return 1;
    return 1 + getNumChar (n / 10);
} 

typedef struct {
    int client_socket;
    bool *killed;
} ThreadArgs;

void *client_handler(void *arg) {
    ThreadArgs *threadArgs = (ThreadArgs *)arg;
    int client_socket = threadArgs->client_socket;
    bool *killed = threadArgs->killed;
    char buffer[BUFFER_SIZE];

    // Send welcome message to the client
    sprintf(buffer, "Server session started.\nUse \"kill\" to exit session, \"killserver\" to kill server \n");
    int n = write(client_socket, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    // Communication loop
    while (1) {
         // gets the current child process id
        int id = getpid();
        int num = getNumChar(id);
        char charID[num]; 
        bzero(buffer, BUFFER_SIZE);
        sprintf(buffer, "%d$", id);
                
            
        // write the child process id each time
        n = write(client_socket, buffer, num+1); 
        if(n < 0)
			error("ERROR writing to socket");

        bzero(buffer, BUFFER_SIZE);
        n = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (n < 0)
            error("ERROR reading from socket");

        printf("Client ID %d sent: %s", getpid(), buffer);

        // Check if the received message is "kill" to terminate the server
        if (strcmp(buffer, "kill\n") == 0) {
            // kill the child thread
            printf("Received 'kill' command. Terminating client connection.\n");
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
            *killed = true;
            break;
        }

        n = write(client_socket, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");
    }

    close(client_socket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    bool killed = false;
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

    pthread_t tid; // Declare pthread_t variable outside the loop

        while (1) {
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");

            ThreadArgs args;
            tid = pthread_create(&tid, NULL, client_handler, (void *)&args);
            args.client_socket = newsockfd;
            args.killed = &killed;
            
            // Create a new thread only if the server has not been killed
            if (!killed && tid != 0)
                error("ERROR creating thread");

            if (killed) {
                printf("Server has been killed. Terminating...\n");
                break;
            }
        }

    close(sockfd);
    return 0;
}
