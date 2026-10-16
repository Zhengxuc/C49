/*
 * @Author: Assistant
 * @Description: ADX111(Q) 16-bit ADC Driver
 * 基于数据手册: 16位配置寄存器, I2C通信, 支持单端/差分输入
 */

#include "ADX111.h"
#include "common.h"
#include <stdio.h>
#include "IIC.h"

static const char *TAG = "ADX111";

// I2C设备地址和总线
#define ADX111_I2C_MASTER_NUM M0P_I2C1

// 寄存器地址指针 (通过I2C第一个字节指定)
#define ADX111_REG_CONVERSION    0x00    // 转换结果寄存器
#define ADX111_REG_CONFIG        0x01    // 配置寄存器
#define ADX111_REG_LO_THRESH     0x02    // 低阈值寄存器
#define ADX111_REG_HI_THRESH     0x03    // 高阈值寄存器

// 配置寄存器默认值 (RESET = 0x8583)
// 默认: 单次模式, PGA=±2.048V, DR=128SPS, MUX=AIN0-AIN1
#define ADX111_CONFIG_DEFAULT    0x8583

// 当前配置寄存器值 (16位)
static uint16_t g_config_reg = ADX111_CONFIG_DEFAULT;

// 单次转换等待时间 (根据数据速率计算)
static uint16_t get_conversion_time_ms(uint16_t dr)
{
    // 转换时间 = 1/DR 秒
    switch(dr) {
        case ADX111_DR_8SPS:    return 125;   // 125ms
        case ADX111_DR_16SPS:   return 63;    // 62.5ms
        case ADX111_DR_32SPS:   return 32;    // 31.25ms
        case ADX111_DR_64SPS:   return 16;    // 15.625ms
        case ADX111_DR_128SPS:  return 8;     // 7.8125ms
        case ADX111_DR_250SPS:  return 4;     // 4ms
        case ADX111_DR_475SPS:  return 3;     // ~2.1ms
        case ADX111_DR_860SPS:  return 2;     // ~1.16ms
        default:                return 8;
    }
}

// 向ADX111写入16位寄存器
static ErrorCode adx111_write_register(uint8_t reg_addr, uint16_t value)
{
    uint8_t write_buf[3];
    write_buf[0] = reg_addr;
    write_buf[1] = (value >> 8) & 0xFF;   // 高字节
    write_buf[2] = value & 0xFF;          // 低字节
    
    if (0 != I2C_MasterWriteData(ADX111_I2C_MASTER_NUM, ADX111_ADDRESS, write_buf[0], &write_buf[1], 2))
    {
        return ADC_SENSOR_BUS_ERROR;
    }
    return NONE_ERROR;
}

// 从ADX111读取16位寄存器
static ErrorCode adx111_read_register(uint8_t reg_addr, uint16_t *value)
{
    uint8_t read_buf[2];
    
    // 先发送寄存器地址
    if (0 != I2C_MasterWriteData(ADX111_I2C_MASTER_NUM, ADX111_ADDRESS, reg_addr, NULL, 0))
    {
        return ADC_SENSOR_BUS_ERROR;
    }
    
    // 读取2字节数据
    if (0 != I2C_MasterReadData(ADX111_I2C_MASTER_NUM, ADX111_ADDRESS, 0xFF, read_buf, 2))
    {
        return ADC_SENSOR_BUS_ERROR;
    }
    
    *value = (read_buf[0] << 8) | read_buf[1];
    return NONE_ERROR;
}

// 初始化ADX111
ErrorCode ADX111_Init(void)
{
    uint16_t device_id;
    uint16_t readback;

    // 尝试读取配置寄存器，验证通信是否正常
    if (NONE_ERROR != adx111_read_register(ADX111_REG_CONFIG, &device_id))
    {
        DEBUG_LOGE(TAG, "ADX111 communication failed (read config)");
        return ADC_SENSOR_BUS_ERROR;
    }
    DEBUG_LOGI(TAG, "ADX111 config readback (power-on): 0x%04X", device_id);

    // 设置默认配置: 连续模式, AIN0-AIN1差分, PGA=±2.048V, DR=128SPS
    // Bit15 OS=1, Bit14-12 MUX=000, Bit11-9 PGA=010, Bit8 MODE=0(连续),
    // Bit7-5 DR=100, Bit4-2 COMP=000, Bit1-0 COMP_QUE=11
    // 0x8483 = 1000 0100 1000 0011
    g_config_reg = 0x8483;

    // 写入配置寄存器
    if (NONE_ERROR != adx111_write_register(ADX111_REG_CONFIG, g_config_reg))
    {
        DEBUG_LOGE(TAG, "ADX111 write config failed");
        return ADC_SENSOR_BUS_ERROR;
    }

    // 读回确认写入成功
    if (NONE_ERROR != adx111_read_register(ADX111_REG_CONFIG, &readback))
    {
        DEBUG_LOGE(TAG, "ADX111 readback config failed");
        return ADC_SENSOR_BUS_ERROR;
    }
    // 连续模式下 OS 位读取恒为 1，因此屏蔽 OS 位后比较
    if ((readback & 0x7FFF) != (g_config_reg & 0x7FFF))
    {
        DEBUG_LOGE(TAG, "ADX111 config mismatch: wrote 0x%04X, read 0x%04X", g_config_reg, readback);
        return ADC_SENSOR_BUS_ERROR;
    }

    DEBUG_LOGI(TAG, "ADX111 init OK, config=0x%04X (continuous mode)", g_config_reg);
    return NONE_ERROR;
}

