#include "GUIPublic.h"
#include "SGUI_Curve.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_GUI_CURVE

/*唯一对象名*/
#define GUI_OBJECT g_stGUICurve

/*日志标识*/
static char *TAG = "GUICurve";

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

#define CURVE_POINT_MUN_MAX (20)
static SGUI_CURVE_STRUCT s_stCurve = {0x00};
static SGUI_BOOL s_arrbPointDataUsed[CURVE_POINT_MUN_MAX] = {0x00};
static SGUI_CURVE_POINT s_arrstPointArray[CURVE_POINT_MUN_MAX] = {0x00};
static SGUI_INT s_iArgument;

static SGUI_CURVE_POINT *HMI_DemoCurve_NewPoint(void);
static void HMI_DemoCurve_RepaintText(SGUI_SCR_DEV *pstDeviceIF);

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    SGUI_CURVE_PARAM stCurveInitParam;
    SGUI_CURVE_POINT *pstNewPoint;

    pstNewPoint = NULL;

    stCurveInitParam.stLayout.iX = 1;
    stCurveInitParam.stLayout.iY = 16;
    stCurveInitParam.stLayout.iWidth = pstDeviceIF->stSize.iWidth - 2;
    stCurveInitParam.stLayout.iHeight = pstDeviceIF->stSize.iHeight - 32;
    stCurveInitParam.stXRange.iMin = 0;
    stCurveInitParam.stXRange.iMax = 4;
    stCurveInitParam.stYRange.iMin = -10;
    stCurveInitParam.stYRange.iMax = 10;

    SGUI_Curve_Initialize(&s_stCurve, &stCurveInitParam);

    pstNewPoint = HMI_DemoCurve_NewPoint();
    SGUI_Curve_InitializePoint(pstNewPoint, 0, 0);
    SGUI_Curve_AddPoint(&s_stCurve, pstNewPoint);

    pstNewPoint = HMI_DemoCurve_NewPoint();
    SGUI_Curve_InitializePoint(pstNewPoint, 1, 0);
    SGUI_Curve_AddPoint(&s_stCurve, pstNewPoint);

    pstNewPoint = HMI_DemoCurve_NewPoint();
    SGUI_Curve_InitializePoint(pstNewPoint, 2, 0);
    SGUI_Curve_AddPoint(&s_stCurve, pstNewPoint);

    pstNewPoint = HMI_DemoCurve_NewPoint();
    SGUI_Curve_InitializePoint(pstNewPoint, 3, 0);
    SGUI_Curve_AddPoint(&s_stCurve, pstNewPoint);

    pstNewPoint = HMI_DemoCurve_NewPoint();
    SGUI_Curve_InitializePoint(pstNewPoint, 4, 0);
    SGUI_Curve_AddPoint(&s_stCurve, pstNewPoint);

    s_iArgument = s_stCurve.stData.stPoints.pstHead->stPoint.iX;

    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    pstDeviceIF->fnClear(); /*清屏*/
    SGUI_Curve_FocusPoint(&s_stCurve, 0);
    SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 0, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);
    SGUI_Basic_DrawLine(pstDeviceIF, 1, 15, pstDeviceIF->stSize.iWidth - 2, 15, SGUI_COLOR_FRGCLR);
    SGUI_Basic_DrawLine(pstDeviceIF, 1, pstDeviceIF->stSize.iHeight - 15, pstDeviceIF->stSize.iWidth - 2, pstDeviceIF->stSize.iHeight - 15, SGUI_COLOR_FRGCLR);
    DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, 2, 0, STR_MEDIATE, tr("User curve setting"));

    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
    SysTickTimerEnable(true);                          /*启动定时器*/
    return HMI_RET_NORMAL;
}

/*绘制页面*/
HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    SGUI_Curve_Repaint(pstDeviceIF, &s_stCurve);

    if (NULL != s_stCurve.stData.pstFocused)
    {
        SGUI_Curve_HighlightFocus(pstDeviceIF, &s_stCurve);
    }
    else
    {
        SGUI_INT iLineX = (s_iArgument - s_stCurve.stParam.stXRange.iMin) * SGUI_RECT_WIDTH(s_stCurve.stParam.stLayout) / SGUI_RANGE_SIZE(s_stCurve.stParam.stXRange) + s_stCurve.stParam.stLayout.iX;
        SGUI_Basic_DrawVerticalLine(pstDeviceIF, iLineX, s_stCurve.stParam.stLayout.iY, s_stCurve.stParam.stLayout.iY + s_stCurve.stParam.stLayout.iHeight - 1, SGUI_COLOR_FRGCLR);
    }
    HMI_DemoCurve_RepaintText(pstDeviceIF);

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

    SGUI_CURVE_POINT *pstFocusedPoint;
    SGUI_CURVE_POINT *pstPointObj;
    pstFocusedPoint = s_stCurve.stData.pstFocused;

    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        if (NULL == s_stCurve.stData.pstFocused)
        {
            s_stCurve.stData.pstFocused = s_stCurve.stData.stPoints.pstHead;
        }
        else
        {
            s_stCurve.stData.pstFocused = s_stCurve.stData.pstFocused->pstNext;
            if (NULL == s_stCurve.stData.pstFocused)
            {
                s_stCurve.stData.pstFocused = s_stCurve.stData.stPoints.pstHead;
            }
        }
        pstFocusedPoint = s_stCurve.stData.pstFocused;
        HMI_GUI_RefreshScreen(pstDeviceIF, NULL);
        break;
    case KEY_VALUE_ESC:
        iProcessAction = HMI_DEMO_PROC_CANCEL;
        break;
    case KEY_VALUE_UP:
        if (NULL != pstFocusedPoint)
        {
            SGUI_Curve_UpdateFocusPoint(&s_stCurve, pstFocusedPoint->stPoint.iX, pstFocusedPoint->stPoint.iY + 1);
            HMI_GUI_RefreshScreen(pstDeviceIF, NULL);
        }
        break;
    case KEY_VALUE_DOWN:
        if (NULL != pstFocusedPoint)
        {
            SGUI_Curve_UpdateFocusPoint(&s_stCurve, pstFocusedPoint->stPoint.iX, pstFocusedPoint->stPoint.iY - 1);
            HMI_GUI_RefreshScreen(pstDeviceIF, NULL);
        }
        break;
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
    return iProcessAction;
}

static SGUI_CURVE_POINT *HMI_DemoCurve_NewPoint(void)
{
    SGUI_INT iIndex;
    SGUI_CURVE_POINT *pstNewPoint;
    pstNewPoint = NULL;
    for (iIndex = 0; iIndex < CURVE_POINT_MUN_MAX; iIndex++)
    {
        if (SGUI_FALSE == s_arrbPointDataUsed[iIndex])
        {
            pstNewPoint = s_arrstPointArray + iIndex;
            s_arrbPointDataUsed[iIndex] = SGUI_TRUE;
            break;
        }
    }
    return pstNewPoint;
}

static void HMI_DemoCurve_RepaintText(SGUI_SCR_DEV *pstDeviceIF)
{
    DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2,
            pstDeviceIF->stSize.iHeight - UFT8_FZXS12.iHeight - 2,
            0, STR_MEDIATE,
            "         ",
            s_stCurve.stData.pstFocused->stPoint.iX,
            s_stCurve.stData.pstFocused->stPoint.iY);

    DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2,
            pstDeviceIF->stSize.iHeight - UFT8_FZXS12.iHeight - 2,
            0, STR_MEDIATE,
            "%d:%d",
            s_stCurve.stData.pstFocused->stPoint.iX,
            s_stCurve.stData.pstFocused->stPoint.iY);
}
