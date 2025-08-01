/* Do not change this file.  It defines the testing framework API. */
/* If you want to add extra functions etc., create other header files. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct sock_OS sock_OS;   // We  define this
typedef struct sock_TCP sock_TCP; // You define this

typedef struct {
  sock_OS *os;   // State we allocate, used by the framework
  sock_TCP *tcp; // State you add for your functions
} sock;

typedef struct {
  uint16_t src_port : 16; // ignore this
  uint16_t dst_port : 16; // ignore this
  uint32_t seq_no : 32;
  uint32_t ack_no : 32;
  unsigned int data_offset : 4; // ignore this
  unsigned int reserved : 4;    // ignore this
  bool CWC : 1, ECE : 1, URG : 1, ACK : 1, PSH : 1, RST : 1, SYN : 1, FIN : 1;
  uint16_t window : 16;
  uint16_t checksum : 16;       // ignore this
  uint16_t urgent_pointer : 16; // ignore this
} tcphdr;

// We implement these
// size_t time (void);
void send_ip(sock *sk, uint8_t *msg, size_t msg_len);
size_t recv_ip(sock *sk, uint8_t *buff, size_t buff_len);
void register_sock(sock *sk, const char *type);
uint32_t randseq();

size_t get_tcp_window(sock_OS *sk); // ignore this
size_t get_tcp_mss(sock_OS *sk);    // ignore this

// You implement these
sock *connect(size_t window, size_t mss, sock_OS *os);
sock *accept(size_t window, size_t mss, sock_OS *os);
void ip_arrived_interrupt(sock *sk);
void sendtcp(sock *sk, uint8_t *msg, size_t msg_len);
size_t recvtcp(sock *sk, uint8_t *msg_buf, size_t msg_buf_len);
