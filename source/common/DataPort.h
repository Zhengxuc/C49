/*
 * @Author: XinXin
 * @Date: 2023-10-21 11:04:36
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-25 13:16:42
 * @FilePath: \source\common\DataPort.h
 * @Description:
 */

#ifndef __DATAPORT_H__
#define __DATAPORT_H__

#include "common.h"

/*限定a的最大值和最小值*/
#define RANGE(min, max, a) (a > max ? max : (a < min ? min : a))

enum DataListIndex
{
    /*
     *DATA_F_XXXX float浮点数类型
     *DATA_I_XXXX int32_t有符号整数类型
     *DATA_B_XXXX bool布尔值
     *DATA_S_XXXX 字符串 char*
     */
    DATA_F_TEMP = 0,       // 温度
    DATA_F_HUMI,           // 湿度
    DATA_F_ANGLE,          // 角度
    DATA_F_INPUT_CURRENT,  // 输入电流
    DATA_I_OUTPUT_CURRENT, // 输出电流
    DATA_F_PRESSURE_CH1,   // 气压通道1
    DATA_F_PRESSURE_CH2,   // 气压通道2
    DATA_F_PRESSURE_CH3,   // 气压通道3
    DATA_F_PRESSURE_CH4,   // 气压通道4

    DATA_I_AUTO_ADJ_ALL_STEP,      // 自整定步骤
    DATA_I_AUTO_ADJ_CURRENT_STEP,  // 当前步骤
    DATA_I_AUTO_ADJ_PROGRESS,      // 当前进度
    DATA_S_AUTO_ADJ_TITLE,         // 标题
    DATA_S_AUTO_ADJ_DESC,          // 描述
    DATA_F_AUTO_ADJ_DESC_VALUE,    // 描述值
    DATA_S_AUTO_ADJ_CONTENT,       // 内容
    DATA_F_AUTO_ADJ_CONTENT_VALUE, // 内容值

    DATA_I_GUI_KEY_VALUE, // 界面按键值

    DATA_I_IP_CURRENT, // IP电流值

    /*末尾保留最大值，用来声明数组长度*/
    DATA_MAX
};
bool Port_Set(enum DataListIndex index, void *dat);
bool Port_Get(void **data, enum DataListIndex index);
void Port_SetFloat(enum DataListIndex index, float32_t dat);
float32_t Port_GetFloat(enum DataListIndex index);
void Port_SetInt(enum DataListIndex index, int32_t dat);
int32_t Port_GetInt(enum DataListIndex index);
void Port_SetStr(enum DataListIndex index, const char *data);
const char *Port_GetStr(enum DataListIndex index);

#endif
