#include <stdio.h>
#include <stdint.h>
#include "my_layer.h"
#include "tcp_layer.h"

void *app_client (void *socket)
{
  const int buff_size = 1000;
  uint8_t buff[buff_size];

  sock_OS *sk_OS = (sock_OS *)socket;

  sock *sk = connect (get_tcp_window(sk_OS), get_tcp_mss(sk_OS), sk_OS);

  sendtcp(sk, (uint8_t*)"Hello", 6);

  int len = recvtcp(sk, buff, buff_size);
  printf("[Client] Received %d bytes: %.*s\n", len, len, buff);

  return NULL;
}

void *app_server (void *socket)
{
  const int buff_size = 1000;
  uint8_t buff[buff_size];

  sock_OS *sk_OS = (sock_OS *)socket;

  sock *sk = accept (get_tcp_window(sk_OS), get_tcp_mss(sk_OS), sk_OS);

  int len = recvtcp(sk, buff, buff_size);
  printf("[Server] Received %d bytes: %.*s\n", len, len, buff);

  sendtcp(sk, (uint8_t*)"Hello to you too", 17);

  return NULL;
}

__attribute__((used)) void* (*__keep_app_client)(void *) = app_client;
__attribute__((used)) void* (*__keep_app_server)(void *) = app_server;