/*
 * @Author: XinXin
 * @Date: 2023-04-08 16:59:02
 * @LastEditors: XinXin
 * @LastEditTime: 2023-12-01 21:20:53
 * @FilePath: \source\driver\src\UserTim4.c
 * @Description:
 */

#include "UserTim4.h"
#include "adt.h"
#include "debugLOG.h"

static uint16_t u16ADTIM4CntValue;
static uint16_t u16Capture;
static volatile uint16_t flag = 0;

void Tim4_IRQHandler(void)
{
    // 上溢中断
    if (TRUE == Adt_GetIrqFlag(M0P_ADTIM4, AdtOVFIrq))
    {
        flag = 1;
        u16ADTIM4CntValue = Adt_GetCount(M0P_ADTIM4);
        Adt_ClearIrqFlag(M0P_ADTIM4, AdtOVFIrq);
    }
}

char Tim4_Get_OverFlag()
{
    // 上溢中断
    if (flag != 0)
    {
        flag = 0;
        return 1;
    }
    return 0;
}

void UserTime4Init(void)
{
    stc_adt_basecnt_cfg_t stcAdtBaseCntCfg;

    DDL_ZERO_STRUCT(stcAdtBaseCntCfg);
    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdvTim, TRUE); // ADT外设时钟使能

    stcAdtBaseCntCfg.enCntMode = AdtTriangleModeA;
    stcAdtBaseCntCfg.enCntDir = AdtCntUp;
    stcAdtBaseCntCfg.enCntClkDiv = AdtClkPClk0Div64;
    Adt_Init(M0P_ADTIM4, &stcAdtBaseCntCfg); // ADT载波、计数模式、时钟配置

    Adt_SetPeriod(M0P_ADTIM4, 0xffff); // 周期设置
    Adt_ClearAllIrqFlag(M0P_ADTIM4);

    Adt_CfgIrq(M0P_ADTIM4, AdtOVFIrq, TRUE); // 上溢中断配置
    EnableNvic(ADTIM4_IRQn, IrqLevel3, TRUE);

    Tim4_ConfigPeriod(0);
}

/**
 * @brief  设置定时器的定时时间
 * @param  tim : 时间 单位ms
 * @retval NoNe
 */
void Tim4_ConfigPeriod(uint16_t tim)
{
    uint16_t u16Period;

    Adt_StopCount(M0P_ADTIM4);
    Adt_ClearCount(M0P_ADTIM4);
    Adt_ClearAllIrqFlag(M0P_ADTIM4);
    if (tim > 0)
    {
        u16Period = (uint16_t)(((float)SystemCoreClock / 2.0f / 1000.0f / 64.0f) * ((float)tim));
        Adt_SetPeriod(M0P_ADTIM4, u16Period - 1); // 周期设置
        Adt_StartCount(M0P_ADTIM4);
    }
}
