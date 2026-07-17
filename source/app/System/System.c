/*
 * @Author: XinXin
 * @Date: 2023-12-01 22:59:15
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-23 14:43:38
 * @FilePath: \source\app\System\System.c
 * @Description:
 */

#include "System.h"
#include "ddl.h"
#include "gpio.h"
#include "Disp.h"
#include "IIC.h"
#include "UserDac.h"
#include "spi0.h"
#include "ADX111.h"
#include "KTH5702.h"
#include "common.h"
#include "key.h"
#include "LowPowerTimer.h"
#include "UserTim4.h"
#include "UserTim6.h"
#include "DataPort.h"
#include "led.h"
#include "Save.h"
#include "UserUart1.h"

void system_HAL_Init(void)
{
    SEGGER_RTT_Init();
    UserUart1_init();
    SPI0_Init();
    Disp_Init();
    delay1ms(1000); // 等待LOGO
    led_init();
    UserTime4Init();
    UserTime6Init();
    UserDacInit();
    IIC_Init();
    KEY_Init();
    DEBUG_LOGI("MAIN", "Init ok");
    DEBUG_LOGI("MAIN", "System Click:%dMhz", SystemCoreClock / 1000000);
    RaisingExceptions(ADX111_Init());
    RaisingExceptions(KTH5702_Init());

    //持续感应模式
    KTH5702_SetOversampling(KTH5702_OSR_128);
    KTH5702_SetMeasInterval(1); // 间隔20ms
    KTH5702_SetContinuousMode();

    Save_System_Init();

    stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);     ///< 初始化结构体变量的值为0
    stcGpioCfg.enDir = GpioDirOut;   ///< 端口方向配置->输出
    stcGpioCfg.enOD = GpioOdDisable; ///< 端口开漏输出配置
    stcGpioCfg.enPu = GpioPuDisable; ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable; ///< 端口下拉配置->禁止
    stcGpioCfg.bOutputVal = TRUE;
    Gpio_Init(GpioPortF, GpioPin4, &stcGpioCfg);
}
