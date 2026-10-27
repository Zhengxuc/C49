/*
 * @Author: XinXin
 * @Date: 2024-05-13 11:23:08
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-13 12:02:52
 * @FilePath: \tools\dispMirror.c
 * @Description:
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISP_HOR_RES 32
#define DISP_VER_RES 32 / 8

static unsigned char disp_buffer[(DISP_VER_RES)][DISP_HOR_RES];

void disp_buff()
{
    printf("\r\n");
    for (int iy = 0; iy < (DISP_VER_RES); iy++)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            for (int iw = 0; iw < DISP_HOR_RES; iw++)
            {
                unsigned char byte = disp_buffer[iy][iw];
                unsigned char bit_value = (byte >> bit) & 0x01;
                if (bit_value)
                {
                    printf("* ");
                }
                else
                {
                    printf("  ");
                }
            }
            printf("\r\n");
        }
    }
}

/*反转数据*/
void mirror_data(unsigned char *data, int length)
{
    unsigned char temp;
    /*数组翻转*/
    for (int i = 0; i < length / 2; i++)
    {
        temp = data[i];
        data[i] = data[length - i - 1];
        data[length - i - 1] = temp;
    }

    /*字节翻转*/
    for (int i = 0; i < length; i++)
    {
        char bit, swapped = 0;
        temp = data[i];
        for (int i = 0; i < 8; i++)
        {
            bit = temp & 1;
            swapped = (swapped << 1) | bit;
            temp >>= 1;
        }
        data[i] = swapped;
    }
}

#ifdef MAIN
int main()
{
    /* 画一个朝左的箭头：横线(行15 列8~28) + 两翼斜线 */
    for (int col = 8; col <= 28; col++)
    {
        disp_buffer[15 / 8][col] |= (1 << (15 % 8));
    }
    for (int i = 1; i <= 4; i++)
    {
        disp_buffer[(15 - i) / 8][8 + i] |= (1 << ((15 - i) % 8)); /* 上翼 */
        disp_buffer[(15 + i) / 8][8 + i] |= (1 << ((15 + i) % 8)); /* 下翼 */
    }

    printf("---- before mirror ----\r\n");
    disp_buff();

    mirror_data(&disp_buffer[0][0], (DISP_HOR_RES * DISP_VER_RES));

    printf("---- after mirror ----\r\n");
    disp_buff();

    getchar();
    return 0;
}
#endif
