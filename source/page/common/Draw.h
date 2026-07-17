/*
 * @Author: XinXin
 * @Date: 2024-03-23 11:07:04
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-15 10:40:48
 * @FilePath: \source\page\common\Draw.h
 * @Description:
 */

#ifndef __DRAW_H__
#define __DRAW_H__

#include "common.h"

void Draw_Number_24x24(int x, int y, int num, uint8_t color);
void Draw_buff(int dx, int dy, int width, int height, const uint8_t *buff, uint8_t color);
void Draw_buff_Byte(int dx, int dy, int width, int height, uint8_t data);
void Draw_w8h16_string(int dx, int dy, char *str, uint8_t color);

#endif
