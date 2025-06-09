/* Caching web proxy program for HTTP/1.1 */

/* Source definitions --------------------------------------------------------*/
#define _POSIX_C_SOURCE 200112L // Define POSIX version
#define _GNU_SOURCE // Define GNU source
/*----------------------------------------------------------------------------*/

/* Inclusions ----------------------------------------------------------------*/
/* Library inclusions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

/* File inclusion */
#include "message.h"

/*----------------------------------------------------------------------------*/

/* Constant definitions ------------------------------------------------------*/
/* Numerical constants */
#define MIN_ARGS 3 // Minimum number of arguments
#define MAX_ARGS 4 // Maximum number of arguments
#define MAX_QUEUE 10 // Maximum number of connection requests in queue
#define BUFFER_SIZE 8192 // Buffer size
#define INIT_I 0 // Initial index
#define INIT_N 0 // Initial number

/* Character constant */
#define CH_NU '\0' // Null character

/* String constants */
#define CACHING_FLAG "-c" // Caching flag
#define PORT_80 "80" // Port 80

/* Status constants */
#define EQUAL 0 // Compared strings are equal
#define SUCCESS 0 // Function executed successfully
#define ENABLED 1 // Flag enabled
#define ERROR -1 // Error occurred

/*----------------------------------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/
int create_listening_socket(char *service);
int create_connection_socket(char *host);
/*----------------------------------------------------------------------------*/

int
main(int argc, char **argv) {
	int sockfd /* Listening socket */, newsockfd /* Connection socket */,
    server_sockfd, port /* Client port */, n=INIT_N /* Number of 'char's read or
    written */, i;
	char *service=NULL, buffer[BUFFER_SIZE+1], *request=NULL /* Client request
	*/, *header=NULL /* Request header */, *host=NULL /* Requested host */,
        *line=NULL /* Message line */;
	bool perform_caching=false;
	size_t line_size=INIT_N; // Size of buffer 'line'
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
    FILE *server_stream=NULL;

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

	/* Listen on socket and queue up to 10 connection requests */
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

	/* Print peer information to 'stderr' */
	getpeername(newsockfd, (struct sockaddr *)&client_addr, &client_addr_size);
    if (client_addr.sin_family==AF_INET) { // IPv4 address
        char ip[INET_ADDRSTRLEN] /* IPv4 address of client */;

        inet_ntop(client_addr.sin_family, &client_addr.sin_addr, ip,
                  INET_ADDRSTRLEN); // Convert IPv4 address to string
        port = ntohs(client_addr.sin_port);
        fprintf(stderr, "New IPv4 connection from %s:%d on socket %d\n", ip,
                port, newsockfd);
    } else { // IPv6 address
        char ip[INET6_ADDRSTRLEN] /* IPv6 address of client */;

        inet_ntop(client_addr.sin_family, &client_addr.sin_addr, ip,
            INET6_ADDRSTRLEN); // Convert IPv6 address to string
        port = ntohs(client_addr.sin_port);
        fprintf(stderr, "New IPv6 connection from %s:%d on socket %d\n", ip,
                port, newsockfd);
    }

	/* Read message from client into 'buffer' and print to 'stderr' */
	n = read_message(newsockfd, buffer, BUFFER_SIZE);
	fprintf(stderr, "Client request:\n%s\n", buffer);
//	fprintf(stderr, "TEMP DEBUG: n = %d, strlen(buffer) = %ld\n", n,
//        strlen(buffer));

    /* Identify 'host' */
    request = strdup(buffer);
    header = strtok(buffer, "\r\n");
    while (header!=NULL) {
        for (i=INIT_I; i<strlen(header)&&header[i]!=':'; i++) {
            header[i] = tolower(header[i]);
        }
//        fprintf(stderr, "TEMP DEBUG: header: %s\n", header);
        if (strstr(header, "host: ")) {
            host = &header[INIT_I+6];
//            fprintf(stderr, "TEMP DEBUG: host: %s\n", host);
            break;
        }
        header = strtok(NULL, "\r\n");
    }

    server_sockfd = create_connection_socket(host);
 	
 	/* Send 'request' to 'host' */
 	n = send_message(server_sockfd, request, strlen(request));
//    fprintf(stderr, "TEMP DEBUG: n = %d, strlen(request) = %ld\n", n,
//            strlen(request));
    free(request);
    request = NULL;

    /* Create 'server_stream' with 'server_sockfd' */
    server_stream = fdopen(server_sockfd, "r");
    if (server_stream==NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    n = read_line(server_stream, &line, &line_size);
    fprintf(stderr, "Server response line: %s\n", line);
//    fprintf(stderr, "TEMP DEBUG: n = %d, line_size = %ld\n", n, line_size);

    free(line);
    line = NULL;

	/* Close stream and sockets */
    fclose(server_stream);
    close(server_sockfd);
	close(newsockfd);
    close(sockfd);

	return 0;
}

/* Create listening socket at port 'service' */
int
create_listening_socket(char *service) {
	int sockfd /* Socket file descriptor */, s, re /* Port reuse flag */;
	struct addrinfo hints /* Address information hints */, *res=NULL /* Pointer
	to address information */;

	/* Create IP address to listen to (with given port number) */
	memset(&hints, INIT_N, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Unspecified IP address type
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

/* Create connection socket to 'host' port 80 */
int
create_connection_socket(char *host) {
    int sockfd, s;
    struct addrinfo hints /* Server address information hints */, *servinfo=NULL
	/* Server address information */, *rp=NULL /* Result (address) pointer */;

    /* Get server address */
	memset(&hints, INIT_N, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Unspecified IP address type
	hints.ai_socktype = SOCK_STREAM; // Socket type is byte streams
	s = getaddrinfo(host, PORT_80, &hints, &servinfo);
	if (s!=SUCCESS) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* Connect to first valid address */
	for (rp=servinfo; rp!=NULL; rp=rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd==ERROR) {
			continue;
		} else if (connect(sockfd, rp->ai_addr, rp->ai_addrlen)==SUCCESS) {
    		// Connection successful
			break;
		}
		close(sockfd);
	}
	if (rp==NULL) {
		fprintf(stderr, "Connection to server failed\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(servinfo);

    return sockfd;
}
