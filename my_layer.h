#ifndef MY_LAYER_H
#define MY_LAYER_H

#include <stdint.h>

// 自定义 TCP 报文头结构
typedef struct {
    uint8_t SYN;
    uint8_t ACK;
    uint8_t FIN;
    uint32_t seq_no;
    uint32_t ack_no;
    uint16_t window;
    // payload 后续附带
} my_tcphdr;

#endif // MY_LAYER_H
