/*
 * @Author: XinXin
 * @Date: 2024-05-12 09:14:01
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-13 18:36:23
 * @FilePath: \source\app\System\SystemServer.c
 * @Description:
 */

#include "SystemServer.h"
#include "UserTim6.h"
#include "Disp.h"
#include "useruart1.h"
#include "GUIMain.h"

static uint32_t clock_100ms = 0;
static uint32_t clock_1000ms = 0;
static uint32_t clock_10s = 0;

void SystemServer_Init(void)
{
	
	
}

void SystemServer(void)
{
#ifdef RT_DISP_MIRROR
    if (get_delay(&clock_100ms, 100))
    {
        Qmi8658_t Qmi8658;
        QMI8658_ReadData(&Qmi8658);

        /*屏幕自动旋转功能*/
        static uint8_t DisplayMirror;
        static uint8_t old_DisplayMirror;
        if (Qmi8658.gx > 8000)
            DisplayMirror = 1;
        else
            DisplayMirror = 0;
        if (DisplayMirror != old_DisplayMirror)
        {
            old_DisplayMirror = DisplayMirror;
            disp_hal_init();
        }
    }
#endif

    if ((get_delay(&clock_10s, 10000)))
    {
			//在这里做温湿度的逻辑
        // AHT20_GetTemperature();
    }
}
