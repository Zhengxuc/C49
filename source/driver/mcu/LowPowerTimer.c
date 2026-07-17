/*
 * @Author: XinXin
 * @Date: 2023-03-18 14:53:57
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-05 14:51:45
 * @FilePath: \source\driver\src\LowPowerTimer.c
 * @Description:
 */
#include "common.h"
#include "lptim.h"
#include "lpm.h"
#include "gpio.h"
#include "debugLOG.h"
#include "LowPowerTimer.h"

void SystemSleep(void)
{
    Lpm_GotoDeepSleep(TRUE);
    // Lpm_GotoSleep(FALSE);
}

/**
 ******************************************************************************
 ** \brief  LPTIMER中断服务函数
 **
 ** \return 无
 ******************************************************************************/
void LpTim0_IRQHandler(void)
{
    if (TRUE == Lptim_GetItStatus(M0P_LPTIMER0))
    {
        Lptim_ClrItStatus(M0P_LPTIMER0); // 清除LPTimer的中断标志位
        // DEBUG_LOGI("Sleep", "sleep");
    }
}

/**
 ******************************************************************************
 ** \brief  初始化LPTIMER
 **
 ** \return 无
 ******************************************************************************/
void App_LPTimerInit(void)
{
    stc_lptim_cfg_t stcLptCfg;
    DDL_ZERO_STRUCT(stcLptCfg);

    ///< 使能LPTIM0 外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralLpTim0, TRUE);

    stcLptCfg.enGate = LptimGateLow;
    stcLptCfg.enGatep = LptimGatePLow;
    stcLptCfg.enTcksel = LptimRcl;
    stcLptCfg.enTogen = LptimTogEnLow;
    stcLptCfg.enCt = LptimTimerFun;
    stcLptCfg.enMd = LptimMode1; // 工作模式为模式1：无自动重装载16位计数器/定时器
    stcLptCfg.u16Arr = 32767;    // 预装载寄存器值
    Lptim_Init(M0P_LPTIMER0, &stcLptCfg);

    Lptim_ClrItStatus(M0P_LPTIMER0);  // 清除中断标志位
    Lptim_ConfIt(M0P_LPTIMER0, TRUE); // 允许LPTIMER中断
    EnableNvic(LPTIM_0_1_IRQn, IrqLevel3, TRUE);
    Lptim_Cmd(M0P_LPTIMER0, TRUE); // LPT 运行
}