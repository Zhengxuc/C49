/*
 * @Author: XinXin
 * @Date: 2024-03-23 11:06:57
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-15 10:41:07
 * @FilePath: \source\page\common\Draw.c
 * @Description:simpleguiGUI的绘制效率有点低，就有了此文件，用于绘制一些简单的图，
 *              该库函数的致命缺点是绘制的Y坐标只能是8的倍数，这是由LCD的驱动决定的
 */

#include "Draw.h"
#include "segmentCodeScreen.h"
#include "FontAscii_w8h16.h"
#include "Disp.h"
#include "GUIMain.h"
#include "SGUI_Basic.h"

#define MEMCPY 0

void Draw_Number_24x24(int dx, int dy, int num, uint8_t color)
{
#if MEMCPY
    uint32_t y;
    y = dy / 8;
    uint8_t *disp_buff = get_disp_buffer();
    for (int iy = 0; iy < 3; iy++)
    {
        memcpy(&disp_buff[((y + iy) * DISP_HOR_RES) + dx], &segmentCodeScreen_Number[num][(iy * 24)], 24);
    }
#else
    Draw_buff(dx, dy, 24, 24, segmentCodeScreen_Number[num], color);
#endif
}

void Draw_buff(int dx, int dy, int width, int height, const uint8_t *buff, uint8_t color)
{
#if MEMCPY
    uint32_t y;
    y = dy / 8;
    uint8_t *disp_buff = get_disp_buffer();
    for (int iy = 0; iy < (height / 8); iy++)
    {
        memcpy(&disp_buff[((y + iy) * DISP_HOR_RES) + dx], buff + (iy * width), width);
    }
#else
    for (int iy = 0; iy < (height / 8); iy++)
    {
        for (int iw = 0; iw < width; iw++)
        {
            uint8_t byte = buff[((iy * width) + iw)];
            if (color)
                byte = ~byte;
            uint8_t x = dx + iw;
            uint8_t y = dy + (iy * 8);
            for (int ib = 0; ib < 8; ib++)
            {
                gui_get_DeviceIF()->fnSetPixel(x, y + ib, byte & 0x01);
                byte >>= 1;
            }
        }
    }
#endif
}

void Draw_buff_Byte(int dx, int dy, int width, int height, uint8_t data)
{
#if 0
    uint32_t y;
    y = dy / 8;
    uint8_t *disp_buff = get_disp_buffer();
    for (int iy = 0; iy < (height / 8); iy++)
    {
        memset(&disp_buff[((y + iy) * DISP_HOR_RES) + dx], data, width);
    }
#else
    for (int iy = 0; iy < (height / 8); iy++)
    {
        for (int iw = 0; iw < width; iw++)
        {
            uint8_t byte = data;
            uint8_t x = dx + iw;
            uint8_t y = dy + (iy * 8);
            for (int ib = 0; ib < 8; ib++)
            {
                gui_get_DeviceIF()->fnSetPixel(x, y + ib, byte & 0x01);
                byte >>= 1;
            }
        }
    }
#endif
}

void Draw_w8h16_string(int dx, int dy, char *str, uint8_t color)
{
    uint16_t x;
    for (int i = 0; i < strlen(str); i++)
    {
        Draw_buff(dx + x, dy, 8, 16, FontAscii_w8h16[str[i] - ' '], color);
        x += 8;
    }
}
