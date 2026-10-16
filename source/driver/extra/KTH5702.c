/*
 * KTH5702 2D 霍尔角度传感器驱动
 *
 * 硬件：I2C0（M0P_I2C0），SCL=PA9，SDA=PA10，100kHz
 * 依赖：IIC.c 中 I2C_MasterWriteThenRead / I2C_MasterWriteData
 *
 * I2C 帧格式（本驱动三种操作）：
 *   1. 写寄存器：W [0x60][D15..8][D7..0][reg<<2] → R [status]
 *   2. 读寄存器：W [0x50][reg<<2]              → R [status][D15..8][D7..0]
 *   3. 模式命令：W [cmd]                       → R [status]
 *   4. 读数据：  W [0x46]                      → R [status][A_H][A_L][B_H][B_L]
 *
 * --------------------------------------------------------------------------
 * 本工程使用情况一览（标注依据：全工程 grep 调用点）
 *
 *   【使用中】KTH5702_Init              System.c  上电初始化（RaisingExceptions）
 *   【使用中】KTH5702_SetContinuousMode System.c  上电进入连续测量模式
 *   【使用中】KTH5702_ReadData          main.c    ControlTask 每 50ms 读角度
 *   【内部用】KTH5702_SetIdleMode       Init/SoftReset 内部调用
 *   【内部用】KTH5702_SingleConvert     便捷读取 API 内部调用
 *
 *   【预留】  KTH5702_SoftReset / SetWakeupMode / SetSingleMode /
 *             SetOversampling / SetDigitalFilter / SetMeasInterval /
 *             SetWakeupThreshold / ReadRawAngle / ReadAngle / GetStatus
 *             （唤醒模式、低功耗场景预留，当前无调用点）
 * --------------------------------------------------------------------------
 */

#include "KTH5702.h"
#include "common.h"
#include "IIC.h"

static const char *TAG = "KTH5702";

#define KTH5702_I2Cx                M0P_I2C0
#define KTH5702_REG_CMD_SHIFT       2u      /* 寄存器索引左移位数，手册协议要求 */

/* 总线寄存器访问前缀字节（SLA+W 后首字节） */
#define KTH5702_BUS_READ_REG        0x50u
#define KTH5702_BUS_WRITE_REG       0x60u

/* 运行时配置缓存，Init 时写入芯片，SetXxx 时同步更新 */
static uint8_t g_meas_interval = 0;
static KTH5702_Osr_t g_osr = KTH5702_OSR_128;
static KTH5702_Filter_t g_filter = KTH5702_FILTER_0;

/* --------------------------------------------------------------------------
 * 内部辅助函数
 * -------------------------------------------------------------------------- */

/** 将寄存器索引转为总线命令字节（index << 2） */
static uint8_t reg_cmd_byte(uint8_t reg_index)
{
    return (uint8_t)(reg_index << KTH5702_REG_CMD_SHIFT);
}

/**
 * 写 16 位寄存器
 * 帧：START, SLA+W, 0x60, D15..8, D7..0, reg<<2, Sr, SLA+R, status, STOP
 * @return true=成功
 */
static bool i2c_write_reg16(uint8_t reg_index, uint16_t value)
{
    uint8_t wr[4];
    uint8_t st;

    wr[0] = KTH5702_BUS_WRITE_REG;
    wr[1] = (uint8_t)((value >> 8) & 0xFFu);    /* value高字节 */
    wr[2] = (uint8_t)(value & 0xFFu);           /* value低字节 */
    wr[3] = reg_cmd_byte(reg_index);
    return (Ok == I2C_MasterWriteThenRead(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, wr, 4u, &st, 1u));
}

/**
 * 读 16 位寄存器
 * 帧：START, SLA+W, 0x50, reg<<2, Sr, SLA+R, status, D15..8, D7..0, STOP
 * rx[0]=状态（丢弃），rx[1..2]=16 位数据（大端）
 */
