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
#include "DataPort.h"
#include "KTH5702.h"
#include "ADX111.h"
#include "ValveControl.h"
#include "Setpoint.h"
#include "ValveAutoAdjust.h"
#include "OutputDriver.h"
#include "AppManage.h"
#include "Save.h"
#include "common.h"

/* --------------------------------------------------------------------------
 * 任务优先级与栈分配
 *
 * FreeRTOS: configMAX_PRIORITIES = 5 → 优先级 0(空闲) ~ 4(最高)
 * 控制任务必须高于显示任务，保证 50ms 控制周期不被 GUI 阻塞
 * -------------------------------------------------------------------------- */
#define TASK_CTRL_PRIORITY        3u    /* 控制 + 传感：最高 */
#define TASK_DISP_PRIORITY        2u    /* 显示 + 按键 + 后台服务：中 */
#define TASK_CTRL_STACK           512u
#define TASK_DISP_STACK           512u

/* 输入电流校准参数保存在 Flash，由菜单"系统→电流标定"标定。
 * 标定流程：信号源给 4mA → 按确认 → 给 20mA → 按确认 → 保存。
 * 未校准时使用 Save.c 中的默认参数。 */

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
        vTaskDelay(pdMS_TO_TICKS(50));/* 界面/按键扫描周期 50ms（原 100ms，按键响应延迟减半） */
    }
}

/* --------------------------------------------------------------------------
 * ControlTask — 传感采集 + 闭环控制 + DAC 输出
 *
 * 职责（一个周期内完成，保证数据新鲜度）：
 *   1. KTH5702 阀位角度 → DATA_F_ANGLE
 *   2. ADX111  输入电流 → DATA_F_INPUT_CURRENT
 *   3. ValveControl_Run() → PID → OutputDriver → UserDac(DAC)
 *
 * 不做：GUI、Flash 擦写、长时间阻塞操作
 * -------------------------------------------------------------------------- */
static void ControlTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t loop_cnt = 0;
    while (1)
    {
        KTH5702_AngleData_t data;
        if (NONE_ERROR == KTH5702_ReadData(&data))
        {
            Port_SetFloat(DATA_F_ANGLE, data.angle_deg);
        }

        /* 输入电流：ADX111 连续模式下直接读最新转换结果，用 Flash 校准参数换算 */
        {
            int16_t raw = ADX111_GetLastResult();
            float ma = input_current_raw_to_ma_float(raw);
            Port_SetFloat(DATA_F_INPUT_CURRENT, ma);
            /* 4-20mA → 0~1000，供输入框显示 0.0~100.0% */
            Port_SetInt(DATA_I_INPUT_POINT, Setpoint_CurrentToPoint(ma));

            loop_cnt++;
            if ((loop_cnt % 100) == 0)
            {
                /* RTT printf 不支持 %f，放大 100 倍拆整数打印 */
                int32_t ma_i = (int32_t)(ma * 100.0f);
                uint8_t ma_neg = (ma_i < 0) ? 1u : 0u;
                if (ma_neg) { ma_i = -ma_i; }
                SEGGER_RTT_printf(0, "ADX111 raw=%d, current=%s%d.%02dmA\n", raw, (ma_neg ? "-" : ""), ma_i / 100, ma_i % 100);
            }
            /* 每 1s 打印角度原始值/换算角/磁场强度，观察抖动来源：mag 应平稳，
             * 若 mag 偏低或波动大 → 磁铁对位/气隙问题（机械层） */
            if ((loop_cnt % 20) == 0)
            {
                int32_t angle_i = (int32_t)(data.angle_deg * 100.0f);
                SEGGER_RTT_printf(0, "KTH5702 raw=%u, angle=%d.%02ddeg, mag=%u\n",
                                  data.raw_angle, angle_i / 100, angle_i % 100, data.raw_mag_plane);
            }
        }

        /* 设定值生产：按控制模式选源（按键/电流，预留485/HART）→ DATA_I_SETPOINT */
        Setpoint_Run();

        /* 同一时刻仅一套控制算法占用 DAC：
         * 自整定页面 → 零/满位标定状态机；其它页面 → 闭环定位 */
        if (appManage_getTask() == APP_TASK_AUTO_ADJ)
        {
            // ValveAutoAdjust_Run();
        }
        else
        {
            ValveControl_Run();
        }

        vTaskDelay(pdMS_TO_TICKS(CONTROL_PERIOD));
    }
}

int32_t main(void)
{
    App_SysClkInit();
    SEGGER_RTT_Init();
    system_HAL_Init();
    // ValveControl_Init();
    OutputDriver_Init();

    xTaskCreate(ControlTask, "Control", TASK_CTRL_STACK, NULL, TASK_CTRL_PRIORITY, NULL);
    xTaskCreate(DispKeyTask, "DispKey", TASK_DISP_STACK, NULL, TASK_DISP_PRIORITY, NULL);

    vTaskStartScheduler();
}

static void App_SysClkInit(void)
{		//晶振的频率可以通过这个设定
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

    // CLK INIT
    //RCH、 RCL、 XTH、 XTL 这四个时钟源中的任意两者可以相互切换； //内部rc振荡器受温度影响比较大，
    //PLL 只可与 RCH、 XTH 这两个时钟进行切换。
    stcClkCfg.enClkSrc = SysctrlClkXTH;    // 外部高速时钟//这里使用的内部震荡器件
    stcClkCfg.enHClkDiv = SysctrlHclkDiv1; // 不分频
    stcClkCfg.enPClkDiv = SysctrlPclkDiv1; // 不分频
    Sysctrl_ClkInit(&stcClkCfg);

    // 使能RCL
    Sysctrl_ClkSourceEnable(SysctrlClkXTH, TRUE);
#endif

    SystemCoreClockUpdate();
}
