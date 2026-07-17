/*
 * @Author: XinXin
 * @Date: 2023-09-08 16:16:30
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-28 10:35:17
 * @FilePath: \source\driver\inc\led.h
 * @Description:
 */

#ifndef __LED_H__
#define __LED_H__

#include "gpio.h"

#define LED_RED_GPIO GpioPortC, GpioPin13
#define LED_GREEN_GPIO GpioPortC, GpioPin14

#define led_red(a) Gpio_WriteOutputIO(LED_RED_GPIO, !a)
#define led_green(a) Gpio_WriteOutputIO(LED_GREEN_GPIO, !a)

void led_init(void);

#endif
