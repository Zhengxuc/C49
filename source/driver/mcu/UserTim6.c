/*
 * @Author: XinXin
 * @Date: 2023-04-08 16:59:02
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-22 23:22:33
 * @FilePath: \source\driver\mcu\UserTim6.c
 * @Description:
 */

#include "UserTim6.h"
#include "debugLOG.h"
#include "gpio.h"

#define SYSTEM_STEP_NUMBER 250 // 100ms中断一次,16M时钟下65535/250=262ms

// 系统时钟 定时器64分频 转化为毫秒
#define TIMER_TO_MS (SystemCoreClock / 64 / 1000) // 把16位定时器的值转换为ms

static volatile uint32_t timeCount = 0;
static void Tim6_ConfigPeriod(uint16_t tim);

void Tim6_IRQHandler(void)
{
// 上溢中断
#if 0
    if (TRUE == Adt_GetIrqFlag(M0P_ADTIM6, AdtOVFIrq))
#endif
    if (TRUE == (((M0P_ADTIM6->IFR) >> AdtOVFIrq) & 0x1))
    {
        Adt_ClearIrqFlag(M0P_ADTIM6, AdtOVFIrq);
        timeCount ++;
    }
}

void UserTime6Init(void)
{
    stc_adt_basecnt_cfg_t stcAdtBaseCntCfg;

    DDL_ZERO_STRUCT(stcAdtBaseCntCfg);
    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdvTim, TRUE); // ADT外设时钟使能

    stcAdtBaseCntCfg.enCntMode = AdtSawtoothMode;
    stcAdtBaseCntCfg.enCntDir = AdtCntUp;
    stcAdtBaseCntCfg.enCntClkDiv = AdtClkPClk0Div64;
    Adt_Init(M0P_ADTIM6, &stcAdtBaseCntCfg); // ADT载波、计数模式、时钟配置

    Adt_SetPeriod(M0P_ADTIM6, 1); // 周期设置
    Adt_ClearAllIrqFlag(M0P_ADTIM6);

    Adt_CfgIrq(M0P_ADTIM6, AdtOVFIrq, TRUE); // 上溢中断配置
    EnableNvic(ADTIM6_IRQn, IrqLevel3, TRUE);

    Tim6_ConfigPeriod(SYSTEM_STEP_NUMBER);

/*测试用*/
#if 0
    stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg); ///< 初始化结构体变量的值为0
    stcGpioCfg.enDir = GpioDirOut;   ///< 端口方向配置->输出
    stcGpioCfg.enOD = GpioOdDisable; ///< 端口开漏输出配置
    stcGpioCfg.enPu = GpioPuDisable; ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable; ///< 端口下拉配置->禁止
    stcGpioCfg.bOutputVal = TRUE;
    Gpio_Init(GpioPortF, GpioPin4, &stcGpioCfg);
    static int i = 0;
    Gpio_WriteOutputIO(GpioPortF, GpioPin4, i % 2);
    i++;
#endif
}

/**
 * @brief  设置定时器的定时时间
 * @param  tim : 时间 单位ms
 * @retval NoNe
 */
static void Tim6_ConfigPeriod(uint16_t tim)
{
    uint16_t u16Period;

    Adt_StopCount(M0P_ADTIM6);
    Adt_ClearCount(M0P_ADTIM6);
    Adt_ClearAllIrqFlag(M0P_ADTIM6);
    if (tim > 0)
    {
        u16Period = TIMER_TO_MS * tim - 1;
        Adt_SetPeriod(M0P_ADTIM6, u16Period); // 周期设置
        Adt_StartCount(M0P_ADTIM6);
    }
}

uint32_t get_SystemTime(void)
{
    uint16_t u16Period = M0P_ADTIM6->CNTER_f.CNT;
    return (timeCount * (uint32_t)SYSTEM_STEP_NUMBER) + (u16Period / TIMER_TO_MS);
}

bool get_delay(uint32_t *clock, uint32_t delay)
{
    if ((get_SystemTime()) >= (*clock + delay))
    {
        *clock = get_SystemTime();
        return true;
    }
    return false;
}
