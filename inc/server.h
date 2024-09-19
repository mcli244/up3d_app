#ifndef __SERVER_H__
#define __SERVER_H__

#include <netinet/in.h>
#include <semaphore.h>
#include "lwrb.h"

#define UP_SERVER_PORT  9001
#define SERVER_RX_BUF   1024
#define SERVER_TX_BUF   1024

typedef struct
{
    uint8_t *rx_buf;
    uint8_t *tx_buf;
    sem_t rx_sem;
    sem_t tx_sem;
    lwrb_t rx_lwrb;
    lwrb_t tx_lwrb;
    int fd;
    uint8_t is_connected;
    struct sockaddr_in clientaddr;
}SERVER_CTX_Typedef;

typedef struct
{
    uint16_t head;
    uint16_t cmd;
    uint32_t len;
    uint8_t dat[1];
}SERVER_DATA_Typedef;

#define SERVER_DATA_HEAD_SIZE 8


void server_init(void);
int server_loop(void);

#endif /*__SERVER_H__*/