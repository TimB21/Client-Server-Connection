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
#include <limits.h>

#define BUFFER_SIZE 256

void error(char *msg) {
    perror(msg);
    exit(1);
}

// https://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c
// simple helper method to get number of characters for the id
int getNumChar(int n) {
    if (n < 0) return getNumChar((n == INT_MIN) ? INT_MAX : -n);
    if (n < 10) return 1;
    return 1 + getNumChar(n / 10);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    int clients = 0;

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

    char killMessage[] = "kill\n";
    char killserver[] = "killserver\n";

    // keeps track of the server being killed
    bool killed = false;

    while (1) {
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        // accept new client connection if server is not killed
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
                error("ERROR on accept");
        
        clients++; 

        // fork new process
        int pid = fork();
        if (pid == -1) {
            printf("Cannot fork child process");
        } else if (pid == 0) {
            // We found this while searching how allow the child to keep running after the parent process terminates
            // https://stackoverflow.com/questions/61622290/process-child-creating-a-new-session
            // https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-setsid-create-session-set-process-group-id
            // creates a new session to allow client to run
            setsid();

            // close the listening socket in the child process
            close(sockfd);

            // print the startup message and instructions on kill/killserver to the buffer
            sprintf(buffer, "Server session started.\nUse \"kill\" to exit session, \"killserver\" to kill server \n");
            
            // write the buffer to the new sock
            n = write(newsockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");

            // new client was created and will run until it is killed by break condition
            while (1) {
                // gets the current child process id
                int id = getpid();
                // get number of characters allocated to the process id
                int num = getNumChar(id);
                // create a char array based on the number of characters in the child process id
                char charID[num];
                // clears the buffer and prints the process id with the $ proceeding
                bzero(buffer, BUFFER_SIZE);
                sprintf(buffer, "%d$", id);

                // write the child process id each time
                n = write(newsockfd, buffer, num + 1);
                if (n < 0)
                    error("ERROR writing to socket");

                // clear the buffer
                bzero(buffer, BUFFER_SIZE);
                // read from the buffer
                n = read(newsockfd, buffer, BUFFER_SIZE - 1);
                if (n < 0)
                    error("ERROR reading from socket");

                // prints out the client id to the server
                printf("Client ID %d sent: %s", getpid(), buffer);

                // check if the received message is "kill" to terminate the server
                if (strcmp(buffer, killMessage) == 0) {
                    // kill the child process
                    printf("Received 'kill' command. Terminating client connection.\n");

                    // print onto the buffer the kill message
                    sprintf(buffer, "kill\n");
                    // send the kill command back to the client
                    n = write(newsockfd, buffer, strlen(buffer));
                    if (n < 0)
                        error("ERROR writing to socket");

                    break;
                }

                // check if the received message is "kill" to terminate the server
                if (killed == false && strcmp(buffer, killserver) == 0) {
                    printf("Received 'kill' command. Terminating server.\n");
                    // https://aljensencprogramming.wordpress.com/2014/05/15/the-kill-function-in-c/
                    // Send signal to parent process to terminate it
                    kill(getppid(), SIGKILL);
                    killed = true;
                }

                // echo the user's message back to the client
                n = write(newsockfd, buffer, BUFFER_SIZE - 1);
                if (n < 0)
                    error("ERROR writing to socket");
            }
            // close the socket in the child process
            close(newsockfd);
            // exit the child process
            exit(0);
        }
        // close the socket in the parent process after child has finished running
        close(newsockfd);
    }

    // close the listening socket in the parent process
    close(sockfd);
    return 0;
}