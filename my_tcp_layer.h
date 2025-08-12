#ifndef MY_TCP_LAYER_H
#define MY_TCP_LAYER_H

/* Inclusion -----------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/

/* Struct definitions --------------------------------------------------------*/

/* Adapted from https://embedjournal.com/implementing-circular-buffer-embedded-
c/ ---------------------------------------------------------------------------*/
typedef struct {
    uint8_t *buff;
    int maxlen;
    int head;
    int tail;
    int curr_len;
} ring_buff_t;
/* End reference -------------------------------------------------------------*/

struct sock_TCP {
  size_t window;
  size_t mss;
  ring_buff_t ring_buff;
  bool connection_established;
  uint32_t next_seq_no; // Next sent packet's sequence number
  uint32_t next_ack_no; // Next received packet's expected sequence number
};

/*----------------------------------------------------------------------------*/

/* Constant definitions ------------------------------------------------------*/
#define INIT_I 0 // Initial index
#define INIT_N 0 // Initial number

#define CH_NU '\0' // Null character
/*----------------------------------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/
void ring_buff_push(ring_buff_t *ring_buff_p, uint8_t *data, size_t data_len);
void ring_buff_pop(ring_buff_t *ring_buff_p, uint8_t *buff, size_t data_len);

uint32_t rand_seq();
/*----------------------------------------------------------------------------*/

#endif
