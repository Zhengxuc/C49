/*
 * KTH5702 2D 霍尔角度传感器（I2C）
 * 命令/寄存器编码与昆泰芯数据手册一致；寄存器索引在总线上需左移 2 位。
 */

#ifndef __KTH5702_H__
#define __KTH5702_H__

#include "common.h"

/*
 * 本工程 IIC 库 slaveaddr 为 7 位地址（IIC.c 中发送字节为 slaveaddr<<1|R/W）。
 * 手册 8 位写地址 0xD0 时，此处应填 7 位地址 0x68（与 A0/A1 接法对应宏一致）。
 */
#define KTH5702_ADDR_A0GND_A1GND    0x68u
#define KTH5702_ADDR_A0VDD_A1GND    0x69u
#define KTH5702_ADDR_A0GND_A1VDD    0x6Au
#define KTH5702_ADDR_A0VDD_A1VDD    0x6Bu

#ifndef KTH5702_ADDRESS
#define KTH5702_ADDRESS             KTH5702_ADDR_A0GND_A1GND
#endif

/*
 * I2C 测量类命令：SLA+W 后写 1 字节命令，再 SLA+R 读 1 字节状态（复位除外，仅写）。
 * 持续 0x16、唤醒 0x26、单次 0x36 = BASE|平面掩码 0x06；空闲固定 0x80。
 */
#define KTH5702_CMD_AXIS_PLANE      0x06u
#define KTH5702_CMD_CONTINUOUS_BASE 0x10u
#define KTH5702_CMD_WAKEUP_BASE     0x20u
#define KTH5702_CMD_SINGLE_BASE     0x30u
#define KTH5702_CMD_IDLE            0x80u /* B10000000，无轴掩码 */
#define KTH5702_CMD_RESET           0xF0u /* B11110000，仅写、不读状态 */
#define KTH5702_CMD_DATA_READ_BASE  0x40u

#define KTH5702_CMD_CONTINUOUS      (KTH5702_CMD_CONTINUOUS_BASE | KTH5702_CMD_AXIS_PLANE)
#define KTH5702_CMD_WAKEUP          (KTH5702_CMD_WAKEUP_BASE | KTH5702_CMD_AXIS_PLANE)
#define KTH5702_CMD_SINGLE          (KTH5702_CMD_SINGLE_BASE | KTH5702_CMD_AXIS_PLANE)
#define KTH5702_CMD_DATA_READ       (KTH5702_CMD_DATA_READ_BASE | KTH5702_CMD_AXIS_PLANE)

/* 寄存器索引（读写时内部左移 2 位） */
#define KTH5702_REG_STATUS          0x06u
#define KTH5702_REG_WXYTH           0x19u
#define KTH5702_REG_CONFIG1         0x1Cu
#define KTH5702_REG_CONFIG2         0x1Du
#define KTH5702_REG_ZERO            0x1Eu

typedef enum
{
    KTH5702_OSR_32  = 0x00u,
    KTH5702_OSR_64  = 0x01u,
    KTH5702_OSR_128 = 0x02u,
    KTH5702_OSR_256 = 0x03u
} KTH5702_Osr_t;

typedef enum
{
    KTH5702_FILTER_0 = 0x00u,
    KTH5702_FILTER_1 = 0x01u,
    KTH5702_FILTER_2 = 0x02u,
    KTH5702_FILTER_3 = 0x03u
} KTH5702_Filter_t;

/* measSel：选通 BA 通道（与手册 measSel 定义一致时保持 0x06） */
#define KTH5702_MEAS_BA             0x06u

#define KTH5702_STATUS_DRDY         (1u << 0)
#define KTH5702_STATUS_SOFTRST      (1u << 1)
/* 手册与参考代码对 status 位 4 定义不一致，驱动内不再据此判致命错误 */
#define KTH5702_STATUS_RSVD4        (1u << 4)
#define KTH5702_STATUS_SINGLE       (1u << 5)
#define KTH5702_STATUS_WAKEUP       (1u << 6)
#define KTH5702_STATUS_CONTINUOUS   (1u << 7)

typedef struct
{
    uint8_t status;
    uint16_t raw_angle;   /* A：平面夹角对应量，LSB = 360°/65536 */
    float angle_deg;
    uint16_t raw_mag_plane; /* B：平面磁感应强度对应量；物理值见手册 B×0.60725/Sensitivity */
} KTH5702_AngleData_t;

ErrorCode KTH5702_Init(void);
ErrorCode KTH5702_SoftReset(void);

ErrorCode KTH5702_SetContinuousMode(void);
ErrorCode KTH5702_SetWakeupMode(void);
ErrorCode KTH5702_SetSingleMode(void);
ErrorCode KTH5702_SetIdleMode(void);

ErrorCode KTH5702_SingleConvert(KTH5702_AngleData_t *data);
ErrorCode KTH5702_ReadData(KTH5702_AngleData_t *data);

void KTH5702_SetOversampling(KTH5702_Osr_t osr);
void KTH5702_SetDigitalFilter(KTH5702_Filter_t filter);
void KTH5702_SetMeasInterval(uint8_t interval_ms_20x);
void KTH5702_SetWakeupThreshold(float angle_deg);

uint16_t KTH5702_ReadRawAngle(void);
float KTH5702_ReadAngle(void);
uint8_t KTH5702_GetStatus(void);

#endif /* __KTH5702_H__ */
