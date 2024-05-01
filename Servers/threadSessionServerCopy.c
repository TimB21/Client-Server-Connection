#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFFER_SIZE 256

void error(char *msg) {
    perror(msg);
    exit(1);
}

int getNumChar(int n) {
    if (n < 0) return getNumChar((n == INT_MIN) ? INT_MAX : -n);
    if (n < 10) return 1;
    return 1 + getNumChar(n / 10);
}

// Structure to hold client connection information
typedef struct {
    int sockfd;
    int clientID;
} ClientInfo;

// Function to handle client connections
void *handle_client(void *arg) {
    ClientInfo *clientInfo = (ClientInfo *)arg;
    int sockfd = clientInfo->sockfd;
    int clientID = clientInfo->clientID;
    char buffer[BUFFER_SIZE];
    int n;

    // Print startup message and instructions
    sprintf(buffer, "Server session started.\nUse \"kill\" to exit session, \"killserver\" to kill server \n");
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    while (1) {
        // Read client message
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0)
            error("ERROR reading from socket");

        // Check client message
        if (strcmp(buffer, "kill\n") == 0) {
            printf("Client ID %d sent: kill\n", clientID);
            break;
        } else if (strcmp(buffer, "killserver\n") == 0) {
            printf("Client ID %d sent: killserver\n", clientID);
            pthread_exit(NULL);
        } else {
            // Echo client message
            printf("Client ID %d sent: %s", clientID, buffer);
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
        }
    }

    // Close connection
    close(sockfd);
    free(clientInfo);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    int clientID = 0; // Client ID counter

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        // Accept new client connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        // Create client thread
        pthread_t tid;
        ClientInfo *clientInfo = malloc(sizeof(ClientInfo));
        if (clientInfo == NULL)
            error("ERROR allocating memory");
        clientInfo->sockfd = newsockfd;
        clientInfo->clientID = ++clientID;
        if (pthread_create(&tid, NULL, handle_client, (void *)clientInfo) != 0)
            error("ERROR creating thread");
        pthread_detach(tid);
    }

    close(sockfd);
    return 0;
}
