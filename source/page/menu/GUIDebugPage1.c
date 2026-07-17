#include "GUIPublic.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_DEBUG_PAGE1

/*唯一对象名*/
#define GUI_OBJECT g_stGUIDebugPage1

/*日志标识*/
static char *TAG = "DebugPage1";

/*函数声明*/
static HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF);
static HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUI_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID);
static HMI_ENGINE_RESULT HMI_GUI_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID);
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_UINT16 uiKeyValue);
static HMI_ENGINE_RESULT GUI_Time_Event(void);

/*GUI接口*/
static const HMI_SCREEN_ACTION s_stGUIActions = {
    HMI_GUI_Initialize,
    HMI_GUI_Prepare,
    HMI_GUI_RefreshScreen,
    HMI_GUI_ProcessEvent,
    HMI_GUI_PostProcess};

/*导出GUI接口*/
EXPORT_PAGE(GUI_OBJECT, GUI_ID, s_stGUIActions);

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    pstDeviceIF->fnClear();                            /*清屏*/
    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
    return HMI_RET_NORMAL;
}

/*绘制页面*/
HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    /*边框*/
    SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 0, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight, SGUI_COLOR_FRGCLR, SGUI_COLOR_TRANS);

    return HMI_RET_NORMAL;
}

/*事件*/
HMI_ENGINE_RESULT HMI_GUI_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID)
{
    HMI_ENGINE_RESULT eProcessResult;
    SGUI_INT iProcessAction;
    KEY_PRESS_EVENT *pstKeyEvent;
    DATA_EVENT *pstDataEvent;

    eProcessResult = HMI_RET_NORMAL;
    iProcessAction = HMI_DEMO_PROC_NO_ACT;
    pstKeyEvent = (KEY_PRESS_EVENT *)pstEvent;
    switch (pstEvent->iID)
    {
    case EVENT_ID_KEY_PRESS:
        if (SGUI_FALSE == HMI_EVENT_SIZE_CHK(*pstKeyEvent, KEY_PRESS_EVENT))
        {
            eProcessResult = HMI_RET_INVALID_DATA;
        }
        else if (EVENT_ID_KEY_PRESS == pstEvent->iID)
        {
            iProcessAction = GUI_Key_Event(KEY_CODE_VALUE(pstKeyEvent->Data.uiKeyValue));
        }
        if (NULL != piActionID)
        {
            *piActionID = iProcessAction;
        }
        break;
    case EVENT_ID_TIMER:
        pstDataEvent = (DATA_EVENT *)pstEvent;
        if (SGUI_FALSE == HMI_EVENT_SIZE_CHK(*pstDataEvent, DATA_EVENT))
        {
            eProcessResult = HMI_RET_INVALID_DATA;
        }
        else
        {
            eProcessResult = GUI_Time_Event();
        }
        break;
    default:
        break;
    }

    return eProcessResult;
}

/*GUI进程*/
HMI_ENGINE_RESULT HMI_GUI_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID)
{
    if (HMI_PROCESS_SUCCESSFUL(eProcResult))
    {
        if (HMI_DEMO_PROC_CANCEL == iActionID)
        {
            HMI_GoBack(NULL);
        }
    }
    return HMI_RET_NORMAL;
}

/*按键事件*/
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_UINT16 uiKeyValue)
{
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        break;
    case KEY_VALUE_ESC:
        iProcessAction = HMI_DEMO_PROC_CANCEL;
        break;
    case KEY_VALUE_UP:
        break;
    case KEY_VALUE_DOWN:
        break;
    default:
        break;
    }
    return iProcessAction;
}

/*定时事件*/
static HMI_ENGINE_RESULT GUI_Time_Event(void)
{
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    return iProcessAction;
}
