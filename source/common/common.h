/*
 * @Author: XinXin
 * @Date: 2023-09-29 11:29:06
 * @LastEditors: XinXin
 * @LastEditTime: 2024-04-25 15:17:35
 * @FilePath: \source\common\common.h
 * @Description:
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include "debugLOG.h"
#include "error.h"
#include "ddl.h"
#include <stdbool.h>

#define CONTROL_PERIOD 20 // ms
/* 控制周期（秒），PID 积分/微分含 Ts，改周期无需重调参数 */
#define CONTROL_PERIOD_S (CONTROL_PERIOD / 1000.0f)

#define scope(value, min, max) (value < min ? min : (value > max ? max : value))

/*取绝对值*/
int32_t to_abs(int32_t num);

#endif
