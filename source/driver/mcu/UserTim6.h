/*
 * @Author: XinXin
 * @Date: 2023-04-08 16:59:12
 * @LastEditors: XinXin
 * @LastEditTime: 2023-12-01 21:33:29
 * @FilePath: \source\driver\inc\UserTim6.h
 * @Description:
 */

#ifndef __USERTIM6_H__
#define __USERTIM6_H__

#include "common.h"
#include "adt.h"
#include "ddl.h"

void UserTime6Init(void);

/*获取系统启动时间：单位ms*/
uint32_t get_SystemTime(void);
bool get_delay(uint32_t *clock, uint32_t delay);

#endif
