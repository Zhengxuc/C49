/*
 * @Author: XinXin
 * @Date: 2023-03-16 19:25:08
 * @LastEditors: XinXin
 * @LastEditTime: 2024-12-23 17:42:37
 * @FilePath: \source\main.c
 * @Description:
 */
#include "ddl.h"
#include "System.h"
#include "GUIMain.h"
#include "gpio.h"
#include "UserDac.h"
#include "SystemServer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "SEGGER_RTT.h"
#include "dataport.h"
#include "KTH5702.h"

static void App_SysClkInit(void);

static void DispKeyTask(void *pvParameters)
{

    GUIMainInit();
    SystemServer_Init();
    appManage_switchTask(APP_TASK_AUTO_CONTROL);
    while (1)
    {
        SystemServer();
        GUIMainProcess();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void ControlTask(void *pvParameters)
{
    static int i = 0;
    while (1)
    {
        KTH5702_AngleData_t data;
        if (NONE_ERROR == KTH5702_ReadData(&data))
        {
            /*data.raw_mag_plane 表示磁感应强度，越大表示磁场越强*/
            /*data.angle_deg 表示磁场角度，0-360度*/
            Port_SetFloat(DATA_F_ANGLE, data.angle_deg);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int32_t main(void)
{
    App_SysClkInit();
    SEGGER_RTT_Init();
    system_HAL_Init();

    xTaskCreate(ControlTask, "Control", 512, NULL, 1, NULL);
    xTaskCreate(DispKeyTask, "DispKey", 512, NULL, 2, NULL);

    vTaskStartScheduler();
}

static void App_SysClkInit(void)
{
    stc_sysctrl_clk_cfg_t stcClkCfg;

    /*
      38.4K校准值地址:0x00100C20ul-0x00100C21
    32.768K校准值地址:0x00100C22ul-0x00100C23
    */
    M0P_SYSCTRL->RCL_CR_f.TRIM = (*((volatile uint16_t *)(0x00100C22ul)));

    /*
       24M校准值地址:0x00100C00ul-0x00100C01
    22.12M校准值地址:0x00100C02ul-0x00100C03
       16M校准值地址:0x00100C04ul-0x00100C05
        8M校准值地址:0x00100C06ul-0x00100C07
        4M校准值地址:0x00100C08ul-0x00100C09
    */
    M0P_SYSCTRL->RCH_CR_f.TRIM = (*((volatile uint16_t *)(0x00100C06ul)));

#ifdef IN_CLICK
    stcClkCfg.enClkSrc = SysctrlClkRCH;    // 内部RC振荡器
    stcClkCfg.enHClkDiv = SysctrlHclkDiv1; // 不分频
    stcClkCfg.enPClkDiv = SysctrlPclkDiv1; // 不分频
    Sysctrl_ClkInit(&stcClkCfg);

    // 使能RCL
    Sysctrl_ClkSourceEnable(SysctrlClkRCL, TRUE);
#else
    stcClkCfg.enClkSrc = SysctrlClkXTH;    // 内部RC振荡器
    stcClkCfg.enHClkDiv = SysctrlHclkDiv1; // 不分频
    stcClkCfg.enPClkDiv = SysctrlPclkDiv1; // 不分频
    Sysctrl_ClkInit(&stcClkCfg);

    // 使能RCL
    Sysctrl_ClkSourceEnable(SysctrlClkXTH, TRUE);
#endif

    SystemCoreClockUpdate();
}
