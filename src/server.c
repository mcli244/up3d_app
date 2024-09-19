#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <semaphore.h>

#include "lwrb.h"
#include "server.h"


#define UP_SERVER_PORT  9001
#define SERVER_RX_BUF   1024
#define DATA_HEADER     0xA53D
typedef struct
{
    uint8_t *rx_buf;
    sem_t rx_sem;
    lwrb_t rx_lwrb;
    int fd;
}SERVER_CTX_Typedef;

typedef struct
{
    uint16_t head;
    uint16_t cmd;
    uint32_t len;
    uint8_t *dat;
    uint16_t crc;
}SERVER_DATA_Typedef;


SERVER_CTX_Typedef   up3d_server;

static int sys_run = 1;

/* 日志时间戳,精确到毫秒 */
char* get_stime(void)
{ 
    static char timestr[200] ={0};
    struct tm * pTempTm;
    struct timeval time;
        
    gettimeofday(&time,NULL);
    pTempTm = localtime(&time.tv_sec);
    if( NULL != pTempTm )
    {
        snprintf(timestr,199,"%04d-%02d-%02d %02d:%02d:%02d.%03ld",
            pTempTm->tm_year+1900,
            pTempTm->tm_mon+1, 
            pTempTm->tm_mday,
            pTempTm->tm_hour, 
            pTempTm->tm_min, 
            pTempTm->tm_sec,
            time.tv_usec/1000);
    }
    return timestr;
}


void * recv_pthread(void *arg)
{
    SERVER_CTX_Typedef *ctx = (SERVER_CTX_Typedef *)arg;
    struct sockaddr_in client_addr;
    int addrLen = sizeof(client_addr);
    int recv_size = 0;

    uint32_t recv_cnt = 0;
    uint32_t index = 0;

    /* 处理从子节点传输过来的数据 */
    uint8_t buf[1024];
    while (1)
    {
        recv_size = recvfrom(ctx->fd, ctx->rx_buf, SERVER_RX_BUF, 0, (struct sockaddr *)&client_addr, &addrLen);
        if(recv_size < 0)
        {
            _LOG(_LOG_ERR, "recvfrom failed");
            break;
        }
        _LOG(_LOG_INFO, "recvfrom [%s:%d] %d bytes",  inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), recv_size);
        lwrb_write(&ctx->rx_lwrb, ctx->rx_buf, recv_size);
        _LOG(_LOG_INFO, "used:%d", (int)lwrb_get_full(&ctx->rx_lwrb));
        sem_post(&ctx->rx_sem);
    }
}

void * dat_pro_pthread(void *arg)
{
    SERVER_CTX_Typedef *ctx = (SERVER_CTX_Typedef *)arg;
    int ret = -1;
    uint32_t recv_cnt = 0;
    SERVER_DATA_Typedef *dat;
    char buf[1024];

    while(1)
    {
        ret = sem_wait(&ctx->rx_sem);

        // TODO: 做错误帧处理
        recv_cnt = lwrb_read(&ctx->rx_lwrb, buf, sizeof(buf));
        if(recv_cnt == 0)
            continue;

        dat = (SERVER_DATA_Typedef *)&buf;
        _LOG(_LOG_INFO, "recv_cnt:%d head:0x%4X cmd:%d len:%d", recv_cnt, dat->head, dat->cmd, dat->len);

    }
}

int upd_server_init(void)
{
    struct sockaddr_in localaddr;
    int sockfd, optval, ret;

    if((sockfd = socket(AF_INET , SOCK_DGRAM , 0)) < 0)
	{
		perror("socket error");
		return sockfd;
	}
    optval = 1;//这个值一定要设置，否则可能导致sendto()失败
    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));
    if(ret < 0)
    {
        close(sockfd);
        _LOG(_LOG_ERR, "setsockopt SO_BROADCAST | SO_REUSEADDR failed");
        perror("setsockopt");
        return sockfd;
    }
    
    // //绑定网卡
    // if(strlen(inf->devif))
    // {
    //     struct ifreq ifr;
    //     memset(&ifr, 0x00, sizeof(ifr));
    //     strncpy(ifr.ifr_name, inf->devif, strlen(inf->devif));
    //     ret = setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));
    //     if(ret < 0)
    //     {
    //         UP_LOG(_LOG_ERR, "bind device failed devif:[%s]", inf->devif);
    //         perror("setsockopt");
    //         goto exit;
    //     }
    // }


    /* 给sockfd帮定一个固定的端口号 */ 
    bzero(&localaddr , sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(UP_SERVER_PORT);
    if(bind(sockfd , (struct sockaddr *)&localaddr , sizeof(localaddr)))
	{
		_LOG(_LOG_ERR, "bind socket failed");
        perror("bind");
        goto exit;
	}
    
    return sockfd;
exit:
    return -1;
}

int upd_server_deinit(int server_fd)
{
    return close(server_fd);
}


static void exist_ignal(int SigNum)
{
    sys_run = 0;
    printf("Signal %d! ,app will exist\n", SigNum);
}

int server_loop(void)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t recv_pid, dat_pro_pid;
    int ret, recv_size, server_fd;

    // signal(SIGINT, exist_ignal);  // Ctrl+C
    signal(SIGTERM, exist_ignal); // kill
    signal(SIGQUIT, exist_ignal);
    // SIGTSTP ctrl+z 挂起

    up3d_server.fd = upd_server_init();
    if(up3d_server.fd < 0)
    {
        _LOG(_LOG_ERR, "upd_server_init failed");
        return -1;
    }

    up3d_server.rx_buf = (uint8_t *)malloc(SERVER_RX_BUF);
    if(up3d_server.rx_buf == NULL)
    {
        _LOG(_LOG_ERR, "rx buffer malloc failed");
        perror("malloc");
        goto exit;
    }

    ret = lwrb_init(&up3d_server.rx_lwrb, up3d_server.rx_buf, SERVER_RX_BUF);
    if(ret != 1)
    {
        _LOG(_LOG_ERR, "lwrb_init failed");
        goto exit1;
    }

    if(0 != sem_init(&up3d_server.rx_sem, 0, 0))
    {
        _LOG(_LOG_ERR, "sem_init(&up3d_server.sem_rx, 0, 0) failed");
        goto exit1;
    }

    ret = pthread_create(&recv_pid , NULL , recv_pthread, (void *)&up3d_server);
    if(0 != ret)
    {
        _LOG(_LOG_ERR, "pthread_create recv_pthread failed");
        perror("recv_pthread");
        goto exit1;
    }

    ret = pthread_create(&dat_pro_pid , NULL , dat_pro_pthread, (void *)&up3d_server);
    if(0 != ret)
    {
        _LOG(_LOG_ERR, "pthread_create recv_pthread failed");
        perror("recv_pthread");
        goto exit1;
    }

    while (sys_run)
    {
        _LOG(_LOG_INFO, "server is runing");
        sleep(30);
    }

    sem_destroy(&up3d_server.rx_sem);

exit1:
    free(up3d_server.rx_buf);
    up3d_server.rx_buf = NULL;

exit:
    upd_server_deinit(server_fd);
    return 0;
}   