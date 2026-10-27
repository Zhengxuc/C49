#include "GUIPublic.h"
#include "SGUI_ProcessBar.h"
#include "System.h"
#include "UserTim6.h"
#include "Draw.h"
#include "ValveAutoAdjust.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_GUI_AUTO_ADJUST

/*唯一对象名*/
#define GUI_OBJECT g_stGUIAutoAdjust

/*日志标识*/
static char *TAG = "AutoAdjust";

/*函数声明*/
static HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF);
static HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUI_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID);
static HMI_ENGINE_RESULT HMI_GUI_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID);
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT16 uiKeyValue);
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF);
static void ProcessBar(SGUI_SCR_DEV *pstDeviceIF, int value);
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
    pstDeviceIF->fnClear(); /*清屏*/

    ProcessBar(pstDeviceIF, 0);

    DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, 3, 0, STR_MEDIATE, tr("自动调节运行参数"));

    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
    SysTickTimerEnable(true);                          /*启动定时器*/

    /*切换APP，并复位状态机（否则上次的完成标志会让本页立即自动返回）*/
    appManage_switchTask(APP_TASK_AUTO_ADJ);
    ValveAutoAdjust_Reset();

    return HMI_RET_NORMAL;
}

/*绘制页面*/
HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
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
            iProcessAction = GUI_Key_Event(pstDeviceIF, KEY_CODE_VALUE(pstKeyEvent->Data.uiKeyValue));
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
            iProcessAction = (SGUI_INT)GUI_Time_Event(pstDeviceIF);
        }
        break;
    default:
        break;
    }

    if (NULL != piActionID)
    {
        *piActionID = iProcessAction;
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
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT16 uiKeyValue)
{
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        /* 开始零/满位自整定（状态机在 ControlTask 中运行） */
        ValveAutoAdjust_Start();
        break;
    case KEY_VALUE_ESC:
        ValveAutoAdjust_Cancel();
        appManage_switchTask(APP_TASK_AUTO_CONTROL);
        iProcessAction = HMI_DEMO_PROC_CANCEL;
        break;
    case KEY_VALUE_UP:
        break;
    case KEY_VALUE_DOWN:
        break;
    case KEY_VALUE_F1:
        ValveAutoAdjust_Cancel();
        appManage_switchTask(APP_TASK_AUTO_CONTROL);
        iProcessAction = HMI_DEMO_PROC_CANCEL;
        break;
    default:
        break;
    }
    return iProcessAction;
}

/*定时事件*/
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF)
{
    char strBuff[32];
    const char *p;

    int32_t all_step = 0, step = 0;

    /*标题*/
    p = Port_GetStr(DATA_S_AUTO_ADJ_TITLE);
    if (p != NULL)
    {
        DrawRt(pstDeviceIF, pstDeviceIF->stSize.iWidth / 4, 40, pstDeviceIF->stSize.iWidth / 2, 12);
        DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, 40, 0, STR_MEDIATE, "%s", p);
    }

    /*描述名称*/
    p = Port_GetStr(DATA_S_AUTO_ADJ_DESC);
    if (p != NULL)
    {
        DrawRt(pstDeviceIF, 0, 58, pstDeviceIF->stSize.iWidth / 2, 12);
        DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 - 3, 58, 0, STR_RIGHT, "%s:", p);
    }

    /*内容名称*/
    p = Port_GetStr(DATA_S_AUTO_ADJ_CONTENT);
    if (p != NULL)
    {
        DrawRt(pstDeviceIF, 0, 74, pstDeviceIF->stSize.iWidth / 2, 12);
        DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 - 3, 74, 0, STR_RIGHT, "%s:", p);
    }

    /*描述值*/
    float32_t f;
    if (Port_Get((void *)&f, DATA_F_AUTO_ADJ_DESC_VALUE))
    {
        Draw_buff_Byte(pstDeviceIF->stSize.iWidth / 2, 56, pstDeviceIF->stSize.iWidth / 2, 16, 0x00);
        snprintf(strBuff, sizeof(strBuff), "%.2f", f);
        Draw_w8h16_string(pstDeviceIF->stSize.iWidth / 2 + 3, 56, strBuff, 0);
    }

    /*内容值*/
    if (Port_Get((void *)&f, DATA_F_AUTO_ADJ_CONTENT_VALUE))
    {
        Draw_buff_Byte(pstDeviceIF->stSize.iWidth / 2, 72, pstDeviceIF->stSize.iWidth / 2, 16, 0x00);
        snprintf(strBuff, sizeof(strBuff), "%.2f", f);
        Draw_w8h16_string(pstDeviceIF->stSize.iWidth / 2 + 3, 72, strBuff, 0);
    }

    /*总步骤*/
    if (Port_Get((void *)&all_step, DATA_I_AUTO_ADJ_ALL_STEP))
    {
        DrawStr(pstDeviceIF, 40 / 2, 18,0,  STR_MEDIATE, "%d/%d", step, all_step);
    }

    /*当前步骤*/
    if (Port_Get((void *)&step, DATA_I_AUTO_ADJ_CURRENT_STEP))
    {
        DrawStr(pstDeviceIF, 40 / 2, 18,0,  STR_MEDIATE, "%d/%d", step, all_step);
    }

    /*进度条*/
    int progress;
    if (Port_Get((void *)&progress, DATA_I_AUTO_ADJ_PROGRESS))
    {
        if (progress > 100)
            progress = 100;

        ProcessBar(pstDeviceIF, progress);

        snprintf(strBuff, sizeof(strBuff), "%3d%%", progress);
        Draw_buff_Byte(pstDeviceIF->stSize.iWidth - 35, 16, 32, 16, 0x00);
        Draw_w8h16_string(pstDeviceIF->stSize.iWidth - 35, 16, strBuff, 0);
    }
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;

    /* 整定结束（完成/失败）：自动返回上一页，任务恢复由状态机完成 */
    if (ValveAutoAdjust_IsDone())
    {
        iProcessAction = HMI_DEMO_PROC_CANCEL;
    }
    return iProcessAction;
}

static void ProcessBar(SGUI_SCR_DEV *pstDeviceIF, int value)
{
    Draw_buff_Byte(40, 16, pstDeviceIF->stSize.iWidth - 80, 8, 0x01);
    Draw_buff_Byte(40, 24, pstDeviceIF->stSize.iWidth - 80, 8, 0x80);
    Draw_buff_Byte(40, 16, 1, 16, 0xff);
    Draw_buff_Byte(pstDeviceIF->stSize.iWidth - 40, 16, 1, 16, 0xff);
    Draw_buff_Byte(40, 16, ((float)(pstDeviceIF->stSize.iWidth - 80.0f) / 100.0f * value), 16, 0xff);
}