
#ifndef __KEY_H__
#define __KEY_H__

#include "gpio.h"

/* 按键引脚（低电平松开 / 高电平按下），按键扫描与 GUI 手动直读共用 */
#define KEY_FN_PIN   GpioPortD, GpioPin2
#define KEY_ENT_PIN  GpioPortC, GpioPin12
#define KEY_DOWN_PIN GpioPortC, GpioPin11
#define KEY_UP_PIN   GpioPortC, GpioPin10

void KEY_Init(void);
int KEY_GetState();
int KEY_GetValue();

#endif
