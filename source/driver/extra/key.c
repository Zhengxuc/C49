/*
 * @Author: XinXin
 * @Date: 2023-09-27 19:12:25
 * @LastEditors: XinXin
 * @LastEditTime: 2023-12-01 22:15:53
 * @FilePath: \source\driver\src\key.c
 * @Description:
 */

#include "key.h"
#include "gpio.h"
#include "UserTim6.h"
#include "common.h"
#include "GUIActions.h"

#define KEY_FN_PIN GpioPortD, GpioPin2
#define KEY_ENT_PIN GpioPortC, GpioPin12
#define KEY_DOWN_PIN GpioPortC, GpioPin11
#define KEY_UP_PIN GpioPortC, GpioPin10

static int key_value = 0;

///< IO端口配置
void KEY_Init(void)
{
    stc_gpio_cfg_t stcGpioCfg;

    DDL_ZERO_STRUCT(stcGpioCfg); ///< 初始化结构体变量的值为0

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE); ///< 开启GPIO时钟门控

    stcGpioCfg.enDir = GpioDirIn;    ///< 端口方向配置->输出
    stcGpioCfg.enOD = GpioOdDisable; ///< 端口开漏输出配置->开漏输出使能
    stcGpioCfg.enPu = GpioPuDisable;  ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable; ///< 端口下拉配置->禁止
    stcGpioCfg.bOutputVal = TRUE;

    Gpio_Init(KEY_UP_PIN, &stcGpioCfg);
    Gpio_Init(KEY_DOWN_PIN, &stcGpioCfg);
    Gpio_Init(KEY_ENT_PIN, &stcGpioCfg);
    Gpio_Init(KEY_FN_PIN, &stcGpioCfg);
}

//默认下拉接地，按下上拉高电平
int KEY_GetState()
{
    static bool record = false;
    static int timeout = 0;
    static int lock = 0;
    if (lock == 0)//空闲状态下
    {
        if (Gpio_GetInputIO(KEY_FN_PIN) == 1)//按键按下
        {
            lock = 1;//非空闲状态
            record = true;
            key_value = KEY_VALUE_ESC;//标记当下key状态
            return KEY_VALUE_ESC;
        }
        else if (Gpio_GetInputIO(KEY_UP_PIN) == 1)
        {
            lock = 1;
            record = true;
					
            key_value = KEY_VALUE_UP;
            return KEY_VALUE_UP;
        }
        else if (Gpio_GetInputIO(KEY_DOWN_PIN) == 1)
        {
            lock = 1;
            record = true;
            key_value = KEY_VALUE_DOWN;
            return KEY_VALUE_DOWN;
        }
        else if (Gpio_GetInputIO(KEY_ENT_PIN) == 1)
        {
            lock = 1;
            record = true;
            key_value = KEY_VALUE_ENTER;
            return KEY_VALUE_ENTER;
        }
    }
    else
    {
        if ((Gpio_GetInputIO(KEY_FN_PIN) == 0) &&
            (Gpio_GetInputIO(KEY_UP_PIN) == 0) &&
            (Gpio_GetInputIO(KEY_DOWN_PIN) == 0) &&
            (Gpio_GetInputIO(KEY_ENT_PIN) == 0))
        {
            lock = 0;
            record = true;
            timeout = 0;
        }
        else
        {
            timeout++;
            if (timeout >= 4)
            {
                timeout = 4;//延时消抖+长按连发（4×50ms 扫描 ≈ 400ms 重复一次）
                if (Gpio_GetInputIO(KEY_FN_PIN) == 1)
                {
                    record = true;
                    key_value = KEY_VALUE_ESC;
                    return KEY_VALUE_ESC;
                }
                else if (Gpio_GetInputIO(KEY_UP_PIN) == 1)
                {
                    record = true;
                    key_value = KEY_VALUE_UP;
                    return KEY_VALUE_UP;
                }
                else if (Gpio_GetInputIO(KEY_DOWN_PIN) == 1)
                {
                    record = true;
                    key_value = KEY_VALUE_DOWN;
                    return KEY_VALUE_DOWN;
                }
                else if (Gpio_GetInputIO(KEY_ENT_PIN) == 1)
                {
                    record = true;
                    key_value = KEY_VALUE_ENTER;
                    return KEY_VALUE_ENTER;
                }
            }
            else
            {
                record = false;
            }
        }
    }
    if (record == true)//按键标志位确定
    {
        record = false;
        key_value = KEY_VALUE_NONE;//案件标志
        return KEY_VALUE_NONE;
    }
    return -1;//空 没有按键按下
}

int KEY_GetValue()
{
    return key_value;
}
