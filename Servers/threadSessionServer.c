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

typedef struct {
    int client_socket;
    bool *killed;
} ThreadArgs;

void *client_handler(void *arg) {
    ThreadArgs *threadArgs = (ThreadArgs *)arg;
    int client_socket = threadArgs->client_socket;
    bool *killed = threadArgs->killed;
    char buffer[BUFFER_SIZE];

    // gets the current thread id
    pthread_t tid = pthread_self();
    printf("Thread ID %ld started.\n", tid);

    // Send welcome message to the client
    sprintf(buffer, "Server session started.\nUse \"kill\" to exit session, \"killserver\" to kill server \n");
    int n = write(client_socket, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    // Communication loop
    while (1) {
        int num = snprintf(NULL, 0, "%ld", tid);
        char charID[num + 2];
        sprintf(charID, "%ld$", tid);

        // write the child process id each time
        n = write(client_socket, charID, num + 2);
        if (n < 0)
            error("ERROR writing to socket");

        bzero(buffer, BUFFER_SIZE);
        n = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (n < 0)
            error("ERROR reading from socket");

        printf("Client ID %ld sent: %s", pthread_self(), buffer);

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

        sleep(1);
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

    while (!killed) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        pthread_t tid;
        ThreadArgs args;
        args.client_socket = newsockfd;
        args.killed = &killed;

        if (pthread_create(&tid, NULL, client_handler, (void *)&args) != 0)
            error("ERROR creating thread");
    }

    close(sockfd);
    return 0;
} 