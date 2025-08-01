/* Inclusions ----------------------------------------------------------------*/
/* Library inclusions */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

/* File inclusions */
#include "my_tcp_layer.h"
#include "tcp_layer.h"

/*----------------------------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
bool clients_turn=true;
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
/*----------------------------------------------------------------------------*/

sock *
connect(size_t window, size_t mss, sock_OS *os) {
  uint8_t temp_buff[window];
  bool syn_ack_packet_received=false;
  sock *sk=NULL;
  tcphdr *syn_packet_p=NULL, *syn_ack_packet_p=NULL, *ack_packet_p=NULL;
  
  /* Register socket */
  sk = (sock *)malloc(sizeof(sock));
  assert(sk);
  sk->os = os;
  register_sock(sk, "client");
  // fprintf(stderr, "DEBUG: 'client' socket registered\n");

  sleep(1); // Provide time for 'server' to enter waiting mode

  pthread_mutex_lock(&lock);

  fprintf(stderr, "DEBUG: 'client's turn first!\n");
  sk->tcp = (sock_TCP *)malloc(sizeof(sock_TCP));
  assert(sk->tcp);
  sk->tcp->window = window;
  sk->tcp->mss = mss;
  
  sk->tcp->ring_buff.buff = (uint8_t *)malloc(sizeof(uint8_t)*window);
  assert(sk->tcp->ring_buff.buff);
  sk->tcp->ring_buff.maxlen = window;
  sk->tcp->ring_buff.head = INIT_I;
  sk->tcp->ring_buff.tail = INIT_I;
  sk->tcp->ring_buff.curr_len = INIT_N;
  fprintf(stderr, "DEBUG: 'client' socket created\n");
  clients_turn = false;
  pthread_cond_signal(&cond);
  fprintf(stderr, "DEBUG: 'client' set up, turn finished!\n");
  
  while (!clients_turn) {
    fprintf(stderr, "DEBUG: 'client' waiting for 'server' to finish its turn\n")
    ;
    pthread_cond_wait(&cond, &lock);
  }

  fprintf(stderr, "DEBUG: 'client's turn next!\n");
  syn_packet_p = (tcphdr *)malloc(sizeof(tcphdr));
  assert(syn_packet_p);
  syn_packet_p->SYN = true;
  syn_packet_p->ACK = false;
  syn_packet_p->seq_no = rand_seq();
  fprintf(stderr, "DEBUG: 'client' sent SYN packet with seq no %u\n", 
    syn_packet_p->seq_no);
  send_ip(sk, (uint8_t *)syn_packet_p, sizeof(*syn_packet_p));
  free(syn_packet_p);
  clients_turn = false;
  pthread_cond_signal(&cond);
   fprintf(stderr, "DEBUG: 'client's turn finished!\n");

  if (!clients_turn) {
    fprintf(stderr, "DEBUG: 'client' waiting for 'server' to send SYN+ACK "
      "packet\n");
    pthread_cond_wait(&cond, &lock);
  }

  fprintf(stderr, "DEBUG: 'client's turn next!\n");
  while (!syn_ack_packet_received) {
    if (sk->tcp->ring_buff.curr_len>INIT_N) {
      ring_buff_pop(&(sk->tcp->ring_buff), temp_buff, sizeof(tcphdr));
        syn_ack_packet_p = (tcphdr *)temp_buff;
        if (syn_ack_packet_p->SYN && syn_ack_packet_p->ACK) {
          syn_ack_packet_received = true;
          fprintf(stderr, "DEBUG: 'client' received SYN+ACK packet with seq no "
            "%u and ack no %u\n", syn_ack_packet_p->seq_no, 
            syn_ack_packet_p->ack_no);
        }
    }
  }

  ack_packet_p = (tcphdr *)malloc(sizeof(tcphdr));
  assert(ack_packet_p);
  ack_packet_p->ACK = true;
  ack_packet_p->seq_no = syn_ack_packet_p->ack_no;
  ack_packet_p->ack_no = syn_ack_packet_p->seq_no+1;
  fprintf(stderr, "DEBUG: 'client' sent ACK packet with seq no %u and ack no %u"
    "\n", ack_packet_p->seq_no, ack_packet_p->ack_no);
  send_ip(sk, (uint8_t *)ack_packet_p, sizeof(*ack_packet_p));
  free(ack_packet_p);

  sk->tcp->connection_established = true;
  fprintf(stderr, "DEBUG: 'client' connection established\n");
  clients_turn = false;
  pthread_cond_signal(&cond);
  fprintf(stderr, "DEBUG: 'client's turn finished!\n");

  pthread_mutex_unlock(&lock);

  return sk;
}

