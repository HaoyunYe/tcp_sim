/* Inclusions ----------------------------------------------------------------*/
/* Library inclusions */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

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

  
  //2.6
  sk->tcp->pkt_q_head = 0;
  sk->tcp->pkt_q_tail = 0;
  sk->tcp->pkt_q_count = 0;
  sk->tcp->recv_pending_payload = 0;
  sk->tcp->recv_reading_payload = false;

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

  
  //2.6
  sk->tcp->pkt_q_head = 0;
  sk->tcp->pkt_q_tail = 0;
  sk->tcp->pkt_q_count = 0;
  sk->tcp->recv_pending_payload = 0;
  sk->tcp->recv_reading_payload = false;

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

void ip_arrived_interrupt(sock *sk) {
  size_t recv_len;
  uint8_t *temp_buff = malloc(sk->tcp->window);
  assert(temp_buff);

  fprintf(stderr, "DEBUG: IP packet arrived\n");
  recv_len = recv_ip(sk, temp_buff, sk->tcp->window);
  fprintf(stderr, "DEBUG: IP packet received\n");

  // 将接收到的数据存入 ring buffer
  ring_buff_push(&(sk->tcp->ring_buff), temp_buff, recv_len);
  fprintf(stderr, "DEBUG: IP packet saved to ring buffer\n");

  if (recv_len >= sizeof(tcphdr)) {
    size_t payload = recv_len - sizeof(tcphdr);

    if (payload > 0) {
      // payload 长度入队
      if (sk->tcp->pkt_q_count < 1024) {
        sk->tcp->pkt_len_q[sk->tcp->pkt_q_tail] = (int)payload;
        sk->tcp->pkt_q_tail = (sk->tcp->pkt_q_tail + 1) % 1024;
        sk->tcp->pkt_q_count++;
      } else {
        fprintf(stderr, "Packet length queue overflow\n");
        exit(EXIT_FAILURE);
      }

      // 如果连接已建立，回 ACK
      if (sk->tcp->connection_established) {
        tcphdr *rx = (tcphdr*)temp_buff;
        tcphdr ack = {0};
        ack.ACK = true;
        ack.ack_no = rx->seq_no + (uint32_t)payload;
        ack.seq_no = rand_seq();

        send_ip(sk, (uint8_t*)&ack, sizeof(ack));
        fprintf(stderr,
          "DEBUG: sent DATA-ACK with ack_no=%u (rx seq=%u, payload=%zu)\n",
          ack.ack_no, rx->seq_no, payload);
      }
    }
  }

  free(temp_buff);
}

//stage2

void sendtcp(sock *sk, uint8_t *msg, size_t msg_len) {
  assert(sk && sk->tcp);
  assert(sk->tcp->connection_established);

  size_t mss = sk->tcp->mss;
  size_t sent = 0;
  uint32_t seq_no = rand_seq(); // 初始序列号，可后续存储状态

  while (sent < msg_len) {
    // 本段数据大小
    size_t seg_len = (msg_len - sent > mss) ? mss : (msg_len - sent);

    // 分配内存存放 header + payload
    size_t total_len = sizeof(tcphdr) + seg_len;
    uint8_t *packet = malloc(total_len);
    if (!packet) {
        perror("sendtcp: malloc failed");
        exit(EXIT_FAILURE);
    }

    // 构造 header
    tcphdr *header = (tcphdr *)packet;
    memset(header, 0, sizeof(tcphdr));
    header->seq_no = seq_no;
    header->ACK = true;

    // 拷贝 payload 到 packet
    memcpy(packet + sizeof(tcphdr), msg + sent, seg_len);

    // 发送
    send_ip(sk, packet, total_len);
    fprintf(stderr, "DEBUG: sendtcp segment seq_no=%u, len=%zu\n", seq_no, seg_len);

    // 更新状态
    seq_no += seg_len;
    sent += seg_len;
    free(packet);
  }
}

size_t recvtcp(sock *sk, uint8_t *msg_buf, size_t msg_buf_len) {
  assert(sk && sk->tcp);
  sock_TCP *ts = sk->tcp;

  if (msg_buf_len == 0) return 0;

  size_t copied = 0;

  while (copied < msg_buf_len) {
    // 若当前不在读 payload，则尝试开始读取一个新段
    if (!ts->recv_reading_payload) {
      if (ts->ring_buff.curr_len < sizeof(tcphdr) || ts->pkt_q_count == 0) {
        break; // 暂时读不到更多，先返回已拷贝部分（可能是0）
      }

      // 丢掉头
      uint8_t dummy_hdr[sizeof(tcphdr)];
      ring_buff_pop(&ts->ring_buff, dummy_hdr, sizeof(tcphdr));

      // 取该段payload长度
      ts->recv_pending_payload = (size_t)ts->pkt_len_q[ts->pkt_q_head];
      ts->pkt_q_head = (ts->pkt_q_head + 1) % 1024;
      ts->pkt_q_count--;

      ts->recv_reading_payload = true;

      fprintf(stderr, "DEBUG: recvtcp start packet, payload=%zu\n",
              ts->recv_pending_payload);
    }

    // 如果该段已读完，准备切到下一个段
    if (ts->recv_pending_payload == 0) {
      ts->recv_reading_payload = false;
      continue;
    }

    // 本次最多可读
    size_t want = msg_buf_len - copied;
    size_t can  = ts->recv_pending_payload;
    size_t to_read = (want < can) ? want : can;

    // 保险：环形缓冲中必须至少有 to_read 字节
    if (ts->ring_buff.curr_len < to_read) {
      // 正常不该发生；若发生，先返回已读部分
      break;
    }

    ring_buff_pop(&ts->ring_buff, msg_buf + copied, to_read);
    ts->recv_pending_payload -= to_read;
    copied += to_read;

    // 如果本段读完，标记为可切换到下一段
    if (ts->recv_pending_payload == 0) {
      ts->recv_reading_payload = false;
    }
  }

  return copied;
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
