#include <stdlib.h> // for exit
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h> // for bzero
#include <unistd.h> // for read and write
#include <string.h> 
#include <signal.h> 
#include <sys/unistd.h> // Imports fork and related functions
#include <limits.h> 
#include <stdbool.h>

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
	
	char killMessage[] = "kill\n";
    char killserver[] = "killserver\n";
	listen(sockfd, 5); // 5 is size of queue for handling incoming connections
	clilen = sizeof(cli_addr);
	bool killed = false;
    // client connection loop
	while (1){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if(newsockfd < 0)
			error("ERROR on accept"); 
        
        int pid = -1;
        if(killed == false){
            // Fork child process
            pid = fork();
        }

        // Child process was forked successfully
        if(pid == 0){ 
            sprintf(buffer, "Server session started.\nUse \"kill\" to exit session, \"killserver\" to kill server \n");
            n = write(newsockfd, buffer, strlen(buffer)); 
            if(n < 0)
			    error("ERROR writing to socket");
            
            // new client was created and will run until it is killed
            while(1) {
                // gets the current child process id
                int id = getpid();
                int num = getNumChar(id);
                char charID[num]; 
                bzero(buffer, BUFFER_SIZE);
                sprintf(buffer, "%d$", id);
                
            
                // write the child process id each time
                n = write(newsockfd, buffer, num+1); 
                if(n < 0)
			        error("ERROR writing to socket");

                bzero(buffer, BUFFER_SIZE);
                n = read(newsockfd, buffer, BUFFER_SIZE - 1);
                if(n < 0)
                    error("ERROR reading from socket");
                
                printf("Here is the message: %s\n", buffer); 

                // Check if the received message is "kill" to terminate the server
                if (strcmp(buffer, killMessage) == 0) {
                    // kill the child process
                    printf("Received 'kill' command. Terminating client connection.\n");  

                    sprintf(buffer, "kill\n");
                    n = write(newsockfd, buffer, strlen(buffer)); 
                    if(n < 0)
			            error("ERROR writing to socket");
                    break;
                }  

                // Check if the received message is "kill" to terminate the server
                if (strcmp(buffer, killserver) == 0) {
                    printf("Received 'kill' command. Terminating server.\n");
                    // kill the parent but leave the child process running until the child recieves the kill command
                    // Send signal to parent process to terminate it
                    kill(getppid(), SIGTERM);
                    killed = true;
                }   

                n = write(newsockfd, buffer, BUFFER_SIZE-1); 
                if(n < 0)
                    error("ERROR writing to socket");

    
        }
	} 
    }
	
	close(sockfd);
	return 0;
} 