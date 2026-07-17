/*
 * @Author: XinXin
 * @Date: 2024-03-20 16:08:16
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-23 13:59:09
 * @FilePath: \source\driver\mcu\UserUart1.h
 * @Description:
 */

#ifndef __USERUART1_H__
#define __USERUART1_H__

void UserUart1_init(void);
void UserUart1_send_data(char *pData, int u16Len);
void UserUart1_log(char *format, ...);
void UserUart1_send_report(unsigned char fun, unsigned char *data, unsigned char len);
void UserUart1_send_reportV2(unsigned char fun, unsigned char *data, unsigned char len);

#endif
