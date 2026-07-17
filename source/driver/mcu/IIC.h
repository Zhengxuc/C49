/*
 * @Descripttion:
 * @version:
 * @Author: Youliangxin
 * @Date: 2023-03-14 14:06:13
 * @LastEditors: sueRimn
 * @LastEditTime: 2023-03-14 17:54:11
 */
#ifndef IIC_H__
#define IIC_H__

#include "i2c.h"

void IIC_Init(void);

en_result_t I2C_MasterWriteData(M0P_I2C_TypeDef *I2CX, uint8_t slaveaddr, uint8_t u8Addr, uint8_t *pu8Data, uint32_t u32Len);
en_result_t I2C_MasterReadData(M0P_I2C_TypeDef *I2CX, uint8_t slaveaddr, uint8_t u8Addr, uint8_t *pu8Data, uint32_t u32Len);

/** 单次总线事务：START + SLA+W + 写 pu8WrData[u32WrLen] + Sr + SLA+R + 读 pu8RdData[u32RdLen] + STOP（中间无 STOP） */
en_result_t I2C_MasterWriteThenRead(M0P_I2C_TypeDef *I2CX, uint8_t slaveaddr, const uint8_t *pu8WrData, uint32_t u32WrLen, uint8_t *pu8RdData, uint32_t u32RdLen);

#endif
