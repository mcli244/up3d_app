#include "up3d_dat_pro.h"
#include "up3d.h"
#include "crc-itu-t.h"

#define MAGIC_CRC           0x1E50

char *get_local_ip_str(void)
{
    return "192.168.8.110";
}

static int _make_respond_msg(uint16_t cmd, uint8_t *dat, uint32_t len, 
                            SERVER_DATA_Typedef *txdat, uint32_t *txdat_cnt)
{
    uint16_t crc16 = 0;

    txdat->head = DATA_HEADER;
    txdat->cmd = UP3D_CMD_BROADCAST;
    txdat->len = len;
    if(len)
        memcpy(txdat->dat, dat, len);
    
    // crc 2B
    crc16 = crc_itu_t(MAGIC_CRC, (uint8_t *)&txdat, SERVER_DATA_HEAD_SIZE + len);
    txdat->dat[len] = crc16>>8 & 0xff;
    txdat->dat[len+1] = crc16 & 0xff;

    *txdat_cnt = SERVER_DATA_HEAD_SIZE + len + 2;
    return 0;
}

int boardcast_pro(SERVER_DATA_Pro_Typedef *pDat)
{
    int ret = -1;
    int len = 0;
    char buf[64] = {0};

    len = sprintf(buf, "%s:%d", get_local_ip_str(), UP_SERVER_PORT);
    return _make_respond_msg(UP3D_CMD_BROADCAST, buf, len, (SERVER_DATA_Typedef *)pDat->txdat, &pDat->txcnt);
}


int dat_pro(SERVER_DATA_Pro_Typedef *pDat)
{
    int ret = -1;
    
    SERVER_CTX_Typedef  *ctx    = (SERVER_CTX_Typedef *)pDat->ctx;
    SERVER_DATA_Typedef *rxdat  = (SERVER_DATA_Typedef *)pDat->rxdat;
    SERVER_DATA_Typedef *txdat  = (SERVER_DATA_Typedef *)pDat->txdat;

    if(!ctx || !rxdat || !txdat || !pDat)
    {
        _LOG(_LOG_ERR, "dat is null");
        return -1;
    }

    if(rxdat->head != DATA_HEADER)
    {
        _LOG(_LOG_ERR, "data formatting error!");
        for(int i=0; i< sizeof(SERVER_DATA_Typedef); i++)
            printf("%02X ", *(uint8_t *)&rxdat[i]);
        printf("\n");
        return -1;
    }

    _LOG(_LOG_INFO, "recv_cnt:%d head:0x%4X cmd:%d len:%d", pDat->rxcnt, rxdat->head, rxdat->cmd, rxdat->len);
        
    switch(rxdat->cmd)
    {
        case UP3D_CMD_BROADCAST:
            pDat->respond = 1;
            ret = boardcast_pro(pDat);
            break;
        default:
            break;
    }

    return ret;
}