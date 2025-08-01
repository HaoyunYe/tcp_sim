#define _GNU_SOURCE

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "tcp_layer.h"

#define abort(x) do {fprintf x; exit(1);} while(0)

#define MAX_MSS 8000L

typedef struct {
} trial_data_t;


typedef struct {
  sem_t *semaphore;             // global count of packets not yet scheduled
  pthread_mutex_t *packet_q_mutex;      // global mutex for unsent and inflight buffers
  uint8_t *unsent_pk_buf;
  uint8_t *unsent_pk;
  size_t window, mss;           // buffer size, maximum segment size
  sock *client, *server;
  pthread_barrier_t barrier;
} shared_os_state;

struct sock_OS {
  sem_t queue_size;     // per-socket count of packets ready for recv_ip
  trial_data_t *trial;
  sock *peer;
  uint8_t *tmp_msg;        // will be replace by a proper buffer
  uint16_t tmp_len;
  shared_os_state *state;
};

void *app_client (void *socket);
void *app_server (void *socket);

void register_sock (sock *sk, const char *type)
{
  assert (sk);
  assert (sk->os);
  assert (sk->os->state);

  bool is_client = true;

  if (!strcmp (type, "client")) {
    sk->os->state->client = sk;
  } else if (!strcmp (type, "server")) {
    is_client = false;
    sk->os->state->server = sk;
  } else
    abort ((stderr, "Invalid socket type %s\n", type));
  
  pthread_barrier_wait (&(sk->os->state->barrier));
  if (is_client)
    sk->os->peer = sk->os->state->server;
  else
    sk->os->peer = sk->os->state->client;
  sk->os->peer->os->peer = sk;
}

// parse command line
//
// launch threads
int
main (int argc, char *argv[])
{
  if (argc < 5 || strcmp (argv[1], "-w") || strcmp (argv[3], "-m")) {
    abort ((stderr, "usage: %s -w window -m MSS\n", argv[0]));
  }

  pthread_t client, server;
  int client_id, server_id;


  sem_t unscheduled_packets;
  pthread_mutex_t buffer_mutex;
  shared_os_state state = {.semaphore = &unscheduled_packets,
                           .packet_q_mutex = &buffer_mutex
                          };
  pthread_barrier_init (&(state.barrier), NULL, 2);       // sync client & server sk

  if (!sscanf(argv[2], "%ld", &(state.window)))
    abort((stderr, "Invalid window size %s\n", argv[2]));

  if (!sscanf(argv[4], "%ld", &(state.mss)))
    abort((stderr, "Invalid MSS %s\n", argv[4]));

  if (state.mss >  MAX_MSS)
    abort((stderr, "Maximum segement size %ld exceeds the limit %ld\n", state.mss, MAX_MSS));

  trial_data_t trial_data;

  sock_OS client_sk = {.queue_size = 0,
                      .state = &state,
                      //.sched_list = list_init(),
                      .trial = &trial_data,
                      .tmp_msg = NULL,
                      .peer = NULL
                     };
  sock_OS server_sk = {.queue_size = 0,
                      .state = &state,
                      //.sched_list = list_init(),
                      .trial = &trial_data,
                      .tmp_msg = NULL,
                      .peer = NULL
                     };

  client_id = pthread_create (&client, NULL, app_client, (void*)&client_sk);
  server_id = pthread_create (&server, NULL, app_server, (void*)&server_sk);

  pthread_join (client, NULL);
  pthread_join (server, NULL);

  exit(0);
}

void
send_ip (sock *sk, uint8_t *message, size_t msg_len)
{
  // Copy data to ring buffer, and schedule delivery.
  assert (msg_len <= sk->os->state->mss + sizeof(tcphdr));

  sk->os->peer->os->tmp_msg = message;
  sk->os->peer->os->tmp_len = msg_len;

  ip_arrived_interrupt(sk->os->peer);

  return;
}

// Blocking call to receive one IP packet
size_t
recv_ip(sock *sk, uint8_t *buffer, size_t buff_size)
{
  if (sk->os->tmp_msg) {
    assert (buff_size >= sk->os->tmp_len);
    memcpy (buffer, sk->os->tmp_msg, sk->os->tmp_len);
    sk->os->tmp_msg = NULL;
    return sk->os->tmp_len;
  } else
    return 0;

}

// Handle events "send_ip called" and "IP packet arrived at receiver"
void emulate_ip (trial_data_t trial_data, sem_t *waiting_pk)
{
  sem_wait(waiting_pk); // nothing happens until the first packet is sent
  
  while (1) {
    // Get packet from unscheduled buffer
  }
}

size_t get_tcp_window (sock_OS *sk)
{
  return (sk->state->window);
}

size_t get_tcp_mss (sock_OS *sk)
{
  return (sk->state->mss);
}

uint32_t rand_seq (void) {
  return rand();
}
