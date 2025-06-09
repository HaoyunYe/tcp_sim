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
/* Character constant */
#define CH_NU '\0' // Null character

/* Status constants */
#define SUCCESS 0 // Function executed successfully
#define ERROR -1 // Error occurred

/*----------------------------------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/
int read_message(int sockfd, char *buffer, int buffer_size);
int send_message(int sockfd, char *message, int message_len);
int read_line(FILE *stream, char **line_p, size_t *line_len_p);
/*----------------------------------------------------------------------------*/
