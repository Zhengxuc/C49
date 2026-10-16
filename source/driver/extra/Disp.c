/*
 * @Descripttion:
 * @version:
 * @Author: Youliangxin
 * @Date: 2023-03-14 09:48:13
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-13 23:58:55
 */
#include <string.h>
#include "common.h"
#include "spi.h"
#include "gpio.h"
#include "reset.h"
#include "Disp.h"
#include "bmp.h"

static uint8_t disp_buffer[(DISP_VER_RES / 8)][DISP_HOR_RES];

static void Disp_GPIO_Init();
static void initial_lcd();
static void initial_lcd_19296G918();
static void display_graphic_192x64(const uint8_t *dp);
static void display_graphic_19296G918(const uint8_t *dp);
static void display_graphic_19296G918_mirror(uint8_t const *dp);
static void transfer_command(uint8_t data1);
static void transfer_data(uint8_t data1);
static char mirror = 1;

uint8_t *get_disp_buffer()
{
    return (uint8_t *)(&disp_buffer[0][0]);
}

inline void Disp_SetPixel(int iX, int iY, char iColor)
{
    if (iColor)
        disp_buffer[iY / 8][iX] |= (0x01 << (iY % 8));
    else
        disp_buffer[iY / 8][iX] &= ~((0x01) << (iY % 8));
}

inline void Disp_SetPixel_mirror(int iX, int iY, char iColor)
{
    if (iColor)
        disp_buffer[iY / 8][DISP_HOR_RES - iX - 1] |= (0x80 >> (iY % 8));
    else
        disp_buffer[iY / 8][DISP_HOR_RES - iX - 1] &= ~(0x80 >> (iY % 8));
}

#if 0
char Disp_GetPixel(int iX, int iY, uint8_t iColor)
{
    return ((disp_buffer[iY / 8][iX] >> (iY % 8)) & 0x01);
}
#endif

inline void Disp_ClearDisplay()
{
    memset(disp_buffer, 0, sizeof(disp_buffer));//讲disp_buffer清空
}

inline void Disp_RefreshDisplay()
{
#if DISP_MODULE == JLX19264G740BN
    display_graphic_192x64((const uint8_t *)disp_buffer);
#elif DISP_MODULE == JLX19296G918BN
    display_graphic_19296G918((const uint8_t *)disp_buffer);
#endif
}

inline void Disp_RefreshDisplay_mirror()
{
    display_graphic_19296G918_mirror((uint8_t const *)disp_buffer);
}

void Disp_Init()
{
    Disp_GPIO_Init();
    memset(disp_buffer, 0, sizeof(disp_buffer));
#if DISP_MODULE == JLX19264G740BN
    initial_lcd();
#elif DISP_MODULE == JLX19296G918BN
    initial_lcd_19296G918();
#endif

    Disp_ClearDisplay();

    // memcpy(&disp_buffer[3][0], gImage_Logo, sizeof(gImage_Logo));
    Disp_RefreshDisplay();
}

static void Disp_GPIO_Init()
{
    stc_gpio_cfg_t GpioInitStruct;
    DDL_ZERO_STRUCT(GpioInitStruct);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

    GpioInitStruct.enDrv = GpioDrvH;
    GpioInitStruct.enDir = GpioDirOut;
    GpioInitStruct.bOutputVal = FALSE;
    Gpio_Init(LCD_CS, &GpioInitStruct);
    Gpio_Init(LCD_RST, &GpioInitStruct);
    Gpio_Init(LCD_CD, &GpioInitStruct);

#ifdef SOFT_SPI
    Gpio_Init(SCK, &GpioInitStruct);
    Gpio_WriteOutputIO(SCK, FALSE);
    Gpio_Init(SDA, &GpioInitStruct);
    Gpio_WriteOutputIO(SDA, FALSE);
#endif
}

// LCD 模块初始化
static void initial_lcd()
{
    Gpio_WriteOutputIO(LCD_RST, 0); // 低电平复位
    delay1ms(2);
    Gpio_WriteOutputIO(LCD_RST, 1); // 复位完毕
    delay1ms(2);
    transfer_command(0xe2); // 软复位
    delay1ms(2);
    transfer_command(0x2f); // 打开内部升压
    delay1ms(2);
    transfer_command(0xa0);        //
    transfer_command(0x81);        //
    transfer_command(0x65);        // 微调对比度的值，可设置范围 0x00～0xFF
    transfer_command(0xe8 | 0x03); // 1/9 偏压比（bias）
    transfer_command(0xc4);        // 行列扫描顺序：从上到下
    transfer_command(0xaf);        // 开显示
}

