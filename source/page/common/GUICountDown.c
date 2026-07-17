#include "GUIPublic.h"
#include "FontBigNumber.h"
#include "SGUI_PolarCoordinates.h"
#include "led.h"
#include "language.h"
#include "UserTim6.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_GUI_COUNT_DOWN

/*唯一对象名*/
#define GUI_OBJECT g_stGUICountDown

/*日志标识*/
static char *TAG = "CountDown";

/*函数声明*/
static HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF);
static HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUI_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID);
static HMI_ENGINE_RESULT HMI_GUI_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID);
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT16 uiKeyValue);
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF);

/*GUI接口*/
static const HMI_SCREEN_ACTION s_stGUIActions = {
    HMI_GUI_Initialize,
    HMI_GUI_Prepare,
    HMI_GUI_RefreshScreen,
    HMI_GUI_ProcessEvent,
    HMI_GUI_PostProcess};

/*导出GUI接口*/
EXPORT_PAGE(GUI_OBJECT, GUI_ID, s_stGUIActions);

#define COUNT_DOWN_TIM 3000 // 倒计时 单位：ms
const int CLOCK_RADIUS = 20;

static uint32_t Start_time;
static bool flag = false;
/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    Start_time = get_SystemTime();

    if (pstParameters == NULL)
    {
        flag = true;

        /*如果是从别的界面进入则直接退回主页*/
        SysTickTimerEnable(false);
    }
    else
    {
        flag = false;
        /*如果是从主页进入，则绘制进度条*/
        pstDeviceIF->fnClear();                            /*清屏*/
        HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
        SysTickTimerEnable(true);                          /*启动定时器*/
    }
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
            eProcessResult = GUI_Time_Event(pstDeviceIF);
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
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT16 uiKeyValue)
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
    case KEY_VALUE_NONE:
        if (((get_SystemTime() - Start_time) < COUNT_DOWN_TIM))
        {
            iProcessAction = HMI_DEMO_PROC_CANCEL;
        }
        else
        {
            HMI_SwitchScreen(HMI_SCREEN_ID_MENU_ALL, (const void *)true);
        }
        break;

    default:
        break;
    }
    return iProcessAction;
}

static void HMI_Plate(SGUI_SCR_DEV *pstDeviceIF, int x_coord, int y_coord, SGUI_INT Coordinates)
{
    SGUI_INT iDialAngl = 0;

    if (Coordinates > 100)
        Coordinates = 100;

/*两个圆环*/
#if 1
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS + 3, SGUI_COLOR_FRGCLR, SGUI_COLOR_TRANS);
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);
    for (int i = 0; i < 360; i++)
    {
        if (i < ((float)Coordinates * 3.6f))
            SGUI_PolarCoord_DrawLine(pstDeviceIF, x_coord, y_coord, i, CLOCK_RADIUS, CLOCK_RADIUS + 3, SGUI_COLOR_FRGCLR);
    }
#else
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS + 20, SGUI_COLOR_BKGCLR, SGUI_COLOR_BKGCLR);
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS + 20 - (Coordinates / (100 / 20)), SGUI_COLOR_FRGCLR, SGUI_COLOR_FRGCLR);
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);
#endif
}

/*定时事件*/
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF)
{
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    char StringBuffer[10];
    SGUI_RECT stDisplayArea;
    SGUI_POINT stInnerPos = {0, 0};

    const uint8_t y_spot = (pstDeviceIF->stSize.iHeight / 2) - 5;

    stDisplayArea.iX = (pstDeviceIF->stSize.iWidth / 2) - (BigNumber_FZXS24.iHalfWidth / 2);
    stDisplayArea.iY = y_spot - BigNumber_FZXS24.iHeight / 2;
    stDisplayArea.iWidth = BigNumber_FZXS24.iHalfWidth;
    stDisplayArea.iHeight = BigNumber_FZXS24.iHeight;

    uint32_t timer = get_SystemTime() - Start_time;
    if (timer < COUNT_DOWN_TIM)
    {
        HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, y_spot, (timer) * (100.0f / (float)(COUNT_DOWN_TIM)));

        /*擦除数字区域*/
        SGUI_Basic_FillRectangleArea(pstDeviceIF, stDisplayArea.iX, stDisplayArea.iY, stDisplayArea.iWidth, stDisplayArea.iHeight, SGUI_COLOR_BKGCLR);

        /*绘制数字*/
        SGUI_Common_IntegerToString((int)((COUNT_DOWN_TIM - timer) / 1000) + 1, StringBuffer, 10, 1, ' ');
        SGUI_Text_DrawText(pstDeviceIF, StringBuffer, &BigNumber_FZXS24, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);
    }
    else
    {
        HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, y_spot, 100);

        /*绘制数字*/
        SGUI_Text_DrawText(pstDeviceIF, "0", &BigNumber_FZXS24, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);
        DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2,
                pstDeviceIF->stSize.iHeight / 2 + (CLOCK_RADIUS + 5),
                0, STR_MEDIATE, tr("Release the button go menu"));
    }

    return iProcessAction;
}
