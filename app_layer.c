#include <stdio.h>
#include "tcp_layer.h"

void *app_client (void *socket)
{
  const int buff_size = 1000;
  uint8_t buff[buff_size];

  sock_OS *sk_OS = (sock_OS *)socket;

  sock *sk = connect (get_tcp_window(sk_OS), get_tcp_mss(sk_OS), sk_OS);

  sendtcp(sk, (uint8_t*)"Hello", 6);

  size_t len = recvtcp(sk, buff, buff_size);
  printf ("%.*s.", len < 1000 ? (int)len : 1000, buff);
  return NULL;
}

void *app_server (void *socket)
{
  const int buff_size = 1000;
  uint8_t buff[buff_size];

  sock_OS *sk_OS = (sock_OS *)socket;

  sock *sk = accept (get_tcp_window(sk_OS), get_tcp_mss(sk_OS), sk_OS);

  size_t len = recvtcp(sk, buff, buff_size);
  printf ("%.*s.", len < 1000 ? (int)len : 1000, buff);

  sendtcp(sk, (uint8_t*)"Hello to you too", 8);
  return NULL;
}

