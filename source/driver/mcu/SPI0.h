/*
 * @Descripttion:
 * @version:
 * @Author: Youliangxin
 * @Date: 2023-03-14 10:27:59
 * @LastEditors: sueRimn
 * @LastEditTime: 2023-03-14 10:48:32
 */

#ifndef SPI0_H__
#define SPI0_H__

#define SPI0_SCK_PIN GpioPortA, GpioPin5
#define SPI0_MISO_PIN GpioPortA, GpioPin6
#define SPI0_MOSI_PIN GpioPortA, GpioPin7

void SPI0_Init(void);

#endif
