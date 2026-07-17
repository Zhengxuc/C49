/*
 * @Author: XinXin
 * @Date: 2023-03-25 13:19:26
 * @LastEditors: XinXin
 * @LastEditTime: 2023-03-25 14:40:51
 * @FilePath: \source\driver\src\UsrADC.c
 * @Description:
 */

#include "UserADC.h"
#include "adc.h"
#include "gpio.h"
#include "bgr.h"

volatile uint32_t u32AdcRestult;

static void User_AdcPortInit(void);
static void User_AdcInit(void);
static void User_AdcSglCfg(void);

void UserAdcInit(void)
{
    ///< ADC采样端口初始化
    User_AdcPortInit();
    ///< ADC模块 初始化
    User_AdcInit();
    ///< ADC 单次采样模式 配置
    User_AdcSglCfg();

    while (1)
    {
        if (TRUE == Adc_GetIrqStatus(AdcMskIrqSgl))
        {
            ///< 获取采样值
            u32AdcRestult = Adc_GetSglResult();
            Adc_ClrIrqStatus(AdcMskIrqSgl);
        }
    }
}

///< ADC 采样端口初始化
static void User_AdcPortInit(void)
{
    ///< 开启ADC/BGR GPIO外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
    Gpio_SetAnalogMode(GpioPortC, GpioPin4); // PC4 (AIN14)
}

///< ADC模块 初始化
static void User_AdcInit(void)
{
    stc_adc_cfg_t stcAdcCfg;

    DDL_ZERO_STRUCT(stcAdcCfg);

    ///< 开启ADC/BGR外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdcBgr, TRUE);

    Bgr_BgrEnable(); ///< 开启BGR

    ///< ADC 初始化配置
    stcAdcCfg.enAdcMode = AdcSglMode;                   ///< 采样模式-单次
    stcAdcCfg.enAdcClkDiv = AdcMskClkDiv1;              ///< 采样分频-1
    stcAdcCfg.enAdcSampCycleSel = AdcMskSampCycle12Clk; ///< 采样周期数-12
    stcAdcCfg.enAdcRefVolSel = AdcMskRefVolSelInBgr2p5; ///< 参考电压选择-内部2.5V
    stcAdcCfg.enAdcOpBuf = AdcMskBufDisable;            ///< OP BUF配置-关
    stcAdcCfg.enInRef = AdcMskInRefEnable;              ///< 内部参考电压使能-开
    stcAdcCfg.enAdcAlign = AdcAlignRight;               ///< 转换结果对齐方式-右
    Adc_Init(&stcAdcCfg);
}

///< ADC 单次采样模式 配置
static void User_AdcSglCfg(void)
{
    ///< 配置单次采样通道
    Adc_CfgSglChannel(AdcExInputCH0);
    ///< 启动单次一直采样
    Adc_SGL_Always_Start();
}
