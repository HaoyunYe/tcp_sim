/* Read and send messages to and from client and server */

#include "message.h"

/* Read message from client or server to 'buffer' and returns number of 'char's
read */
int
read_message(int sockfd, char *buffer, int buffer_size) {
    int n; // Number of 'char's read

    n = read(sockfd, buffer, buffer_size);
	if (n<SUCCESS) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	buffer[n] = CH_NU;

    return n;
}

/* Send message to client or server and returns number of 'char's sent */
int
send_message(int sockfd, char *message, int message_len) {
    int n; // Number of 'char's sent

    n = write(sockfd, message, message_len);
	if (n<SUCCESS) {
		perror("write");
		exit(EXIT_FAILURE);
	}

    return n;
}