static bool i2c_read_reg16(uint8_t reg_index, uint16_t *value)
{
    uint8_t wr[2];
    uint8_t rx[3];

    if (value == NULL)
    {
        return false;
    }
    wr[0] = KTH5702_BUS_READ_REG;
    wr[1] = reg_cmd_byte(reg_index);
    if (Ok != I2C_MasterWriteThenRead(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, wr, 2u, rx, 3u))
    {
        return false;
    }
    *value = (uint16_t)(((uint16_t)rx[1] << 8) | rx[2]);
    (void)rx[0];
    return true;
}

/**
 * 读测量数据帧（CMD_DATA_READ = 0x46）
 * 回读 5 字节：status(1) + raw_angle(2, 大端) + raw_mag_plane(2, 大端)
 * angle_deg = raw_angle × (360 / 65536)
 */
static ErrorCode kth5702_read_data_frame(KTH5702_AngleData_t *data)
{
    uint8_t rx[5];
    uint16_t raw_angle;
    uint16_t raw_b;

    if (data == NULL)
    {
        return ERROR_INVALID_PARAM;
    }

    uint8_t cmdb = KTH5702_CMD_DATA_READ;
    if (Ok != I2C_MasterWriteThenRead(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, &cmdb, 1u, rx, 5u))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }

    data->status = rx[0];                                   /* 第一个元素 */
    raw_angle = (uint16_t)(((uint16_t)rx[1] << 8) | rx[2]); /* 第二、三个元素 */
    raw_b = (uint16_t)(((uint16_t)rx[3] << 8) | rx[4]);     /* 第四、五个元素 */

    data->raw_angle = raw_angle;                            /* 获得原始值 */
    data->angle_deg = (float)raw_angle * (360.0f / 65536.0f); /* 转换为360度 16位0-65536 */
    data->raw_mag_plane = raw_b;                            /* 磁场强度 */

    return NONE_ERROR;
}

/**
 * 发送模式/测量命令并读回 1 字节状态
 * 帧：START, SLA+W, cmd, Sr, SLA+R, status, STOP
 */
static ErrorCode send_cmd_read_status(uint8_t cmd, uint8_t *status_out)
{
    uint8_t st;

    if (Ok != I2C_MasterWriteThenRead(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, &cmd, 1u, &st, 1u))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    if (status_out != NULL)
    {
        *status_out = st;
    }
    return NONE_ERROR;
}

/* --------------------------------------------------------------------------
 * 公开 API：初始化 / 复位
 * -------------------------------------------------------------------------- */

/** 【本工程使用中】System.c 上电初始化；SoftReset 后配置丢失也需重走本函数 */
ErrorCode KTH5702_Init(void)
{
    uint16_t regv;
    ErrorCode ec;

    delay1ms(5);    /* 上电稳定 */

    /* 读 STATUS 探测 I2C 通信是否正常 */
    if (!i2c_read_reg16(KTH5702_REG_STATUS, &regv))
    {
        DEBUG_LOGE(TAG, "probe STATUS failed (check 7-bit I2C addr / wiring)");
        return ANGLE_SENSOR_BUS_ERROR;
    }

    /* CONFIG1: OSR[10:9] | filter[2:0] */
    if (!i2c_write_reg16(KTH5702_REG_CONFIG1, (uint16_t)(((uint16_t)g_osr << 9) | (uint16_t)g_filter)))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }

    /* CONFIG2: measSel[8:6]=BA | interval[5:0] */
    if (!i2c_write_reg16(KTH5702_REG_CONFIG2, (uint16_t)(((uint16_t)KTH5702_MEAS_BA << 6) | (uint16_t)g_meas_interval)))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }

    ec = KTH5702_SetIdleMode();
    if (ec != NONE_ERROR)
    {
        return ec;
    }

    DEBUG_LOGI(TAG, "init ok, STATUS=0x%04X", regv);
    return NONE_ERROR;
}

