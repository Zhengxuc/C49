/*
 * @Author: XinXin
 * @Date: 2023-03-14 10:48:21
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-13 12:06:08
 * @FilePath: \source\driver\extra\Disp.h
 * @Description:
 */

#ifndef DISP_H__
#define DISP_H__

#define JLX19264G740BN 0
#define JLX19296G918BN 1

#define DISP_MODULE JLX19296G918BN

#if DISP_MODULE == JLX19264G740BN
#define DISP_HOR_RES 192
#define DISP_VER_RES 64
#elif DISP_MODULE == JLX19296G918BN
#define DISP_HOR_RES 192
#define DISP_VER_RES 96
#else
#error "not define display module"
#endif

// #define LCD_BACK_LED GpioPortC, GpioPin7
#define LCD_CS GpioPortB, GpioPin12
#define LCD_RST GpioPortC, GpioPin8
#define LCD_CD GpioPortC, GpioPin7

#define SCK GpioPortB, GpioPin13
#define SDA GpioPortB, GpioPin15

void Disp_Init();

inline void Disp_SetPixel(int iX, int iY, char iColor);
inline void Disp_SetPixel_mirror(int iX, int iY, char iColor);
inline void Disp_RefreshDisplay();
inline void Disp_RefreshDisplay_mirror();
inline void Disp_ClearDisplay();
uint8_t *get_disp_buffer();
void display_mirror_data(uint8_t *data, int length);
void lcd_set_contrast_ratio(int ratio);

#endif
