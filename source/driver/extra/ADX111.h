/*
 * @Author: Assistant
 * @Description: ADX111(Q) 16-bit ADC Driver Header
 */

#ifndef __ADX111_H__
#define __ADX111_H__

#include "common.h"

// 设备I2C地址 (由ADDR引脚连接方式决定)
// GND: 0x90, VDD: 0x92, SDA: 0x94, SCL: 0x96
// 注意: I2C地址是8位格式(包含R/W位)，实际使用时根据你的IIC库调整
#define ADX111_ADDR_GND     0x48    // ADDR接GND
#define ADX111_ADDR_VDD     0x49    // ADDR接VDD  
#define ADX111_ADDR_SDA     0x4A    // ADDR接SDA
#define ADX111_ADDR_SCL     0x4B    // ADDR接SCL

// 默认使用地址 (根据硬件连接修改)
#define ADX111_ADDRESS      ADX111_ADDR_GND

// 输入通道配置 (MUX[2:0])
typedef enum {
    ADX111_MUX_DIFF_0_1 = 0x00,    // AIN0 - AIN1 (差分)
    ADX111_MUX_DIFF_0_3 = 0x01,    // AIN0 - AIN3 (差分)
    ADX111_MUX_DIFF_1_3 = 0x02,    // AIN1 - AIN3 (差分)
    ADX111_MUX_DIFF_2_3 = 0x03,    // AIN2 - AIN3 (差分)
    ADX111_MUX_SE_0     = 0x04,    // AIN0 对地 (单端)
    ADX111_MUX_SE_1     = 0x05,    // AIN1 对地 (单端)
    ADX111_MUX_SE_2     = 0x06,    // AIN2 对地 (单端)
    ADX111_MUX_SE_3     = 0x07     // AIN3 对地 (单端)
} ADX111_Mux_t;

// 满量程范围配置 (PGA[2:0])
typedef enum {
    ADX111_PGA_6_144V   = 0x00,    // ±6.144V
    ADX111_PGA_4_096V   = 0x01,    // ±4.096V
    ADX111_PGA_2_048V   = 0x02,    // ±2.048V (默认)
    ADX111_PGA_1_024V   = 0x03,    // ±1.024V
    ADX111_PGA_0_512V   = 0x04,    // ±0.512V
    ADX111_PGA_0_256V   = 0x05     // ±0.256V
} ADX111_Pga_t;

// 数据速率配置 (DR[2:0])
typedef enum {
    ADX111_DR_8SPS      = 0x00,    // 8 SPS
    ADX111_DR_16SPS     = 0x01,    // 16 SPS
    ADX111_DR_32SPS     = 0x02,    // 32 SPS
    ADX111_DR_64SPS     = 0x03,    // 64 SPS
    ADX111_DR_128SPS    = 0x04,    // 128 SPS (默认)
    ADX111_DR_250SPS    = 0x05,    // 250 SPS
    ADX111_DR_475SPS    = 0x06,    // 475 SPS
    ADX111_DR_860SPS    = 0x07     // 860 SPS
} ADX111_Dr_t;

// 工作模式
typedef enum {
    ADX111_MODE_CONTINUOUS = 0x00,  // 连续转换模式
    ADX111_MODE_SINGLE     = 0x01   // 单次转换模式 (默认)
} ADX111_Mode_t;

// 函数声明
ErrorCode ADX111_Init(void);
int16_t ADX111_Read(uint8_t ch);
int16_t ADX111_ReadDiff(void);
void ADX111_SetConfig(ADX111_Mux_t mux, ADX111_Pga_t pga, ADX111_Dr_t dr, ADX111_Mode_t mode);
void ADX111_SetSingleChannel(uint8_t ch);

// 获取最新转换结果 (不触发新转换)
int16_t ADX111_GetLastResult(void);

// 触发单次转换并等待结果
int16_t ADX111_SingleShot(ADX111_Mux_t mux, ADX111_Pga_t pga);

#endif /* __ADX111_H__ */
