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

  //2.6：记录每个到达的payload长度
  int    pkt_len_q[1024];
  int    pkt_q_head;
  int    pkt_q_tail;
  int    pkt_q_count;

  //还剩多少payload未读
  size_t recv_pending_payload;
  bool   recv_reading_payload;

  //3
  size_t   rcv_unread_bytes;   
  uint16_t rcv_adv_wnd;        
  uint16_t snd_peer_wnd;       
  uint32_t snd_una;            
  uint32_t snd_nxt;            

  bool connection_established;
};

/*----------------------------------------------------------------------------*/

/* Constant definitions ------------------------------------------------------*/
#define INIT_I 0 // Initial index
#define INIT_N 0 // Initial number
/*----------------------------------------------------------------------------*/

/* Function prototypes -------------------------------------------------------*/
void ring_buff_push(ring_buff_t *ring_buff_p, uint8_t *data, size_t data_len);
void ring_buff_pop(ring_buff_t *ring_buff_p, uint8_t *buff, size_t data_len);

uint32_t rand_seq();
/*----------------------------------------------------------------------------*/

#endif
