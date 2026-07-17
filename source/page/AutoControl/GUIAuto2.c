#include "GUIPublic.h"
#include "language.h"
#include "page.h"
#include "SGUI_PolarCoordinates.h"
#include "FontBigNumber.h"
#include "AppManage.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_AUTO2

/*唯一对象名*/
#define GUI_OBJECT g_stGUIAuto2

/*日志标识*/
static char *TAG = "Auto2";

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

#define CLOCK_RADIUS (16)
#define CLOCK_CENTER_Y_COORD (55)

static SGUI_NUM_VARBOX_STRUCT TempVariableBox;

/*函数申明*/
static void HMI_Plate(SGUI_SCR_DEV *pstDeviceIF, int x_coord, int y_coord, SGUI_INT Coordinates, float data, const char *srt);

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    appManage_switchTask(APP_TASK_PRESSURE);
    pstDeviceIF->fnClear(); /*清屏*/

    SGUI_NUM_VARBOX_PARAM stNumBoxInitParam;

    DrawStr(pstDeviceIF, 02, 2, 0, STR_LEFT, tr("In Value"));
    DrawStr(pstDeviceIF, 02 + 50, 2, 0, STR_LEFT, "%2.1f%%", 50.0);
    DrawStr(pstDeviceIF, 98, 2, 0, STR_LEFT, tr("SetValue"));
    DrawStr(pstDeviceIF, 98 + 50, 2, 0, STR_LEFT, "%2.1f%%", 50.0);

    /*温度*/
    stNumBoxInitParam.eAlignment = SGUI_CENTER;
    stNumBoxInitParam.iMin = -99;
    stNumBoxInitParam.iMax = 99;
    stNumBoxInitParam.pstFontRes = &UFT8_FZXS12;
    stNumBoxInitParam.stLayout.iX = 167;
    stNumBoxInitParam.stLayout.iY = 16;
    stNumBoxInitParam.stLayout.iWidth = UFT8_FZXS12.iHalfWidth * 2;
    stNumBoxInitParam.stLayout.iHeight = UFT8_FZXS12.iHeight;
    SGUI_NumberVariableBox_Initialize(&TempVariableBox, &stNumBoxInitParam);
    DrawStr(pstDeviceIF, stNumBoxInitParam.stLayout.iX + stNumBoxInitParam.stLayout.iWidth, stNumBoxInitParam.stLayout.iY, 0, STR_LEFT, "℃");

    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/

    SysTickTimerEnable(true); /*启动定时器*/
    return HMI_RET_NORMAL;
}

/*绘制页面*/
HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    /*边框*/
    SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 0, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight, SGUI_COLOR_FRGCLR, SGUI_COLOR_TRANS);

    /*温度*/
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &TempVariableBox, SGUI_DRAW_NORMAL);

    HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 - 45, CLOCK_CENTER_Y_COORD, 0, 0, tr("IN"));
    HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 - 00, CLOCK_CENTER_Y_COORD, 0, 0, tr("OUT1"));
    HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 + 45, CLOCK_CENTER_Y_COORD, 0, 0, tr("OUT2"));

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
    static unsigned int aaa = GUI_ID;

    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        // HMI_SwitchScreen(HMI_SCREEN_ID_MENU_ALL, NULL);
        HMI_SwitchScreen(HMI_SCREEN_ID_GUI_COUNT_DOWN, &aaa);
        break;
    case KEY_VALUE_ESC:
        appManage_switchTask(APP_TASK_AUTO_CONTROL);

        GUI_OBJECT.pstPrevious = NULL;
        HMI_SwitchScreen(HMI_SCREEN_ID_HOME, NULL);
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
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF)
{
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;

    static int i;
    i++;
    float32_t data1 = Port_GetFloat(DATA_F_PRESSURE_CH1) - 100;
    float32_t data2 = Port_GetFloat(DATA_F_PRESSURE_CH2) - 100;
    float32_t data3 = Port_GetFloat(DATA_F_PRESSURE_CH3) - 100;
    if (data1 >= 0)
        HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 - 45, CLOCK_CENTER_Y_COORD, data1 / 100.0, data1 / 100.0, tr("IN"));
    if (data2 >= 0)
        HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 - 00, CLOCK_CENTER_Y_COORD, data2 / 100.0, data2 / 100.0, tr("OUT1"));
    if (data3 >= 0)
        HMI_Plate(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2 + 45, CLOCK_CENTER_Y_COORD, data3 / 100.0, data3 / 100.0, tr("OUT2"));

    SGUI_NumberVariableBox_SetValue(&TempVariableBox, Port_GetFloat(DATA_F_TEMP));
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &TempVariableBox, SGUI_DRAW_NORMAL);

    return iProcessAction;
}

static void HMI_Plate(SGUI_SCR_DEV *pstDeviceIF, int x_coord, int y_coord, SGUI_INT Coordinates, float data, const char *srt)
{
    SGUI_INT iDialAngl = 0;

    if (Coordinates > 180)
        Coordinates = 180;

    /*两个圆环*/
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS + 3, SGUI_COLOR_FRGCLR, SGUI_COLOR_TRANS);
    SGUI_Basic_DrawCircle(pstDeviceIF, x_coord, y_coord, CLOCK_RADIUS, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);

    /*绘制刻度*/
    SGUI_PolarCoord_DrawLine(pstDeviceIF, x_coord, y_coord, 90, CLOCK_RADIUS - 3, CLOCK_RADIUS - 1, SGUI_COLOR_FRGCLR);  /*6点*/
    SGUI_PolarCoord_DrawLine(pstDeviceIF, x_coord, y_coord, 180, CLOCK_RADIUS - 3, CLOCK_RADIUS - 1, SGUI_COLOR_FRGCLR); /*9点*/
    SGUI_PolarCoord_DrawLine(pstDeviceIF, x_coord, y_coord, 270, CLOCK_RADIUS - 3, CLOCK_RADIUS - 1, SGUI_COLOR_FRGCLR); /*12点*/

    /*表盘指针*/
    iDialAngl = (Coordinates % 60) * 6;
    SGUI_PolarCoord_DrawLine(pstDeviceIF, x_coord, y_coord, 270 - iDialAngl, 4, CLOCK_RADIUS - 6, SGUI_COLOR_FRGCLR);

    SGUI_RECT stDisplayArea;
    SGUI_POINT stInnerPos;
    stInnerPos.iX = 0;
    stInnerPos.iY = 0;
    stDisplayArea.iX = x_coord - 3;
    stDisplayArea.iY = y_coord - (SGUI_DEFAULT_FONT_MiniNum.iHeight / 2);
    stDisplayArea.iWidth = 3 * SGUI_DEFAULT_FONT_MiniNum.iHalfWidth;
    stDisplayArea.iHeight = SGUI_DEFAULT_FONT_MiniNum.iHeight + 1;
    char buff[32];
    sprintf(buff, "%2.1f", data);
    SGUI_Text_DrawText(pstDeviceIF, buff, &SGUI_DEFAULT_FONT_MiniNum, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);

    DrawStr(pstDeviceIF, x_coord, y_coord + CLOCK_RADIUS + 4, 0, STR_MEDIATE, srt);
}
