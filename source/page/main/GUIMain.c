#include "GUIMain.h"
#include "SGUI_Basic.h"
#include "GUIActions.h"
#include "Disp.h"
#include "key.h"
#include "UserTim4.h"
#include "UserTim6.h"
#include "language.h"
#include "Page.h"
#include "Save.h"
#include "led.h"
#include "System.h"
#include "DataPort.h"

#define BMP_DATA_BUFFER_SIZE (512)

SGUI_SCR_DEV g_stDeviceInterface;

HMI_SCREEN_OBJECT *g_arrpstScreenObjs[] = {
    &g_stGUIHomeSet,
    &g_stGUISystem,
    &g_stGUIMenuAll,
    &g_stGUIModule,
    &g_stGUIDebugPage1,
    &g_stGUIAuto2,
    &g_stGUIMAutoSet,
    &g_stGUIInputSet,
    &g_stGUICurve,
    &g_stGUIMechanical,
    &g_stGUITravel,
    &g_stGUIControlParameter,
    &g_stGUICountDown,
    &g_stGUITest,
    &g_stGUIAutoAdjust,
    &g_stGUICurrentCalib,
    &g_stGUITravelParameter,
    &g_stGUIIPParameter,
};

HMI_ENGINE_OBJECT g_stDemoEngine;
SGUI_BYTE s_pBmpDataBuffer[BMP_DATA_BUFFER_SIZE];

static void KeyPressEventProc(uint16_t key);
static void RTCEventProc(void);
static void SysTickTimerEventProc(void);

SGUI_SCR_DEV *gui_get_DeviceIF()
{
    return &g_stDeviceInterface;
}

uint8_t auto_disp_dir(void)
{
//    Qmi8658_t Qmi8658;
//    DEBUG_LOGI("GUIMain", "自动调整屏幕方向:");
//    for (int i = 0; i < 10; i++)
//    {
//        if (QMI8658_ReadData(&Qmi8658) == 0)
//        {
//            /*屏幕自动旋转功能*/
//            if (Qmi8658.gx > 8000)
//                return 2;
//        }
//    }
    return 1;
}

void disp_hal_init(void)
{
    SGUI_SystemIF_MemorySet(&g_stDeviceInterface, 0x00, sizeof(SGUI_SCR_DEV));
    g_stDeviceInterface.stSize.iWidth = DISP_HOR_RES;
    g_stDeviceInterface.stSize.iHeight = DISP_VER_RES;

    int32_t disp_dir = Read_System(SAVE_DISPLAY_MIRROR);
    if (disp_dir == 0) /*自动*/
    {
        disp_dir = auto_disp_dir();
    }
    if (disp_dir == 2)
    {
        /*反向*/
        g_stDeviceInterface.fnSetPixel = (void *)Disp_SetPixel_mirror;
        g_stDeviceInterface.fnSyncBuffer = Disp_RefreshDisplay_mirror;
        DEBUG_LOGI("GUIMain", "屏幕反转");
    }
    else
    {
        /*正常*/
        g_stDeviceInterface.fnSetPixel = (void *)Disp_SetPixel;
        g_stDeviceInterface.fnSyncBuffer = Disp_RefreshDisplay;
        DEBUG_LOGI("GUIMain", "屏幕正常");
    }

    g_stDeviceInterface.fnFillRect = NULL;
    g_stDeviceInterface.fnClear = Disp_ClearDisplay;
    g_stDeviceInterface.stBuffer.pBuffer = s_pBmpDataBuffer;
    g_stDeviceInterface.stBuffer.sSize = BMP_DATA_BUFFER_SIZE;
}

HMI_ENGINE_RESULT GUIMainInit(void)
{
    HMI_ENGINE_RESULT eProcessResult;
    int iIndex;
    eProcessResult = HMI_RET_NORMAL;

    /*设置语言*/
    set_language(Read_System(SAVE_LANGUAGE));

    /*屏幕接口初始化*/
    disp_hal_init();

    SGUI_SystemIF_MemorySet(&g_stDemoEngine, 0x00, sizeof(HMI_ENGINE_OBJECT));

    /*绑定界面*/
    g_stDemoEngine.ScreenCount = sizeof(g_arrpstScreenObjs) / sizeof(*g_arrpstScreenObjs);
    g_stDemoEngine.ScreenObjPtr = g_arrpstScreenObjs;
    g_stDemoEngine.Interface = &g_stDeviceInterface;

    /*初始化所有界面*/
    if (NULL != g_stDemoEngine.ScreenObjPtr)
    {
        for (iIndex = 0; iIndex < g_stDemoEngine.ScreenCount; iIndex++)
        {
            if ((NULL != g_stDemoEngine.ScreenObjPtr[iIndex]) && (NULL != g_stDemoEngine.ScreenObjPtr[iIndex]->pstActions) && (NULL != g_stDemoEngine.ScreenObjPtr[iIndex]->pstActions->Initialize))
            {
                g_stDemoEngine.ScreenObjPtr[iIndex]->pstActions->Initialize(&g_stDeviceInterface);
                g_stDemoEngine.ScreenObjPtr[iIndex]->pstPrevious = NULL;
            }
        }
    }

    /* Active engine object. */ // HMI_SCREEN_ID_GUI_AUTO_ADJUST HMI_SCREEN_ID_HOME
    eProcessResult = HMI_ActiveEngine(&g_stDemoEngine, HMI_SCREEN_ID_HOME);
    if (HMI_PROCESS_FAILED(eProcessResult))
    {
        /* Active engine failed. */
        return 0;
    }
    /* Start engine process. */
    eProcessResult = HMI_StartEngine(NULL);
    if (HMI_PROCESS_FAILED(eProcessResult))
    {
        /* Start engine failed. */
        return 0;
    }

    return eProcessResult;
}

