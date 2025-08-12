#include <stdio.h>
#include <unistd.h>
#include "tcp_layer.h"

void *app_client (void *socket)
{
  const int buff_size = 1000;
  uint8_t buff[buff_size];
  uint8_t buffer[3];  // 小缓冲区
  size_t len;

  sock_OS *sk_OS = (sock_OS *)socket;

  sock *sk = connect (get_tcp_window(sk_OS), get_tcp_mss(sk_OS), sk_OS);

  sendtcp(sk, (uint8_t*)"Hellooooooooooooooooooooooooooooooooooooooooooooooooooooooooo", 62); //长消息测试

  while ((len = recvtcp(sk, buffer, sizeof(buffer))) == 0) {
    usleep(100);
  }

  /* 关键：把第一次读到的这块先打印出来，别丢 */
  printf("%.*s", (int)len, (char*)buffer);

  /* 再继续把后续块都读完、打印 */
  while ((len = recvtcp(sk, buffer, sizeof(buffer))) > 0) {
    printf("%.*s", (int)len, (char*)buffer);
  }
  printf("\n");

  return NULL;
}

void *app_server (void *socket)
{
  const int buff_size = 1000;
  uint8_t buff[buff_size];

  sock_OS *sk_OS = (sock_OS *)socket;

  sock *sk = accept (get_tcp_window(sk_OS), get_tcp_mss(sk_OS), sk_OS);

  size_t len;
  // 连续读取直到本次消息读完
  while ((len = recvtcp(sk, buff, buff_size)) > 0) {
    printf("%.*s", (int)len, (char*)buff);
  }
  printf("\n");

  sendtcp(sk, (uint8_t*)"Hello to you too", 17);
  return NULL;
}

