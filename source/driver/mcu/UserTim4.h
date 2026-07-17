/*
 * @Author: XinXin
 * @Date: 2023-04-08 16:59:12
 * @LastEditors: XinXin
 * @LastEditTime: 2023-04-08 16:59:21
 * @FilePath: \source\driver\inc\UserTim4.h
 * @Description: 
 */


#ifndef __USERTIM4_H__
#define __USERTIM4_H__

#include "common.h"

void UserTime4Init(void);
char Tim4_Get_OverFlag();
void Tim4_ConfigPeriod(uint16_t tim);

#endif