static void initial_lcd_19296G918()
{
    Gpio_WriteOutputIO(LCD_RST, 0); // 低电平复位
    delay1ms(2);
    Gpio_WriteOutputIO(LCD_RST, 1); // 复位完毕
    delay1ms(2);

    transfer_command(0x30); // EXT1=0，EXT0=0,表示选择了“扩展指令表 1”
    transfer_command(0x94); // 退出睡眠
    transfer_command(0x31); // EXT=1
    transfer_command(0xD7); // Autoread disable
    transfer_data(0X9F);    //
    transfer_command(0x32); // LCD 偏压比设置
    transfer_data(0x00);    // 振荡频率的调整
    transfer_data(0x01);    // 升压电容器的频率->6KHz
    transfer_data(0x03);    // Bias=1/11
    transfer_command(0x31); // EXT=1
    transfer_command(0xf0); // 此指令比较重要,不加此指令升压会慢 0.5s
    transfer_data(0x0f);
    transfer_data(0x0f);
    transfer_data(0x0f);
    transfer_data(0x0f);
    transfer_command(0x20); // 灰度设置
    transfer_data(0x01);
    transfer_data(0x03);
    transfer_data(0x05);
    transfer_data(0x07);
    transfer_data(0x09);
    transfer_data(0x0b);
    transfer_data(0x0d);
    transfer_data(0x10);
    transfer_data(0x11);
    transfer_data(0x13);
    transfer_data(0x15);
    transfer_data(0x17);
    transfer_data(0x19);
    transfer_data(0x1b);
    transfer_data(0x1d);
    transfer_data(0x1f);
    transfer_command(0x30); // EXT1=0，EXT0=0,表示选择了“扩展指令表 1”
    transfer_command(0x75); // 页地址设置
    transfer_data(0X00);    // 起始页地址：YS=0X00
    transfer_data(0X60);    // 结束页地址：YE=0x1F 每 4 行为一页，第 0～3 行为第 0 页，第 124～127 行为第 31 页（31=0x1f）
    transfer_command(0x15); // 列地址设置
    transfer_data(0X00);    // 起始列地址：XS=0
    transfer_data(0Xc0);    // 结束列地址：XE=256（0xff）
    transfer_command(0xBC); // 数据扫描方向
    transfer_data(0x00);    // MX.MY=Normal
    transfer_data(0xA6);
    transfer_command(0x0c); // 数据格式选择,0x0C 是低位在前 D0-D7,0x08 是高位在前 D7-D0
    transfer_command(0xCA); // 显示控制
    transfer_data(0X00);    // 设置 CL 驱动频率：CLD=0
    transfer_data(0X5F);    // 占空比：Duty=128
    transfer_data(0X20);    // N 行反显：Nline=off
    transfer_command(0xF0); // 显示模式
    transfer_data(0X10);    // 如果设为 0x11：表示选择 4 灰度级模式，如果设为 0x10:表示选择黑白模式
    transfer_command(0x81); // 设置对比度， “0x81”不可改动，紧跟着的 2 个数据是可改的，但“先微调后粗调”这个顺序别乱了
    transfer_data(18);      // 对比度微调，可调范围 0x00～0x3f，共 64 级
    transfer_data(0x03);    // 对比度粗调，可调范围 0x00～0x07，共 8 级

    transfer_command(0x20); // Power control
    transfer_data(0x0B);    // D0=regulator ; D1=follower ; D3=booste, on:1 off:0
    delay100us(1);
    transfer_command(0xAF); // 打开显示
}

void lcd_set_contrast_ratio(int ratio)
{
    ratio = scope(ratio, 1, 30);
    ratio += 190;
    transfer_command(0x81);      // 设置对比度
    transfer_data(ratio % 64);   // 微调，可调范围 0x00～0x3f，共 64 级
    transfer_data((ratio / 64)); // 粗调，可调范围 0x00～0x07，共 8 级
    DEBUG_LOGD("DISP", "设置对比度:%d,%d,%d", ratio, ratio % 64, (ratio / 64));
}

// 写指令到 LCD 模块
void transfer_command(uint8_t data1)
{
    char i;
    Gpio_WriteOutputIO(LCD_CS, 0);
    Gpio_WriteOutputIO(LCD_CD, 0);
#ifdef SOFT_SPI
    for (i = 0; i < 8; i++)
    {
        Gpio_WriteOutputIO(SCK, 0);
        if (data1 & 0x80)
            Gpio_WriteOutputIO(SDA, 1);
        else
            Gpio_WriteOutputIO(SDA, 0);
        Gpio_WriteOutputIO(SCK, 1);
        data1 = data1 << 1;
    }
#else
    Spi_SetCS(M0P_SPI1, FALSE);
    Spi_RWByte(M0P_SPI1, data1);
    Spi_SetCS(M0P_SPI1, TRUE);
#endif
    Gpio_WriteOutputIO(LCD_CS, 1);
}

