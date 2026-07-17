/*
 * @Descripttion:
 * @version:
 * @Author: Youliangxin
 * @Date: 2023-03-24 15:27:35
 * @LastEditors: XinXin
 * @LastEditTime: 2023-12-04 00:48:24
 */
#ifndef __ERROR_H__
#define __ERROR_H__

typedef enum ERROR_CODE
{
    NONE_ERROR = 0,
    TEMP_SENSOR_BUS_ERROR,       // 1:温度传感器总线错误
    ANGLE_SENSOR_BUS_ERROR,      // 2 角度传感器总线错误
    ANGLE_SENSOR_MAGNETIC_ERROR, // 3 角度传感器磁场错误
    ADC_SENSOR_BUS_ERROR,        // 4 ADC芯片总线错误
    GYRO_SENSOR_BUS_ERROR,       // 5 陀螺仪总线错误
    GYRO_SENSOR_MODEL_ERROR,     // 6 陀螺仪型号错误
    EEPROM_BUS_ERROR,            // 7 外部存储总线错误
    EEPROM_WRITE_ERROR,          // 8 外部存储写错误
    EEPROM_READ_ERROR,           // 9 外部存储读错误
    AUTO_ADJ_ANGLE_ERROR,        // 10自动整定角度错误
    FLASH_WRITE_ERROR,           // 11 Flash写错误
    ERROR_INVALID_PARAM,         // 12 参数无效
} ErrorCode;

void RaisingExceptions(ErrorCode error);

#endif
