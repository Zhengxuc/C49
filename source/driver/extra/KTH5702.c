/*
 * KTH5702 2D 霍尔角度传感器驱动（I2C，HC32 I2C 主机库）
 */

#include "KTH5702.h"
#include "common.h"
#include "IIC.h"

static const char *TAG = "KTH5702";

#define KTH5702_I2Cx                M0P_I2C0
#define KTH5702_REG_CMD_SHIFT       2u

/* 总线寄存器访问：先发 SLA+W，再发命令字节与后续数据（与手册 I2C 帧一致） */
#define KTH5702_BUS_READ_REG        0x50u /* 读寄存器 */
#define KTH5702_BUS_WRITE_REG       0x60u /* 写寄存器 */

static uint8_t g_meas_interval = 0;
static KTH5702_Osr_t g_osr = KTH5702_OSR_128;
static KTH5702_Filter_t g_filter = KTH5702_FILTER_0;

static uint8_t reg_cmd_byte(uint8_t reg_index)
{
    return (uint8_t)(reg_index << KTH5702_REG_CMD_SHIFT);
}

static bool i2c_write_reg16(uint8_t reg_index, uint16_t value)
{
    uint8_t wr[4];
    uint8_t st;

    /* 单帧：START, SLA+W, 0x60, D15..8, D7..0, reg<<2, Sr, SLA+R, 状态, STOP */
    wr[0] = KTH5702_BUS_WRITE_REG;
    wr[1] = (uint8_t)((value >> 8) & 0xFFu);
    wr[2] = (uint8_t)(value & 0xFFu);
    wr[3] = reg_cmd_byte(reg_index);
    return (Ok == I2C_MasterWriteThenRead(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, wr, 4u, &st, 1u));
}

static bool i2c_read_reg16(uint8_t reg_index, uint16_t *value)
{
    uint8_t wr[2];
    uint8_t rx[3];

    if (value == NULL)
    {
        return false;
    }
    /* 单帧：START, SLA+W, 0x50, reg<<2, Sr, SLA+R, 状态+数据(2), STOP */
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

    /* 回读帧：状态 1B + A(平面角) 2B + B(平面磁强对应量) 2B，共 5B */
    if (Ok != I2C_MasterWriteThenRead(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, &cmdb, 1u, rx, 5u))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }

    data->status = rx[0];
    raw_angle = (uint16_t)(((uint16_t)rx[1] << 8) | rx[2]);
    raw_b = (uint16_t)(((uint16_t)rx[3] << 8) | rx[4]);

    data->raw_angle = raw_angle;
    data->angle_deg = (float)raw_angle * (360.0f / 65536.0f);
    data->raw_mag_plane = raw_b;

    return NONE_ERROR;
}

static ErrorCode send_cmd_read_status(uint8_t cmd, uint8_t *status_out)
{
    uint8_t st;

    /* 单帧：START, SLA+W, 命令, Sr, SLA+R, 状态, STOP */
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

ErrorCode KTH5702_Init(void)
{
    uint16_t regv;
    ErrorCode ec;

    delay1ms(5);

    if (!i2c_read_reg16(KTH5702_REG_STATUS, &regv))
    {
        DEBUG_LOGE(TAG, "probe STATUS failed (check 7-bit I2C addr / wiring)");
        return ANGLE_SENSOR_BUS_ERROR;
    }

    if (!i2c_write_reg16(KTH5702_REG_CONFIG1, (uint16_t)(((uint16_t)g_osr << 9) | (uint16_t)g_filter)))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
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

ErrorCode KTH5702_SoftReset(void)
{
    (void)KTH5702_SetIdleMode();
    /* 复位：仅 SLA+W 后写 0xF0，无读状态阶段 */
    if (Ok != I2C_MasterWriteData(KTH5702_I2Cx, (uint8_t)KTH5702_ADDRESS, KTH5702_CMD_RESET, NULL, 0))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    delay1ms(5);
    return NONE_ERROR;
}

ErrorCode KTH5702_SetContinuousMode(void)
{
    uint8_t status;

    if (!i2c_write_reg16(KTH5702_REG_CONFIG2, (uint16_t)(((uint16_t)KTH5702_MEAS_BA << 6) | (uint16_t)g_meas_interval)))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    if (NONE_ERROR != send_cmd_read_status(KTH5702_CMD_CONTINUOUS, &status))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    if ((status & KTH5702_STATUS_CONTINUOUS) == 0u)
    {
        DEBUG_LOGW(TAG, "continuous: status 0x%02X (no bit7), I2C ok", status);
    }
    return NONE_ERROR;
}

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

ErrorCode KTH5702_SetIdleMode(void)
{
    uint8_t status;

    if (NONE_ERROR != send_cmd_read_status(KTH5702_CMD_IDLE, &status))
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    return NONE_ERROR;
}

ErrorCode KTH5702_SingleConvert(KTH5702_AngleData_t *data)
{
    if (NONE_ERROR != KTH5702_SetSingleMode())
    {
        return ANGLE_SENSOR_BUS_ERROR;
    }
    delay1ms(1);
    return KTH5702_ReadData(data);
}

ErrorCode KTH5702_ReadData(KTH5702_AngleData_t *data)
{
    uint8_t status = 0;
    uint8_t retry;

    if (data == NULL)
    {
        return ERROR_INVALID_PARAM;
    }
#if 0
    for (retry = 0; retry < 20u; retry++)
    {
        status = KTH5702_GetStatus();
        if ((status & KTH5702_STATUS_DRDY) != 0u)
        {
            break;
        }
        delay1ms(1);
    }
    if ((status & KTH5702_STATUS_DRDY) == 0u)
    {
        DEBUG_LOGE(TAG, "DRDY timeout, status 0x%02X", status);
        return ANGLE_SENSOR_BUS_ERROR;
    }
#endif
    return kth5702_read_data_frame(data);
}

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

void KTH5702_SetWakeupThreshold(float angle_deg)
{
    uint16_t threshold_raw;

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

uint16_t KTH5702_ReadRawAngle(void)
{
    KTH5702_AngleData_t data;

    if (NONE_ERROR != KTH5702_SingleConvert(&data))
    {
        return 0u;
    }
    return data.raw_angle;
}

float KTH5702_ReadAngle(void)
{
    KTH5702_AngleData_t data;

    if (NONE_ERROR != KTH5702_SingleConvert(&data))
    {
        return 0.0f;
    }
    return data.angle_deg;
}

uint8_t KTH5702_GetStatus(void)
{
    uint16_t status_reg;

    if (!i2c_read_reg16(KTH5702_REG_STATUS, &status_reg))
    {
        return 0xFFu;
    }
    return (uint8_t)(status_reg & 0xFFu);
}