/** 【预留】软复位：RESET 后 CONFIG1/2 恢复默认，必须重新调用 KTH5702_Init() */
ErrorCode KTH5702_SoftReset(void)
{
    (void)KTH5702_SetIdleMode();
    /* RESET 命令仅写、无读回阶段 */
    if (Ok != I2C_MasterWriteData(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, KTH5702_CMD_RESET, NULL, 0))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    delay1ms(5);
    return NONE_ERROR;
}

/* --------------------------------------------------------------------------
 * 公开 API：工作模式
 * -------------------------------------------------------------------------- */

/** 【本工程使用中】System.c 上电进入连续测量模式，之后 ControlTask 周期 ReadData */
ErrorCode KTH5702_SetContinuousMode(void)
{
    uint8_t status;

    /* 进入持续模式前先刷新 CONFIG2（含采样间隔） */
    if (!i2c_write_reg16(KTH5702_REG_CONFIG2, (uint16_t)(((uint16_t)KTH5702_MEAS_BA << 6) | (uint16_t)g_meas_interval)))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    if (NONE_ERROR != send_cmd_read_status(KTH5702_CMD_CONTINUOUS, &status))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    /* bit7 未置位仅告警，I2C 成功即认为模式切换完成 */
    if ((status & KTH5702_STATUS_CONTINUOUS) == 0u)
    {
        DEBUG_LOGW(TAG, "continuous: status 0x%02X (no bit7), I2C ok", status);
    }
    return NONE_ERROR;
}

/** 【预留】唤醒模式：休眠中角度变化超 WXYTH 阈值自动测量（低功耗场景） */
ErrorCode KTH5702_SetWakeupMode(void)
{
    uint8_t status;

    if (NONE_ERROR != send_cmd_read_status(KTH5702_CMD_WAKEUP, &status))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    if ((status & KTH5702_STATUS_WAKEUP) == 0u)
    {
        DEBUG_LOGW(TAG, "wakeup: status 0x%02X (no bit6), I2C ok", status);
    }
    return NONE_ERROR;
}

/** 【预留】单次转换模式（便捷 API KTH5702_SingleConvert 内部使用） */
ErrorCode KTH5702_SetSingleMode(void)
{
    uint8_t status;

    if (NONE_ERROR != send_cmd_read_status(KTH5702_CMD_SINGLE, &status))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    if ((status & KTH5702_STATUS_SINGLE) == 0u)
    {
        DEBUG_LOGW(TAG, "single: status 0x%02X (no bit5), I2C ok", status);
    }
    return NONE_ERROR;
}

/** 【内部用】空闲模式：Init/SoftReset 内部调用 */
ErrorCode KTH5702_SetIdleMode(void)
{
    uint8_t status;

    if (NONE_ERROR != send_cmd_read_status(KTH5702_CMD_IDLE, &status))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    return NONE_ERROR;
}

/* --------------------------------------------------------------------------
 * 公开 API：数据读取
 * -------------------------------------------------------------------------- */

/**
 * 【内部用】单次转换：切单次模式 → 等转换 → 读帧
 * 便捷 API ReadRawAngle/ReadAngle 内部调用；注意连续模式下使用会打断连续采样
 */
ErrorCode KTH5702_SingleConvert(KTH5702_AngleData_t *data)
{
    if (NONE_ERROR != KTH5702_SetSingleMode())
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }

    delay1ms(1);    /* 等待单次转换完成（OSR_128 约需数 ms，此处偏保守） */
    return KTH5702_ReadData(data);
}

/**
 * 【本工程使用中】main.c ControlTask 每 50ms 调用
 * 前提：已由 SetContinuousMode 进入连续模式，直读最新转换结果即可
 */
ErrorCode KTH5702_ReadData(KTH5702_AngleData_t *data)
{
    if (data == NULL)
    {
        return ERROR_INVALID_PARAM;
    }
    return kth5702_read_data_frame(data);
}

