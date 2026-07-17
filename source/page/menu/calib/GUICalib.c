#include "GUIPublic.h"
#include "FontBigNumber.h"
#include "SGUI_PolarCoordinates.h"
#include "GUIPublic.h"
#include "ADX111.h"
#include "SGUI_ProcessBar.h"
#include "SaveBox.h"
#include "buffer.h"
#include "Draw.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_GUI_CURRENT_CALIB

/*唯一对象名*/
#define GUI_OBJECT g_stGUICurrentCalib

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
static void step_disp(SGUI_SCR_DEV *pstDeviceIF, int step);

/*GUI接口*/
static const HMI_SCREEN_ACTION s_stGUIActions = {
    HMI_GUI_Initialize,
    HMI_GUI_Prepare,
    HMI_GUI_RefreshScreen,
    HMI_GUI_ProcessEvent,
    HMI_GUI_PostProcess};

/*导出GUI接口*/
EXPORT_PAGE(GUI_OBJECT, GUI_ID, s_stGUIActions);

static int step;
static bool enable_step;
static SGUI_PROCBAR_STRUCT pstProcessBarData;
static bool calib_finish;
static int current_min;   // 最小电流
static int current_max;   // 最大电流
static uint8_t key_press; // 禁用按键期间按下了按键

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    step = 0;
    enable_step = true;
    calib_finish = false;
    key_press = 0;

    pstDeviceIF->fnClear(); /*清屏*/

    /*边框*/
    SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 25, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight - 25, SGUI_COLOR_FRGCLR, SGUI_COLOR_TRANS);

    pstProcessBarData.Parameter.PosX = 40;
    pstProcessBarData.Parameter.PosY = 72;
    pstProcessBarData.Parameter.Width = pstDeviceIF->stSize.iWidth - 80;
    pstProcessBarData.Parameter.Height = 16;
    pstProcessBarData.Parameter.Direction = SGUI_PROCBAR_RIGHT;
    pstProcessBarData.Parameter.MaxValue = 100;
    pstProcessBarData.Data.Value = 0;
    SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);

    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
    SysTickTimerEnable(true);                          /*启动定时器*/

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

    /*保存框事件*/
    switch (Save_box_ProcessEvent(pstDeviceIF, pstEvent))
    {
    case BOX_INVALID:
        break;
    case BOX_ENABLE:
        return HMI_RET_NORMAL;
    case BOX_RETURN:
        return HMI_RET_NORMAL;
    case BOX_YES:
//        save_input_calibration(current_min, current_max);
        if (get_setValue_Box_State() == true)
            Close_setValveBox(true);
        if (NULL != piActionID)
        {
            *piActionID = HMI_DEMO_PROC_CANCEL;
        }
        return HMI_RET_NORMAL;
    case BOX_NO:
        if (get_setValue_Box_State() == true)
            Close_setValveBox(false);
        if (NULL != piActionID)
        {
            *piActionID = HMI_DEMO_PROC_CANCEL;
        }
        return HMI_RET_NORMAL;
    default:
        break;
    }

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

int get_average(IntBuffer_t *pstIntBuffer, int num)
{
    int min = Buffer_Peek_back(pstIntBuffer, 0);
    int max = Buffer_Peek_back(pstIntBuffer, 0);
    int sum = 0;
    for (int i = 0; i < num; i++)
    {
        int value = Buffer_Peek_back(pstIntBuffer, i);
        sum += value;
        if (value < min)
            min = value;
        if (value > max)
            max = value;

        DEBUG_LOGD(TAG, "BUFF %d.%d", i, value);
    }
    DEBUG_LOGI(TAG, "平均值:%d", (sum - min - max) / (num - 2));
    DEBUG_LOGI(TAG, "最小值:%d", min);
    DEBUG_LOGI(TAG, "最大值:%d", max);
    return (sum - min - max) / (num - 2);
}

