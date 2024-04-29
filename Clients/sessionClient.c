#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h> // for exit
#include <strings.h> // for bzero
#include <string.h> // for strlen
#include <unistd.h> // for read and write

#define BUFFER_SIZE 256

void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if(server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }   

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    // Receive server session started message
    n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(n < 0) 
        error("ERROR reading from socket"); 
    printf("%s", buffer); 

	// recieve the message detailing how to use kill and killserver
    n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if(n < 0) 
        error("ERROR reading from socket"); 
    printf("%s", buffer); // Display startup message 2

    // Enter loop for communication
    while (1) {
        // Display prompt with unique ID number
        // Implement code to display prompt with received ID number
        printf("%s$", buffer); // Display prompt

        // Take user input
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, sizeof(buffer), stdin);

        // Send user input to server
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) {
            perror("ERROR writing to socket");
            exit(1);
        }

        bzero(buffer, BUFFER_SIZE);
		fgets(buffer, BUFFER_SIZE - 1, stdin); // Read input to buffer
		n = write(sockfd, buffer, strlen(buffer));
		if(n < 0) 
			error("ERROR writing to socket");

		bzero(buffer, BUFFER_SIZE);
		n = read(sockfd, buffer, BUFFER_SIZE - 1);
		if(n < 0) 
			error("ERROR reading from socket");
		
		printf("%s\n",buffer);
		return 0;
    	}

    close(sockfd);
    return 0;
}
