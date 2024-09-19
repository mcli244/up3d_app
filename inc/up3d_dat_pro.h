#ifndef __UP3D_DAT_PRO_H__
#define __UP3D_DAT_PRO_H__

#include "stdint.h"
#include "server.h"

#define DATA_HEADER     0xA53D

#define UP3D_CMD_BROADCAST  0x10


typedef struct
{
    SERVER_CTX_Typedef *ctx;
    uint8_t *rxdat;
    uint8_t *txdat;
    uint32_t rxcnt;
    uint32_t txcnt;
    uint8_t respond;
}SERVER_DATA_Pro_Typedef;

int dat_pro(SERVER_DATA_Pro_Typedef *pDat);

#endif