/* --------------------------------------------------------------------------
 * 公开 API：参数配置
 * -------------------------------------------------------------------------- */

/** 【预留】过采样率设置（OSR 越大噪声越小、转换越慢） */
void KTH5702_SetOversampling(KTH5702_Osr_t osr)
{
    uint16_t config;
    g_osr = osr;
    if (!i2c_read_reg16(KTH5702_REG_CONFIG1, &config))
    {
        return;
    }
    config = (uint16_t)((config & (uint16_t) ~(0x03u << 9)) | (uint16_t)((uint16_t)osr << 9));
    (void)i2c_write_reg16(KTH5702_REG_CONFIG1, config);
}

/** 【预留】数字滤波设置 */
void KTH5702_SetDigitalFilter(KTH5702_Filter_t filter)
{
    uint16_t config;
    g_filter = filter;
    if (!i2c_read_reg16(KTH5702_REG_CONFIG1, &config))
    {
        return;
    }
    config = (uint16_t)((config & (uint16_t) ~0x07u) | (uint16_t)filter);
    (void)i2c_write_reg16(KTH5702_REG_CONFIG1, config);
}

/** 【预留】连续模式采样间隔：interval_ms_20x × 20ms（如 1 → 20ms） */
void KTH5702_SetMeasInterval(uint8_t interval_ms_20x)
{
    uint16_t config;
    g_meas_interval = interval_ms_20x;
    if (!i2c_read_reg16(KTH5702_REG_CONFIG2, &config))
    {
        return;
    }
    config = (uint16_t)((config & (uint16_t) ~0x3Fu) | (uint16_t)(interval_ms_20x & 0x3Fu));
    (void)i2c_write_reg16(KTH5702_REG_CONFIG2, config);
}

/** 【预留】唤醒模式角度阈值：角度变化超过该值芯片自动醒来测量 */
void KTH5702_SetWakeupThreshold(float angle_deg)
{
    uint16_t threshold_raw;
    /* 角度 → 16 位原始量：raw = angle × (65536 / 360) */

    if (angle_deg >= 360.0f)
    {
        threshold_raw = 0xFFFFu;
    }
    else if (angle_deg <= 0.0f)
    {
        threshold_raw = 0u;
    }
    else
    {
        threshold_raw = (uint16_t)(angle_deg * (65536.0f / 360.0f));
    }

    (void)i2c_write_reg16(KTH5702_REG_WXYTH, threshold_raw);
}

/* --------------------------------------------------------------------------
 * 公开 API：便捷读取（内部走 SingleConvert，会切换为单次模式）
 * 注意：本工程已固定使用连续模式 + ReadData，以下 API 与连续模式冲突，勿混用
 * -------------------------------------------------------------------------- */

/** 【预留】读原始角度值（0~65535），失败返回 0 */
uint16_t KTH5702_ReadRawAngle(void)
{
    KTH5702_AngleData_t data;
    if (NONE_ERROR != KTH5702_SingleConvert(&data))
    {
        return 0u;
    }
    return data.raw_angle;
}

/** 【预留】读角度值（0~360°），失败返回 0.0f */
float KTH5702_ReadAngle(void)
{
    KTH5702_AngleData_t data;
    if (NONE_ERROR != KTH5702_SingleConvert(&data))
    {
        return 0.0f;
    }
    return data.angle_deg;
}

/**
 * 【预留】读状态寄存器低 8 位（bit0=DRDY）
 * 注意：I2C 失败返回 0xFF 哨兵值，判位前需意识到所有位都会被误判为置位
 */
uint8_t KTH5702_GetStatus(void)
{
    uint16_t status_reg;

    if (!i2c_read_reg16(KTH5702_REG_STATUS, &status_reg))
    {
        return 0xFFu;    /* I2C 失败哨兵值 */
    }
    return (uint8_t)(status_reg & 0xFFu);
}