// 设置ADX111配置
void ADX111_SetConfig(ADX111_Mux_t mux, ADX111_Pga_t pga, ADX111_Dr_t dr, ADX111_Mode_t mode)
{
    // 保留OS位和比较器相关位，更新MUX、PGA、DR、MODE
    // 配置寄存器结构:
    // Bit15: OS (操作状态)
    // Bit14-12: MUX[2:0]
    // Bit11-9: PGA[2:0]  
    // Bit8: MODE
    // Bit7-5: DR[2:0]
    // Bit4: COMP_MODE
    // Bit3: COMP_POL
    // Bit2: COMP_LAT
    // Bit1-0: COMP_QUE[1:0]
    
    g_config_reg = (g_config_reg & ~(0x7000)) | ((mux & 0x07) << 12);   // 更新MUX
    g_config_reg = (g_config_reg & ~(0x0E00)) | ((pga & 0x07) << 9);    // 更新PGA
    g_config_reg = (g_config_reg & ~(0x0100)) | ((mode & 0x01) << 8);    // 更新MODE
    g_config_reg = (g_config_reg & ~(0x00E0)) | ((dr & 0x07) << 5);      // 更新DR
    
    // 保持比较器配置为默认禁用状态 (COMP_QUE=11)
    
    adx111_write_register(ADX111_REG_CONFIG, g_config_reg);
    
    DEBUG_LOGI(TAG, "Config set: 0x%04X, MUX=%d, PGA=%d, DR=%d, MODE=%d", 
               g_config_reg, mux, pga, dr, mode);
}

// 设置单端输入通道 (兼容MS1112的使用方式)
void ADX111_SetSingleChannel(uint8_t ch)
{
    ADX111_Mux_t mux;
    
    switch(ch) {
        case 0: mux = ADX111_MUX_SE_0; break;
        case 1: mux = ADX111_MUX_SE_1; break;
        case 2: mux = ADX111_MUX_SE_2; break;
        case 3: mux = ADX111_MUX_SE_3; break;
        default: mux = ADX111_MUX_SE_0; break;
    }
    
    ADX111_SetConfig(mux, ADX111_PGA_2_048V, ADX111_DR_128SPS, ADX111_MODE_CONTINUOUS);
}

// 触发单次转换并读取结果
int16_t ADX111_SingleShot(ADX111_Mux_t mux, ADX111_Pga_t pga)
{
    uint16_t config;
    uint16_t result;
    uint16_t wait_ms;
    uint8_t dr;
    
    // 获取当前数据速率配置
    dr = (g_config_reg >> 5) & 0x07;
    wait_ms = get_conversion_time_ms(dr);
    
    // 构建单次转换配置: OS位设为1启动转换
    config = (g_config_reg & ~(0x7000 | 0x0E00 | 0x0100)) | 
             ((mux & 0x07) << 12) | 
             ((pga & 0x07) << 9) |
             (ADX111_MODE_SINGLE << 8) |
             (1 << 15);  // OS=1 启动转换
    
    // 写入配置启动单次转换
    adx111_write_register(ADX111_REG_CONFIG, config);
    
    // 等待转换完成
    delay1ms(wait_ms + 1);
    
    // 检查转换是否完成 (读取OS位)
    adx111_read_register(ADX111_REG_CONFIG, &config);
    if ((config >> 15) & 0x01) {
        // OS=1表示转换未完成或设备空闲，再等一会儿
        delay1ms(wait_ms);
        adx111_read_register(ADX111_REG_CONFIG, &config);
    }
    
    // 读取转换结果
    adx111_read_register(ADX111_REG_CONVERSION, &result);
    
    // 返回有符号16位结果
    return (int16_t)result;
}

// 读取指定通道 (兼容原有接口，单次转换模式)
int16_t ADX111_Read(uint8_t ch)
{
    ADX111_Mux_t mux;
    
    // 映射通道到单端输入
    switch(ch) {
        case 0: mux = ADX111_MUX_SE_0; break;
        case 1: mux = ADX111_MUX_SE_1; break;
        case 2: mux = ADX111_MUX_SE_2; break;
        case 3: mux = ADX111_MUX_SE_3; break;
        default: mux = ADX111_MUX_SE_0; break;
    }
    
    return ADX111_SingleShot(mux, ADX111_PGA_2_048V);
}

// 读取差分输入 (AIN0 - AIN1)
int16_t ADX111_ReadDiff(void)
{
    return ADX111_SingleShot(ADX111_MUX_DIFF_0_1, ADX111_PGA_2_048V);
}

// 获取最新转换结果 (不触发新转换，适用于连续模式)
int16_t ADX111_GetLastResult(void)
{
    uint16_t result;
    static uint32_t fail_cnt = 0;

    if (NONE_ERROR != adx111_read_register(ADX111_REG_CONVERSION, &result))
    {
        fail_cnt++;
        // 限流：每 100 次失败打一次日志，避免刷屏
        if ((fail_cnt % 100) == 0)
        {
            DEBUG_LOGE(TAG, "ADX111 read conversion failed, cnt=%lu", (unsigned long)fail_cnt);
        }
        return 0;
    }

    return (int16_t)result;
}