// 写数据到 LCD 模块
void transfer_data(uint8_t data1)
{
    char i;
    Gpio_WriteOutputIO(LCD_CS, 0);
    Gpio_WriteOutputIO(LCD_CD, 1);
#ifdef SOFT_SPI
    for (i = 0; i < 8; i++)
    {
        Gpio_WriteOutputIO(SCK, 0);
        if (data1 & 0x80)
            Gpio_WriteOutputIO(SDA, 1);
        else
            Gpio_WriteOutputIO(SDA, 0);
        Gpio_WriteOutputIO(SCK, 1);
        data1 = data1 << 1;
    }
#else
    Spi_SetCS(M0P_SPI1, FALSE);
    Spi_RWByte(M0P_SPI1, data1);
    Spi_SetCS(M0P_SPI1, TRUE);
#endif
    Gpio_WriteOutputIO(LCD_CS, 1);
}

void lcd_address(uint8_t page, uint8_t column)
{
    transfer_command(0xb0 + page);                   // 每页是8行。一个画面的64行被分成8个页。
    transfer_command(((column >> 4) & 0x0f) | 0x10); // 设置列地址的高 4 位
    transfer_command(column & 0x0f);                 // 设置列地址的低 4 位
}

/*写 LCD 行列地址：X 为起始的列地址，Y 为起始的行地址，x_total,y_total 分别为列地址及行地址的起点到终点的差值 */
void lcd_address_19296G918(int x, int y, int x_total, int y_total)
{
    transfer_command(0x15); // Set Column Address
    transfer_data(x);
    transfer_data(x + x_total);
    transfer_command(0x75); // Set Page Address
    transfer_data(y);
    transfer_data(y + y_total);
    transfer_command(0x30);
    transfer_command(0x5c);
}

/*显示 196*96 点阵的图像*/
static void display_graphic_19296G918(const uint8_t *dp)
{
    int i, j;
    lcd_address_19296G918(0, 0, 191, 11);
    Gpio_WriteOutputIO(LCD_CS, 0);
    Gpio_WriteOutputIO(LCD_CD, 1);

    Spi_SetCS(M0P_SPI1, FALSE);
    for (i = 0; i < 12; i++)
    {
#if SOFT_SPI
        for (j = 0; j < 192; j++)
        {
            transfer_data(*dp);
            dp++;
        }
#else
        Spi_SendBuf(M0P_SPI1, (uint8_t *)dp, 192);
        dp += 192;
#endif
    }
    Spi_SetCS(M0P_SPI1, TRUE);

    Gpio_WriteOutputIO(LCD_CS, 1);
}

/*显示 196*96 点阵的图像*/
static void display_graphic_19296G918_mirror(uint8_t const *dp)
{
    int i, j;
    uint8_t const *pix = dp + ((DISP_VER_RES / 8) * DISP_HOR_RES) - 192;
    lcd_address_19296G918(0, 0, 191, 11);
    Gpio_WriteOutputIO(LCD_CS, 0);
    Gpio_WriteOutputIO(LCD_CD, 1);

    Spi_SetCS(M0P_SPI1, FALSE);
    for (i = 0; i < 12; i++)
    {
        Spi_SendBuf(M0P_SPI1, (uint8_t *)pix, 192);
        pix -= 192;
    }
    Spi_SetCS(M0P_SPI1, TRUE);

    Gpio_WriteOutputIO(LCD_CS, 1);
}

static void display_graphic_192x64(const uint8_t *dp)
{
    uint8_t i, j;
    uint8_t *p;
    p = (uint8_t *)dp;
    for (i = 0; i < 8; i++)
    {
        lcd_address(i, 0);
#if SOFT_SPI
        for (j = 0; j < 192; j++)
        {
            transfer_data(*dp);
            dp++;
        }
#else
        Gpio_WriteOutputIO(LCD_CS, 0);
        Gpio_WriteOutputIO(LCD_CD, 1);
        Spi_SetCS(M0P_SPI1, FALSE);
        Spi_SendBuf(M0P_SPI1, p, 192);
        Spi_SetCS(M0P_SPI1, TRUE);
        Gpio_WriteOutputIO(LCD_CS, 1);
        p += 192;
#endif
    }
}

/*反转数据*/
void display_mirror_data(uint8_t *data, int length)
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
