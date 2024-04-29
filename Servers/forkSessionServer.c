#include <stdlib.h> // for exit
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h> // for bzero
#include <unistd.h> // for read and write

#define BUFFER_SIZE 256

void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno, clilen;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	if(argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	bzero((char *) &serv_addr, sizeof(serv_addr)); // write zero-valued bytes to variable
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno); // host-to-network byte order conversion
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	// Listen for connections
    listen(sockfd, MAX_PENDING_CONNECTIONS);
    clilen = sizeof(cli_addr);

    while (1) {
        // Accept connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        // Fork child process
        pid_t pid = fork();
        if (pid < 0) 
            error("Cannot fork process")

        if (pid == 0) { // Child process
            sprintf(buffer, "Server session started. \n Use \"kill\" to exit session, \"killserver\" to kill server \n");
            write(newsockfd, buffer, strlen(buffer));

            while (1) {
                bzero(buffer, BUFFER_SIZE);
                n = read(newsockfd, buffer, BUFFER_SIZE - 1);
                if(n < 0)
                    error("ERROR reading from socket");

                printf("%d$ %s", pid, buffer); // Print message with process ID
                
                if (strcmp(buffer, "kill\n") == 0) {
                    close(newsockfd);
                    exit(0); // terminates the child process
                } else if (strcmp(buffer, "killserver\n") == 0) {
                    if (strcmp(buffer, "killserver\n") == 0) {
                    // Send signal to parent process to terminate it
                    kill(getppid(), SIGTERM);
                    exit(0);
                    }
                }
            }
            exit(0); // Exit child process
        } else { // Parent process
            close(newsockfd); // Close child socket in parent
        }
    }
    close(sockfd);
    return 0;
}