/*GUI进程*/
HMI_ENGINE_RESULT HMI_GUI_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID)
{
    int input_current;
    static IntBuffer_t input;            // 环形缓冲
    static int input_current_buffer[21]; // 缓冲区
    switch (step)
    {
    case 0: /*输入最小电流*/
        Buffer_Init(&input, input_current_buffer, sizeof(input_current_buffer) / sizeof(input_current_buffer[0]));
        enable_step = true;
        step_disp(pstDeviceIF, 1);
        input_current = ADX111_Read(0);
        Buffer_Push(&input, input_current);
        DrawStr(pstDeviceIF, 96, 40, 0, STR_MEDIATE, "%s:%dmA", tr("Adjust current to"), 4);
        Draw_buff_Byte(46, 56, 100, 16, 0x00);
        DrawStr(pstDeviceIF, 96, 56, 0, STR_MEDIATE, "%s:%d", tr("input value"), input_current);
        break;
    case 1: /*检察最小电流*/
        enable_step = false;
        pstProcessBarData.Data.Value += 5;
        input_current = ADX111_Read(0);
        Buffer_Push(&input, input_current);
        SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);
        if (pstProcessBarData.Data.Value >= 100)
        {
            if (Buffer_Length(&input) >= 20)
            {
                current_min = get_average(&input, 20);
                pstProcessBarData.Data.Value = 0;
                SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);
                step++;
            }
        }
        break;
    case 2: /*输入最大电流*/
        step_disp(pstDeviceIF, 2);
        input_current = ADX111_Read(0);
        Buffer_Clear(&input);
        Buffer_Push(&input, input_current);

        /*条件没达到，按下按键闪烁*/
        if (key_press % 3 == 0)
            DrawStr(pstDeviceIF, 96, 40, 0, STR_MEDIATE, "%s:%dmA", tr("Adjust current to"), 20);
        else
            DrawStr(pstDeviceIF, 96, 40, 0, STR_MEDIATE, "                       ");

        DrawStr(pstDeviceIF, 96, 56, 0, STR_MEDIATE, "%s:%d", tr("input value"), input_current);

        /*最大值大于最小值才允许下一步*/
        if (input_current > (current_min + 1000))
            enable_step = true;
        break;
    case 3: /*检查最小电流和最大电流*/
        enable_step = false;
        pstProcessBarData.Data.Value += 5;
        input_current = ADX111_Read(0);
        Buffer_Push(&input, input_current);
        SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);
        if (pstProcessBarData.Data.Value >= 100)
        {
            if (Buffer_Length(&input) >= 20)
            {
                current_max = get_average(&input, 20);
                pstProcessBarData.Data.Value = 0;
                SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);
                step++;
            }
        }
        break;
    case 4: /*校准结束*/
        enable_step = false;
        SGUI_Basic_FillRectangleArea(pstDeviceIF, 1, 26,
                                     pstDeviceIF->stSize.iWidth - 2,
                                     pstDeviceIF->stSize.iHeight - 27,
                                     SGUI_COLOR_BKGCLR);

        /*图标*/
        const SGUI_POINT stInnerPos = {0, 0};
        INS_SOURCE(gImage_ImageFinish);
        SGUI_RECT stArea;
        SGUI_BMP_RES ImageSrc = {51, 48, (const SGUI_BYTE *)gImage_ImageFinish};
        stArea.iX = pstDeviceIF->stSize.iWidth / 2 - 22;
        stArea.iY = 27;
        stArea.iWidth = 51;
        stArea.iHeight = 48;
        SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &ImageSrc, SGUI_DRAW_NORMAL);
        step_disp(pstDeviceIF, 3);
        DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, 80, 0, STR_MEDIATE, tr("End of calibration"));
        calib_finish = true;
        step++;
        break;
    case 5:
        break;
    default:
        break;
    }

    if (HMI_PROCESS_SUCCESSFUL(eProcResult))
    {
        if (HMI_DEMO_PROC_CANCEL == iActionID)
        {
            HMI_GoBack((const void *)2);
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
        if (enable_step)
        {
            enable_step = false;
            step++;
        }
        else
        {
            key_press = 9;
        }
        break;
    case KEY_VALUE_ESC:
        if (calib_finish)
        {
            Save_box(pstDeviceIF, true);
        }
        else
            iProcessAction = HMI_DEMO_PROC_CANCEL;
        break;
    case KEY_VALUE_UP:
        break;
    case KEY_VALUE_DOWN:
        break;
    case KEY_VALUE_NONE:
        break;

    default:
        break;
    }
    return iProcessAction;
}

/*定时事件*/
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF)
{
    /*控制闪烁*/
    if (key_press != 0)
        key_press--;
    return HMI_RET_NORMAL;
}

/*> 右边填充*/
static const char right_solid[] = {
    0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF9, 0xFF,
    0x08, 0x0C, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F};

static void step_disp(SGUI_SCR_DEV *pstDeviceIF, int step)
{
    SGUI_RECT stArea;
    SGUI_POINT stInnerPos = {0, 0};
    SGUI_BMP_RES stCharBitmap = {10, 20, (const SGUI_BYTE *)&right_solid};

    stArea.iX = 0;
    stArea.iY = 0;
    stArea.iWidth = 10;
    stArea.iHeight = 20;

    if (step == 1)
    {
        /*左边箭头*/
        SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &stCharBitmap, SGUI_DRAW_NORMAL);

        stArea.iX = 80;
        stArea.iY = 0;
        /*右边箭头*/
        SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &stCharBitmap, SGUI_DRAW_REVERSE);

        /*中间填充*/
        SGUI_Basic_FillRectangleArea(pstDeviceIF, 10, 0, 70, 20, SGUI_COLOR_FRGCLR);

        DrawStr_reverse(pstDeviceIF, 15, 4, STR_LEFT, tr("1.MinRange"));
    }
    else
    {
        SGUI_Basic_FillRectangleArea(pstDeviceIF, 0, 0, 90, 20, SGUI_COLOR_BKGCLR);
        DrawStr(pstDeviceIF, 15, 4, 0, STR_LEFT, tr("1.MinRange"));
    }

    if (step == 2)
    {
        stArea.iX = 90;
        stArea.iY = 0;
        /*左边箭头*/
        SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &stCharBitmap, SGUI_DRAW_NORMAL);

        /*中间填充*/
        SGUI_Basic_FillRectangleArea(pstDeviceIF, 100, 0, 70, 20, SGUI_COLOR_FRGCLR);

        stArea.iX = 170;
        stArea.iY = 0;
        /*右边箭头*/
        SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &stCharBitmap, SGUI_DRAW_REVERSE);

        DrawStr_reverse(pstDeviceIF, 105, 4, STR_LEFT, tr("2.MaxRange"));
    }
    else
    {
        /*中间填充*/
        SGUI_Basic_FillRectangleArea(pstDeviceIF, 90, 0, 90, 20, SGUI_COLOR_BKGCLR);

        DrawStr(pstDeviceIF, 105, 4, 0, STR_LEFT, tr("2.MaxRange"));
    }
}