sock *
accept(size_t window, size_t mss, sock_OS *os) {
  uint8_t temp_buff[window];
  bool syn_packet_received=false, ack_packet_received=false;
  tcphdr *syn_packet_p=NULL, *syn_ack_packet_p=NULL, *ack_packet_p=NULL;
  
  sock *sk = (sock *)malloc(sizeof(sock));
  assert(sk);

  sk->os = os;
  register_sock(sk, "server");

  pthread_mutex_lock(&lock);

  while (clients_turn) {
    fprintf(stderr, "DEBUG: 'server' waiting for 'client' to finish its turn\n")
    ;
    pthread_cond_wait(&cond, &lock);
  }

  fprintf(stderr, "DEBUG: 'server's turn next!\n");
  sk->tcp = (sock_TCP *)malloc(sizeof(sock_TCP));
  assert(sk->tcp);
  sk->tcp->window = window;
  sk->tcp->mss = mss;
  
  sk->tcp->ring_buff.buff = (uint8_t *)malloc(sizeof(uint8_t)*window);
  assert(sk->tcp->ring_buff.buff);
  sk->tcp->ring_buff.maxlen = window;
  sk->tcp->ring_buff.head = INIT_I;
  sk->tcp->ring_buff.tail = INIT_I;
  sk->tcp->ring_buff.curr_len = INIT_N;
  fprintf(stderr, "DEBUG: 'server' socket created\n");
  clients_turn = true;
  pthread_cond_signal(&cond);
  fprintf(stderr, "DEBUG: 'server' set up, turn finished!\n");

  while (clients_turn) {
    fprintf(stderr, "DEBUG: 'server' waiting for 'client' to send SYN packet\n")
    ;
    pthread_cond_wait(&cond, &lock);
  }

  fprintf(stderr, "DEBUG: 'server's turn next!\n");
  while (!syn_packet_received) {
    if (sk->tcp->ring_buff.curr_len>INIT_N) {
      ring_buff_pop(&(sk->tcp->ring_buff), temp_buff, sizeof(tcphdr));
      syn_packet_p = (tcphdr *)temp_buff;
      if (syn_packet_p->SYN && !(syn_packet_p->ACK)) {
        syn_packet_received = true;
        fprintf(stderr, "DEBUG: 'server' received SYN packet with seq no %u\n",
          syn_packet_p->seq_no);
      }
    }
  }

  syn_ack_packet_p = (tcphdr *)malloc(sizeof(tcphdr));
  assert(syn_ack_packet_p);
  syn_ack_packet_p->SYN = true;
  syn_ack_packet_p->ACK = true;
  syn_ack_packet_p->seq_no = rand_seq();
  syn_ack_packet_p->ack_no = syn_packet_p->seq_no+1;
  fprintf(stderr, "DEBUG: 'server' sent SYN+ACK packet with seq no %u and ack "
    "no %u\n", syn_ack_packet_p->seq_no, syn_ack_packet_p->ack_no);
  send_ip(sk, (uint8_t *)syn_ack_packet_p, sizeof(*syn_ack_packet_p));
  free(syn_ack_packet_p);
  clients_turn = true;
  pthread_cond_signal(&cond);
  fprintf(stderr, "DEBUG: 'server's turn finished!\n");

  while (clients_turn) {
    fprintf(stderr, "DEBUG: 'server' waiting for 'client' to send ACK packet\n")
    ;
    pthread_cond_wait(&cond, &lock);
  }
  
  fprintf(stderr, "DEBUG: 'server's turn next!\n");
  while (!ack_packet_received) {
    if (sk->tcp->ring_buff.curr_len>INIT_N) {
      ring_buff_pop(&(sk->tcp->ring_buff), temp_buff, sizeof(tcphdr));
      ack_packet_p = (tcphdr *)temp_buff;
      if (ack_packet_p->ACK) {
        ack_packet_received = true;
        fprintf(stderr, "DEBUG: 'server' received ACK packet with seq no %u and"
          " ack no %u\n", ack_packet_p->seq_no, ack_packet_p->ack_no);
      }
    }
  }

  sk->tcp->connection_established = true;
  fprintf(stderr, "DEBUG: 'server' connection established\n");
  clients_turn = true;
  pthread_cond_signal(&cond);
  fprintf(stderr, "DEBUG: 'server's turn finished!\n");

  pthread_mutex_unlock(&lock);

  return sk;
}

void
ip_arrived_interrupt(sock *sk) {
  size_t recv_len;
  uint8_t *temp_buff=NULL;
  
  temp_buff = (uint8_t *)malloc(sizeof(uint8_t)*sk->tcp->window);
  assert(temp_buff);

  fprintf(stderr, "DEBUG: IP packet arrived\n");
  recv_len = recv_ip(sk, temp_buff, sk->tcp->window);
  fprintf(stderr, "DEBUG: IP packet received\n");
  ring_buff_push(&(sk->tcp->ring_buff), temp_buff, recv_len);
  fprintf(stderr, "DEBUG: IP packet saved to ring buffer\n");
  
  free(temp_buff);
}

void sendtcp(sock *sk, uint8_t *msg, size_t msg_len) {
  // Your code here
}

size_t recvtcp(sock *sk, uint8_t *msg_buf, size_t msg_buf_len) {
  // Your code here
  return 0;
}

/* Adapted from https://embedjournal.com/implementing-circular-buffer-embedded-
c/ ---------------------------------------------------------------------------*/

void
ring_buff_push(ring_buff_t *ring_buff_p, uint8_t *data, size_t data_len) {
    int i;

    if (data_len>ring_buff_p->maxlen-ring_buff_p->curr_len) {
        fprintf(stderr, "Ring buffer full\n");
        exit(EXIT_FAILURE);
    }

    for (i=INIT_I; i<data_len; i++) {
        ring_buff_p->buff[ring_buff_p->head] = data[i];
        ring_buff_p->head++;
        if (ring_buff_p->head>=ring_buff_p->maxlen) {
          ring_buff_p->head = INIT_I;
        }
        ring_buff_p->curr_len++;
    }
}

void
ring_buff_pop(ring_buff_t *ring_buff_p, uint8_t *buff, size_t data_len) {
  int i;

  if (data_len>ring_buff_p->curr_len) {
    fprintf(stderr, "Not enough data in ring buffer\n");
    exit(EXIT_FAILURE);
  }

  for (i=INIT_I; i<data_len; i++) {
    buff[i] = ring_buff_p->buff[ring_buff_p->tail];
    ring_buff_p->tail++;
    if (ring_buff_p->tail>=ring_buff_p->maxlen) {
      ring_buff_p->tail = INIT_I;
    }
    ring_buff_p->curr_len--;
  }
}

/* End reference -------------------------------------------------------------*/
