#include <stdlib.h>
#include <assert.h>
#include <pthread.h> 
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "my_layer.h"
#include "tcp_layer.h"


enum {
  TCP_LISTEN,
  TCP_SYN_RECEIVED,
  TCP_ESTABLISHED
};


struct sock_TCP {
  bool connected;
  int state;     // TCP_LISTEN, TCP_SYN_RECEIVED, TCP_ESTABLISHED
  size_t recv_len; // 当前缓冲区已接收字节数
  uint32_t seq_no;
  uint32_t ack_no;
  size_t window_size;
  uint8_t *recv_buffer;
  pthread_mutex_t lock;
  pthread_cond_t cond;
};

uint32_t randseq() {
  static int seeded = 0;
  if (!seeded) {
    srand(time(NULL));
    seeded = 1;
  }
  return (uint32_t)rand();
}


sock* connect(size_t window, size_t mss, sock_OS *os) {
  sock *sk = (sock *)malloc(sizeof(sock));

  assert(sk);

  sk->os = os;
  sk->tcp = (sock_TCP *)malloc(sizeof(sock_TCP));
  assert(sk->tcp);

  register_sock(sk, "client");

  // 初始化 sock_TCP
  sock_TCP *tcp = sk->tcp;
  tcp->connected = false;
  tcp->window_size = window;
  tcp->recv_buffer = (uint8_t *)malloc(window);

  //1
  memset(tcp->recv_buffer, 0, window);

  tcp->seq_no = randseq();
  pthread_mutex_init(&tcp->lock, NULL);
  pthread_cond_init(&tcp->cond, NULL);

  // 构造 SYN 报文
  my_tcphdr syn_hdr = {0};
  syn_hdr.SYN = 1;
  syn_hdr.seq_no = tcp->seq_no;
  syn_hdr.window = (uint16_t)window;

  // 发送 SYN
  send_ip(sk, (uint8_t *)&syn_hdr, sizeof(my_tcphdr));

  // 等待 SYN+ACK 到达（由 ip_arrived_interrupt 设置 connected）
  pthread_mutex_lock(&tcp->lock);
  while (!tcp->connected) {
    pthread_cond_wait(&tcp->cond, &tcp->lock);
  }
  pthread_mutex_unlock(&tcp->lock);

  return sk;
}


sock* accept(size_t window, size_t mss, sock_OS *os) {
  sock *sk = (sock *)malloc(sizeof(sock));
  assert(sk);

  sk->os = os;
  //分配内存
  sk->tcp = (sock_TCP *)malloc(sizeof(sock_TCP));
  assert(sk->tcp);

  register_sock(sk, "server");

  sock_TCP *tcp = sk->tcp;
  tcp->connected = false;
  tcp->window_size = window;
  tcp->recv_buffer = (uint8_t *)malloc(window);

  //2
  memset(tcp->recv_buffer, 0, window);
  
  pthread_mutex_init(&tcp->lock, NULL);
  pthread_cond_init(&tcp->cond, NULL);
  tcp->state = TCP_LISTEN;

  // 等待 SYN 到达
  pthread_mutex_lock(&tcp->lock);
  while (tcp->state != TCP_ESTABLISHED) {
    pthread_cond_wait(&tcp->cond, &tcp->lock);
  }
  pthread_mutex_unlock(&tcp->lock);

  return sk;
}


void ip_arrived_interrupt(sock* sk)
{
  sock_TCP *tcp = sk->tcp;

  uint8_t packet[2000]; // 缓冲区
  size_t len = recv_ip(sk, packet, sizeof(my_tcphdr));

  size_t data_len = len - sizeof(my_tcphdr); // 任何有数据的报文都拷贝 payload 到 recv_buffer
  if (len < sizeof(my_tcphdr)) return; // 异常包，忽略

  my_tcphdr *hdr = (my_tcphdr *)packet;

  pthread_mutex_lock(&tcp->lock);

  if (!tcp->connected) {
    if (hdr->SYN && !hdr->ACK && tcp->state == TCP_LISTEN) {
      // 服务端收到 SYN

      tcp->ack_no = hdr->seq_no + 1;
      tcp->seq_no = randseq();
      tcp->state = TCP_SYN_RECEIVED;

      // 构造 SYN+ACK
      my_tcphdr reply = {0};
      reply.SYN = 1;
      reply.ACK = 1;
      reply.seq_no = tcp->seq_no;
      reply.ack_no = tcp->ack_no;
      reply.window = (uint16_t)tcp->window_size;

      send_ip(sk, (uint8_t *)&reply, sizeof(my_tcphdr));

    } else if (hdr->SYN && hdr->ACK && tcp->state != TCP_ESTABLISHED) {
      // 客户端收到 SYN+ACK

      tcp->ack_no = hdr->seq_no + 1;

      // 发送 ACK
      my_tcphdr reply = {0};
      reply.ACK = 1;
      reply.seq_no = tcp->seq_no + 1;
      reply.ack_no = tcp->ack_no;
      reply.window = (uint16_t)tcp->window_size;

      send_ip(sk, (uint8_t *)&reply, sizeof(my_tcphdr));

      // 完成握手
      tcp->connected = true;

      //4
      tcp->state = TCP_ESTABLISHED;

      pthread_cond_signal(&tcp->cond);

    } else if (hdr->ACK && tcp->state == TCP_SYN_RECEIVED) {
      // 服务端收到 ACK，完成连接

      tcp->connected = true;
      tcp->state = TCP_ESTABLISHED;
      pthread_cond_signal(&tcp->cond);
    }
  }

  if (data_len > 0 && tcp->recv_len + data_len <= tcp->window_size) {
    memcpy(tcp->recv_buffer + tcp->recv_len, packet + sizeof(my_tcphdr), data_len);
    tcp->recv_len += data_len;
  }

  pthread_mutex_unlock(&tcp->lock);
}


void sendtcp(sock* sk, uint8_t *msg, size_t msg_len)
{
  sock_TCP *tcp = sk->tcp;
  pthread_mutex_lock(&tcp->lock);

  // 构造 TCP 报文头
  my_tcphdr hdr = {0};
  hdr.seq_no = tcp->seq_no;
  hdr.ack_no = tcp->ack_no;
  hdr.ACK = 1;
  hdr.window = (uint16_t)tcp->window_size;

  // 分配缓冲区放头+数据
  size_t packet_size = sizeof(my_tcphdr) + msg_len;
  uint8_t *packet = (uint8_t *)malloc(packet_size);
  memcpy(packet, &hdr, sizeof(my_tcphdr));             // header
  memcpy(packet + sizeof(my_tcphdr), msg, msg_len);    // payload

  // 发送数据
  send_ip(sk, packet, packet_size);

  // 更新序号
  tcp->seq_no += msg_len;

  free(packet);
  pthread_mutex_unlock(&tcp->lock);
}


size_t recvtcp(sock* sk, uint8_t *msg_buf, size_t msg_buf_len)
{
  sock_TCP *tcp = sk->tcp;
  pthread_mutex_lock(&tcp->lock);

  // 计算可以读取的最大字节数
  size_t to_copy = (tcp->recv_len < msg_buf_len) ? tcp->recv_len : msg_buf_len;

  memcpy(msg_buf, tcp->recv_buffer, to_copy);

  // 清空已读内容，简单起见：移动后面的数据到头部
  memmove(tcp->recv_buffer, tcp->recv_buffer + to_copy, tcp->recv_len - to_copy);
  tcp->recv_len -= to_copy;

  pthread_mutex_unlock(&tcp->lock);
  return to_copy;
}