void GUIMainProcess(void)
{
    static uint32_t clock = 0;

    // Check and process heart-beat timer event.
    if (true == SysTickTimerTriggered())
    {
        uint32_t time = get_SystemTime();
        SysTickTimerEventProc();
        // DEBUG_LOGW("main", "绘制GUI用时:%dmS", get_SystemTime() - time);
    }

    /*50ms运行一次*/
    if (get_SystemTime() > (clock + 50))
    {
        clock = get_SystemTime();

        // Check and process key press event.
        if (KEY_GetState() != -1)
        {
            KeyPressEventProc(0);
        }

        int port_key = Port_GetInt(DATA_I_GUI_KEY_VALUE);
        if (port_key != 0)
        {
            Port_SetInt(DATA_I_GUI_KEY_VALUE, 0);
            KeyPressEventProc(port_key);
        }
    }
}

/*****************************************************************************/
/** Function Name:  KeyPressEventProc                                       **/
/** Purpose:        Deal with user key process in demo process.             **/
/** Parameters:     None.                                                   **/
/** Return:         None.                                                   **/
/** Notice:         The button is just abstract, and the message may come   **/
/**                 from a touch screen, a serial port, and so on.          **/
/*****************************************************************************/
void KeyPressEventProc(uint16_t key)
{
    KEY_PRESS_EVENT stEvent;
    HMI_EVENT_INIT(stEvent);

    stEvent.Head.iID = EVENT_ID_KEY_PRESS;

    if (key == 0)
        stEvent.Data.uiKeyValue = KEY_GetValue();
    else
        stEvent.Data.uiKeyValue = key;

    // stEvent.Data.uiKeyValue |= KEY_OPTION_SHIFT;
    // stEvent.Data.uiKeyValue |= KEY_OPTION_CTRL;
    // stEvent.Data.uiKeyValue |= KEY_OPTION_ALT;

    // Post key press event.
    HMI_ProcessEvent((HMI_EVENT_BASE *)(&stEvent));
}

/*****************************************************************************/
/** Function Name:  SysTickTimerEventProc                                   **/
/** Purpose:        Deal with heartbeat timer event in demo process.        **/
/** Parameters:     None.                                                   **/
/** Return:         None.                                                   **/
/** Notice:         It is usually used to simulate ADC or other sensors.    **/
/*****************************************************************************/
void SysTickTimerEventProc(void)
{
    /*----------------------------------*/
    /* Variable Declaration             */
    /*----------------------------------*/
    DATA_EVENT stEvent;

    /*----------------------------------*/
    /* Initialize                       */
    /*----------------------------------*/
    HMI_EVENT_INIT(stEvent);

    /*----------------------------------*/
    /* Process                          */
    /*----------------------------------*/
    stEvent.Head.iID = EVENT_ID_TIMER;
    stEvent.Data.iValue = (rand() % 200) - 100;
    // Post timer event.
    HMI_ProcessEvent((HMI_EVENT_BASE *)&stEvent);
}

/*****************************************************************************/
/** Function Name:  SysTickTimerTriggered                                   **/
/** Purpose:        Check sys-tick timer interrupt is triggered.            **/
/** Parameters:     None.                                                   **/
/** Return:         None.                                                   **/
/** Notice:         None.                                                   **/
/*****************************************************************************/
bool SysTickTimerTriggered(void)
{
    return Tim4_Get_OverFlag();
}

/*****************************************************************************/
/** Function Name:  SysTickTimerEnable                                      **/
/** Purpose:        Enable or disable sys-tick timer.                       **/
/** Parameters:                                                             **/
/** @ bEnable[in]:  True for enable sys-tick timer, false for disable.      **/
/** Return:         None.                                                   **/
/** Notice:         None.                                                   **/
/*****************************************************************************/
void SysTickTimerEnable(bool bEnable)
{
    Tim4_ConfigPeriod(bEnable ? HEART_BEAT_INTERVAL_MS : 0);
}
