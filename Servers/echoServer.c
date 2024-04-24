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
	
	while (1){
		listen(sockfd, 5); // 5 is size of queue for handling incoming connections
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if(newsockfd < 0)
			error("ERROR on accept");

		bzero(buffer, BUFFER_SIZE);
		n = read(newsockfd, buffer, BUFFER_SIZE - 1);
		if(n < 0)
			error("ERROR reading from socket");
		
		printf("Here is the message: %s\n", buffer); 

		// Check if the received message is "kill" to terminate the server
		if (strcmp(buffer, "kill") == 0) {
			printf("Received 'kill' command. Terminating server.\n");
			break; // Exit the while loop to terminate the server
		}  
		
		n = write(newsockfd, "I got your message", 18); // 18 is length of the string literal in quotes
		if(n < 0)
			error("ERROR writing to socket");
	} 
	
	close(sockfd);
	return 0;
} 

