/*
 * @Author: XinXin
 * @Date: 2024-03-28 20:58:57
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-31 13:36:33
 * @FilePath: \source\driver\mcu\UserDac.h
 * @Description: 
 */

#ifndef __USERDAC_H__
#define __USERDAC_H__

#include "common.h"

void UserDacInit();
void UserDacSetData(uint16_t data);
uint16_t UserDacGetData(void);

#endif
