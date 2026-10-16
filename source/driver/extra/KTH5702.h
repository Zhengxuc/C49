/*

 * KTH5702 2D 霍尔角度传感器（I2C）

 *

 * 芯片：昆泰芯 KTH5702，通过 I2C0（PA9/PA10）通信。

 * 输出：平面磁场角度 A（0~360°）+ 平面磁感应强度 B（原始量）。

 *

 * 典型用法：

 *   KTH5702_Init();

 *   KTH5702_SetOversampling(KTH5702_OSR_128);

 *   KTH5702_SetMeasInterval(1);          // 1 × 20ms = 20ms 采样间隔

 *   KTH5702_SetContinuousMode();         // 持续测量，循环中调用 KTH5702_ReadData()

 *

 * 协议要点：

 *   - 寄存器索引在总线上需左移 2 位（见 KTH5702.c 中 reg_cmd_byte）
 *   - 测量/模式命令：写 1 字节命令 → Repeated Start → 读 1 字节状态
 *   - 读测量数据：写 0x46 → 读 5 字节（状态 + A + B）

 */



#ifndef __KTH5702_H__

#define __KTH5702_H__



#include "common.h"



/* --------------------------------------------------------------------------

 * I2C 从机地址（7 位）

 * 本工程 IIC 库 slaveaddr 为 7 位地址（IIC.c 发送 slaveaddr<<1|R/W）。

 * 手册 8 位写地址 0xD0 对应 7 位 0x68；根据 A0/A1 硬件接法选择宏。

 * -------------------------------------------------------------------------- */

#define KTH5702_ADDR_A0GND_A1GND    0x68u   /* A0=GND, A1=GND */
#define KTH5702_ADDR_A0VDD_A1GND    0x69u   /* A0=VDD, A1=GND */
#define KTH5702_ADDR_A0GND_A1VDD    0x6Au   /* A0=GND, A1=VDD */
#define KTH5702_ADDR_A0VDD_A1VDD    0x6Bu   /* A0=VDD, A1=VDD */



#ifndef KTH5702_ADDRESS //宏定义默认地址,如果没有定义默认地址

#define KTH5702_ADDRESS             KTH5702_ADDR_A0GND_A1GND

#endif



/* --------------------------------------------------------------------------

 * I2C 测量/模式命令字节
 * 格式：BASE | AXIS_PLANE；写命令后读回 1 字节状态（RESET 除外，仅写）。
 * -------------------------------------------------------------------------- */

#define KTH5702_CMD_AXIS_PLANE      0x06u   /* 平面 BA 通道掩码，OR 到测量命令 */
#define KTH5702_CMD_CONTINUOUS_BASE 0x10u   /* 持续测量模式基址 */
#define KTH5702_CMD_WAKEUP_BASE     0x20u   /* 唤醒模式基址（角度变化超阈值唤醒） */
#define KTH5702_CMD_SINGLE_BASE     0x30u   /* 单次测量模式基址 */
#define KTH5702_CMD_IDLE            0x80u   /* 空闲模式，无轴掩码 */
#define KTH5702_CMD_RESET           0xF0u   /* 软复位，仅写、不读状态 */
#define KTH5702_CMD_DATA_READ_BASE  0x40u   /* 读测量数据命令基址 */



#define KTH5702_CMD_CONTINUOUS      (KTH5702_CMD_CONTINUOUS_BASE | KTH5702_CMD_AXIS_PLANE)  /* 0x16 */
#define KTH5702_CMD_WAKEUP          (KTH5702_CMD_WAKEUP_BASE | KTH5702_CMD_AXIS_PLANE)      /* 0x26 */
#define KTH5702_CMD_SINGLE          (KTH5702_CMD_SINGLE_BASE | KTH5702_CMD_AXIS_PLANE)      /* 0x36 */
#define KTH5702_CMD_DATA_READ       (KTH5702_CMD_DATA_READ_BASE | KTH5702_CMD_AXIS_PLANE)   /* 0x46 */



/* --------------------------------------------------------------------------

 * 寄存器索引（读写时驱动内部左移 2 位后发到总线）

 * -------------------------------------------------------------------------- */

#define KTH5702_REG_STATUS          0x06u   /* 状态寄存器 */
#define KTH5702_REG_WXYTH           0x19u   /* 唤醒角度阈值 */
#define KTH5702_REG_CONFIG1         0x1Cu   /* OSR[10:9] + 数字滤波[2:0] */
#define KTH5702_REG_CONFIG2         0x1Du   /* measSel[8:6] + 测量间隔[5:0] */
#define KTH5702_REG_ZERO            0x1Eu   /* 角度零点校准 */



