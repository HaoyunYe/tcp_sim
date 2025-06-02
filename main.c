/* Caching web proxy program for HTTP/1.1 */

#define _POSIX_C_SOURCE 200112L // Define POSIX version

/* Inclusions ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
/*----------------------------------------------------------------------------*/

/* Constant definitions ------------------------------------------------------*/
/* Numerical constants */
#define MIN_ARGS 3 // Minimum number of arguments
#define MAX_ARGS 4 // Maximum number of arguments
#define MAX_QUEUE 5 // Maximum number of connection requests in queue
#define BUFFER_SIZE 255 // Buffer size
#define INIT_I 0 // Initial index
#define INIT_N 0 // Initial number

/* Character constant */
#define CH_NU '\0' // Null character

/* Command line argument flag constant */
#define CACHING_FLAG "-c" // Caching flag

/* Status constants */
#define EQUAL 0 // Compared strings are equal
#define SUCCESS 0 // Function executed successfully
#define ENABLED 1 // Flag enabled

/*----------------------------------------------------------------------------*/

/* Function prototype --------------------------------------------------------*/
int create_listening_socket(char *service);
/*----------------------------------------------------------------------------*/

int
main(int argc, char **argv) {
	int sockfd /* Listening socket */, newsockfd /* Connection socket */, port
	/* Client port */, n /* Number of characters read or written */;
	char *service, ip[INET_ADDRSTRLEN] /* IP address of client */,
        buffer[BUFFER_SIZE+1];
	bool perform_caching=false;
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;

	/* Check number of arguments and parse */
	if (argc<MIN_ARGS||argc>MAX_ARGS) {
		fprintf(stderr, "Invalid number of arguments\n");
		exit(EXIT_FAILURE);
	}
    service = argv[INIT_I+2];
    if (argc>MIN_ARGS&&strcmp(argv[INIT_I+3], CACHING_FLAG)==EQUAL) {
        perform_caching = true;
    }

	sockfd = create_listening_socket(service);

	/* Listen on socket and queue up to 5 connection requests */
	if (listen(sockfd, MAX_QUEUE)<SUCCESS) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Wait for and accept a connection */
	client_addr_size = sizeof(client_addr);
	newsockfd = accept(sockfd, (struct sockaddr *)&client_addr,
        &client_addr_size); // Create connection socket
	if (newsockfd<SUCCESS) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* Print IPv4 peer information */
	getpeername(newsockfd, (struct sockaddr *)&client_addr, &client_addr_size);
	inet_ntop(client_addr.sin_family, &client_addr.sin_addr, ip,
     INET_ADDRSTRLEN); // Convert IP address to string
	port = ntohs(client_addr.sin_port);
	printf("New connection from %s:%d on socket %d\n", ip, port, newsockfd);

	/* Read characters from connection into 'buffer' and print */
	n = read(newsockfd, buffer, BUFFER_SIZE);
	if (n<SUCCESS) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	buffer[n] = CH_NU;
	printf("Received message in 'buffer': %s\n", buffer);

	/* Write message to client */
	n = write(newsockfd, "Message received\n", strlen("Message received\n"));
	if (n<SUCCESS) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	/* Close sockets */
	close(sockfd);
	close(newsockfd);

	return 0;
}

/* Create listening socket at port 'service' */
int
create_listening_socket(char *service) {
	int sockfd /* Socket file descriptor */, s, re /* Port reuse flag */;
	struct addrinfo hints /* Address information hints */, *res /* Pointer to
	address information */;

	/* Create IPv4 address to listen to (with given port number) */
	memset(&hints, INIT_N, sizeof(hints));
	hints.ai_family = AF_INET; // IPv4 address
	hints.ai_socktype = SOCK_STREAM; // Socket type is byte streams
	hints.ai_flags = AI_PASSIVE; // as listening socket
	s = getaddrinfo(NULL, service, &hints, &res);
	// name = 'NULL' means any interface
	if (s!=SUCCESS) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* Create socket */
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd<SUCCESS) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Reuse port if possible */
	re = ENABLED;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int))<SUCCESS)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	/* Bind address to socket */
	if (bind(sockfd, res->ai_addr, res->ai_addrlen)<SUCCESS) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	/* Free memory */
	freeaddrinfo(res);

	return sockfd;
}
