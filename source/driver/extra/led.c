/*
 * @Author: XinXin
 * @Date: 2023-09-08 16:16:30
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-13 12:37:35
 * @FilePath: \source\driver\src\led.c
 * @Description:
 */

#include "led.h"
#include "common.h"

void led_init(void)
{
    stc_gpio_cfg_t stcGpioCfg;

    DDL_ZERO_STRUCT(stcGpioCfg); ///< 初始化结构体变量的值为0

    stcGpioCfg.enDir = GpioDirOut;   ///< 端口方向配置->输出
    stcGpioCfg.enOD = GpioOdDisable; ///< 端口开漏输出配置
    stcGpioCfg.enPu = GpioPuDisable; ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable; ///< 端口下拉配置->禁止
    stcGpioCfg.bOutputVal = TRUE;

    Gpio_Init(LED_GREEN_GPIO, &stcGpioCfg);
    Gpio_Init(LED_RED_GPIO, &stcGpioCfg);
}