/** 过采样率，值越大精度越高、转换时间越长 */

typedef enum

{
    KTH5702_OSR_32  = 0x00u,
    KTH5702_OSR_64  = 0x01u,
    KTH5702_OSR_128 = 0x02u,
    KTH5702_OSR_256 = 0x03u

} KTH5702_Osr_t;



/** 数字滤波等级，0=无滤波，3=最强滤波 */

typedef enum
{

    KTH5702_FILTER_0 = 0x00u,
    KTH5702_FILTER_1 = 0x01u,
    KTH5702_FILTER_2 = 0x02u,
    KTH5702_FILTER_3 = 0x03u

} KTH5702_Filter_t;



/** CONFIG2 measSel：选通 BA 平面通道（与手册 measSel 定义一致） */

#define KTH5702_MEAS_BA             0x06u



/* --------------------------------------------------------------------------

 * 状态字节位定义（命令回读或 DATA_READ 首字节）

 * -------------------------------------------------------------------------- */

#define KTH5702_STATUS_DRDY         (1u << 0)   /* 测量数据就绪 */
#define KTH5702_STATUS_SOFTRST      (1u << 1)   /* 软复位标志 */
#define KTH5702_STATUS_RSVD4        (1u << 4)   /* 手册与参考代码定义不一致，不作致命判据 */
#define KTH5702_STATUS_SINGLE       (1u << 5)   /* 当前处于单次模式 */
#define KTH5702_STATUS_WAKEUP       (1u << 6)   /* 当前处于唤醒模式 */
#define KTH5702_STATUS_CONTINUOUS   (1u << 7)   /* 当前处于持续模式 */



/** 一次测量读回的解析结果 */

typedef struct

{

    uint8_t status;         /* 芯片状态字节 */
    uint16_t raw_angle;     /* A：平面角原始量，LSB = 360°/65536 */
    float angle_deg;        /* A 换算为角度（0~360°） */
	uint16_t raw_mag_plane; /* B：平面磁感应强度原始量；物理值 = B×0.60725/Sensitivity(mT) *///注0.60725为磁场强度标定系数

} KTH5702_AngleData_t;



/* ======================== 初始化 / 复位 ======================== */



/** 探测 I2C、写 CONFIG1/2、进入 Idle；失败返回 ANGLE_SENSOR_BUS_ERROR */

ErrorCode KTH5702_Init(void);

/** 先 Idle 再发 RESET 命令，等待 5ms */

ErrorCode KTH5702_SoftReset(void);



/* ======================== 工作模式切换 ======================== */



/** 持续测量（适合主循环周期调用 KTH5702_ReadData） */

ErrorCode KTH5702_SetContinuousMode(void);
/** 唤醒模式（角度变化超 WXYTH 阈值时产生数据） */
ErrorCode KTH5702_SetWakeupMode(void);
/** 单次测量（触发一次转换，需等待 DRDY 后读数据） */
ErrorCode KTH5702_SetSingleMode(void);
/** 空闲模式（低功耗，停止测量） */
ErrorCode KTH5702_SetIdleMode(void);



/* ======================== 数据读取 ======================== */



/** 触发单次模式 → 延时 1ms → ReadData；阻塞式，不适合持续模式场景 */
ErrorCode KTH5702_SingleConvert(KTH5702_AngleData_t *data);
/** 发 DATA_READ 命令读 5 字节帧；持续模式下直接调用即可 */
ErrorCode KTH5702_ReadData(KTH5702_AngleData_t *data);



/* ======================== 参数配置 ======================== */



/** 设置过采样率（CONFIG1 bit[10:9]） */

void KTH5702_SetOversampling(KTH5702_Osr_t osr);
/** 设置数字滤波（CONFIG1 bit[2:0]） */

void KTH5702_SetDigitalFilter(KTH5702_Filter_t filter);

/** 设置持续模式采样间隔，单位 20ms（CONFIG2 bit[5:0]，如 1=20ms） */

void KTH5702_SetMeasInterval(uint8_t interval_ms_20x);

/** 设置唤醒角度阈值（写入 WXYTH 寄存器） */

void KTH5702_SetWakeupThreshold(float angle_deg);



/* ======================== 便捷接口 ======================== */



/** 单次转换后返回原始角度量；失败返回 0 */

uint16_t KTH5702_ReadRawAngle(void);

/** 单次转换后返回角度（°）；失败返回 0.0f */

float KTH5702_ReadAngle(void);

/** 读 STATUS 寄存器低 8 位；I2C 失败返回 0xFF */

uint8_t KTH5702_GetStatus(void);



#endif /* __KTH5702_H__ */


