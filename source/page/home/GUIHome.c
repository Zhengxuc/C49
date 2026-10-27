#include "GUIPublic.h"
#include "FontBigNumber.h"
#include "Save.h"
#include "Draw.h"
#include "FontAscii_w8h16.h"
#include "AppManage.h"
#include "ValveControl.h"
#include "Setpoint.h"       /* get_targetValuePercent 等手动设定接口 */
#include "key.h"            /* KEY_UP_PIN/KEY_DOWN_PIN：纯手动模式电平直读 */

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_HOME

/*唯一对象名*/
#define GUI_OBJECT g_stGUIHomeSet

/*日志标识*/
static char *TAG = "GUI_HOME";

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

#define SCALE_Y 35

/*指示箭头数据*/
const char image[] = {0x01, 0x03, 0x07, 0x03, 0x01};

static int adjust_number;          // 按键设置的目标值
static int adjust_state = 0;       // 指示目标是否生效
static SGUI_RECT InValue_rect;     // 输入值框
static SGUI_RECT Temp_rect;        // 温度框
static SGUI_RECT sensor_rect;      // 传感器

static void HMI_DemoPolarCoord_PaintClock(SGUI_SCR_DEV *pstDeviceIF, bool DrawBase, int scale);
static void DrawScaleMark(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, SGUI_SZSTR srt);
static void DrawBigNumber(SGUI_SCR_DEV *pstDeviceIF, float32_t num);
static void Disp_InValue(SGUI_SCR_DEV *pstDeviceIF, bool disp, int num);
static void Disp_Temp(SGUI_SCR_DEV *pstDeviceIF, bool refresh, int temp, int humi);
static void diso_ip_current(bool drawBase, int data);
static void DrawModeIcon(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 mode);

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

extern const unsigned char gImage_image_temp[64];
extern const unsigned char gImage_image_input[64];
extern const unsigned char gImage_image_manual[48];
extern const unsigned char gImage_image_mode_auto[57];
extern const unsigned char gImage_image_485[32];
extern const unsigned char gImage_image_mode_move[48];

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    pstDeviceIF->fnClear(); /*清屏*/
    adjust_state = 0;
    adjust_number = get_targetValuePercent();
    Port_SetInt(DATA_I_MANUAL_CMD, 0);   /* 进页面先停气，避免上次手动残留指令 */

    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
    SysTickTimerEnable(true);                          /*启动定时器*/

    appManage_switchTask(APP_TASK_AUTO_CONTROL);
    return HMI_RET_NORMAL;
}

/*绘制页面*/
HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    /*阀位显示*/
    SGUI_RECT rect;
    rect.iX = 30;
    rect.iY = 15;
    rect.iWidth = pstDeviceIF->stSize.iWidth - 40;
    rect.iHeight = 40;
    SGUI_Basic_DrawRoundedRectangle(pstDeviceIF, rect.iX, rect.iY, rect.iWidth, rect.iHeight, 5, 1, 1);
    SGUI_Basic_DrawRoundedRectangle(pstDeviceIF, rect.iX + 2, rect.iY + 2, rect.iWidth - 4, rect.iHeight - 4, 5, 1, 0);

    SGUI_Basic_DrawHorizontalLine(pstDeviceIF, rect.iX + 19, rect.iWidth + 11, rect.iY, SGUI_COLOR_BKGCLR);
    SGUI_Basic_DrawHorizontalLine(pstDeviceIF, rect.iX + 20, rect.iWidth + 10, rect.iY + 1, SGUI_COLOR_BKGCLR);
    SGUI_Basic_DrawHorizontalLine(pstDeviceIF, rect.iX + 19, rect.iWidth + 11, rect.iY + 2, SGUI_COLOR_BKGCLR);
    DrawBigNumber(pstDeviceIF, 0);
    HMI_DemoPolarCoord_PaintClock(pstDeviceIF, true, 0);

    /*输入框*/
    InValue_rect.iX = 20;
    InValue_rect.iY = pstDeviceIF->stSize.iHeight - 36;
    InValue_rect.iWidth = 64;
    InValue_rect.iHeight = 32;
    SGUI_Basic_DrawRoundedRectangle(pstDeviceIF,
                                    InValue_rect.iX - 2, InValue_rect.iY - 2,
                                    InValue_rect.iWidth + 2, InValue_rect.iHeight + 4,
                                    5, SGUI_COLOR_FRGCLR, SGUI_COLOR_FRGCLR);
    Draw_buff(InValue_rect.iX,
              InValue_rect.iY,
              16, 32, gImage_image_input, 1); /*输入图标*/
    DrawStr(pstDeviceIF, InValue_rect.iX + 48 + 2, InValue_rect.iY + 2, 1, STR_LEFT, "%%");
    DrawStr(pstDeviceIF, InValue_rect.iX + 48 + 1, 16 + InValue_rect.iY + 2, 1, STR_LEFT, "mA");
    Disp_InValue(pstDeviceIF, true, Port_GetInt(DATA_I_INPUT_POINT)); /*4-20mA 换算 0.0~100.0*/

    /*温度框*/
    Temp_rect.iX = 90;
    Temp_rect.iY = pstDeviceIF->stSize.iHeight - 36;
    Temp_rect.iWidth = 48;
    Temp_rect.iHeight = 32;
    SGUI_Basic_DrawRoundedRectangle(pstDeviceIF,
                                    Temp_rect.iX - 3, Temp_rect.iY - 2,
                                    Temp_rect.iWidth + 3, Temp_rect.iHeight + 4,
                                    5, SGUI_COLOR_FRGCLR, SGUI_COLOR_FRGCLR);
    Draw_buff(Temp_rect.iX,
              Temp_rect.iY,
              16, 32, gImage_image_temp, 1); /*温度计图标*/
    Disp_Temp(pstDeviceIF, true, Port_GetFloat(DATA_F_TEMP), Port_GetFloat(DATA_F_HUMI));
    Draw_buff(Temp_rect.iX + 4 + 32, Temp_rect.iY, 8, 16, FontAscii_w8h16[95], 1);     /*℃*/
    Draw_buff(Temp_rect.iX + 4 + 32, Temp_rect.iY + 16, 8, 16, FontAscii_w8h16[5], 1); /*%*/

    /*震动和噪声,目前只画了框，并没有填入数据*/
    sensor_rect.iX = 144;
    sensor_rect.iY = pstDeviceIF->stSize.iHeight - 36;
    sensor_rect.iWidth = 48;
    sensor_rect.iHeight = 32;
    SGUI_Basic_DrawRoundedRectangle(pstDeviceIF,
                                    sensor_rect.iX - 3, sensor_rect.iY - 2,
                                    sensor_rect.iWidth + 3, sensor_rect.iHeight + 4,
                                    5, SGUI_COLOR_FRGCLR, SGUI_COLOR_FRGCLR);

    diso_ip_current(true, 0);

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
    static int output_current = 0;
    static int i = 0;
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        DEBUG_LOGW(TAG, "KEY VALUE ENTER");
        static uint32_t dat = GUI_ID;
        appManage_switchTask(APP_TASK_NONE);
        HMI_SwitchScreen(HMI_SCREEN_ID_GUI_COUNT_DOWN, &dat);
        break;
    case KEY_VALUE_ESC:
        GUI_OBJECT.pstPrevious = NULL;
        HMI_SwitchScreen(HMI_SCREEN_ID_AUTO2, NULL);
        break;
    case KEY_VALUE_UP:
        if (get_inpu_mode() == CONTROL_MODE_BUTTON_PERCENT)
        {
            adjust_number += 1;  /* 每拍 ±0.1%（0~1000 刻度），长按连发快速调节 */
            if (adjust_number > 1000)
                adjust_number = 1000;
            adjust_state = 15;
        }
        break;
    case KEY_VALUE_DOWN:
        if (get_inpu_mode() == CONTROL_MODE_BUTTON_PERCENT)
        {
            adjust_number -= 1;  /* 每拍 ±0.1% */
            if (adjust_number < 0)
                adjust_number = 0;
            adjust_state = 15;
        }
        break;
    case KEY_VALUE_NONE:
//        Port_SetInt(DATA_I_OUTPUT_CURRENT, get_current_data()->mid);
        DEBUG_LOGE(TAG, "KEY VALUE NONE");
        break;
    default:
        break;
    }
    return iProcessAction;
}

/*定时事件*/
static HMI_ENGINE_RESULT GUI_Time_Event(SGUI_SCR_DEV *pstDeviceIF)
{
    static int time = 0;
    static int flash = 0;
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;

    float32_t angle = Port_GetFloat(DATA_F_ANGLE);//从port获取角度

    /*范围限制*/ 
    angle = scope(angle, -1000, 9999);
    HMI_DemoPolarCoord_PaintClock(pstDeviceIF, false, angle);
    /*显示角度*/
    DrawBigNumber(pstDeviceIF, angle);

    /*输入目标比列*/
    if (get_inpu_mode() == CONTROL_MODE_BUTTON_PERCENT)
    {
        if (adjust_state > 0)
        {
            adjust_state--;
            if (adjust_state == 0)
            {
                set_targetValuePercent(adjust_number); /*停止按键后生效*/
            }
            flash++;
            Disp_InValue(pstDeviceIF, (flash % 3), adjust_number); /*调整中闪烁*/
        }
        else
        {
            flash = 0;
            Disp_InValue(pstDeviceIF, true, adjust_number);
            if (adjust_number != get_targetValuePercent())
                adjust_number = get_targetValuePercent();
        }
    }
    else if (get_inpu_mode() == CONTROL_MODE_BUTTON_MOVE)
    {
        /* 纯手动（按住才动）：定时事件（50ms）直接采样按键电平，两拍确认消抖——
         * 电平指令连续两拍一致才写端口：抖动期电平 0/1 弹跳永远不满足"连续一致"，
         * 指令保持不动（滤抖）；稳定按住/松开后 ≤100ms 内生效。
         * 实际驱动由 ValveControl 在控制任务内执行，本层只发指令 */
        static int s_raw_last = 0;   /* 上一拍采到的电平指令 */
        int raw = 0;

        /* 输入框仍显示 4-20mA 换算百分比（setpoint 跟随电流，供监控） */
        Disp_InValue(pstDeviceIF, true, Port_GetInt(DATA_I_INPUT_POINT));

        if (Gpio_GetInputIO(KEY_UP_PIN) == 1)
        {
            raw = 1;    /* 按住上行：满幅充气 */
        }
        else if (Gpio_GetInputIO(KEY_DOWN_PIN) == 1)
        {
            raw = -1;   /* 按住下行：满幅排气 */
        }

        if (raw == s_raw_last)
        {
            Port_SetInt(DATA_I_MANUAL_CMD, raw);   /* 连续两拍一致才切换/保持指令 */
        }
        s_raw_last = raw;
    }
    else
    {
        /* 电流/485/HART：输入框显示 4-20mA 换算值（ControlTask 周期发布） */
        Disp_InValue(pstDeviceIF, true, Port_GetInt(DATA_I_INPUT_POINT));
    }

    /*IP电流*/
//    int32_t ip_current;
//    if (Port_Get((void *)&ip_current, DATA_I_IP_CURRENT))
//    {
//        if ((ip_current > 0) && (ip_current < 10))
//            ip_current = 10;
//        else if ((ip_current < 0) && (ip_current > -10))
//            ip_current = -10;
//        diso_ip_current(false, ip_current / 10);
//    }

    /*温度湿度*/
    Disp_Temp(pstDeviceIF, false, Port_GetFloat(DATA_F_TEMP), Port_GetFloat(DATA_F_HUMI));

    /*模式显示*/
    DrawModeIcon(pstDeviceIF, get_inpu_mode());

    return iProcessAction;
}

static void HMI_DemoPolarCoord_PaintClock(SGUI_SCR_DEV *pstDeviceIF, bool DrawBase, int scale)
{

    SGUI_RECT stArea;
    SGUI_POINT stInnerPos = {0, 0};
    SGUI_BMP_RES stCharBitmap = {5, 3, (const SGUI_BYTE *)&image};
    static SGUI_UINT16 NextPort = 0xffff;

    int ix = 56;

    stArea.iX = NextPort;
    stArea.iY = SCALE_Y - 3;
    stArea.iWidth = 5;
    stArea.iHeight = 3;

    scale = scope(scale, 0, 1000) / 10;

    if (DrawBase)
    {
        /*绘制刻度条*/
        SGUI_Basic_DrawHorizontalLine(pstDeviceIF, ix, ix + 100, SCALE_Y, SGUI_COLOR_FRGCLR);
        int x = 0;
        for (int i = 0; i < 5; i++)
        {
            x = (i * 25) + (ix);
            SGUI_Basic_DrawLine(pstDeviceIF, x, SCALE_Y + 1, x, SCALE_Y + 2, SGUI_COLOR_FRGCLR);
        }

        /*刻度数字*/
        DrawScaleMark(pstDeviceIF, ix - 1, SCALE_Y + 4, "0");
        DrawScaleMark(pstDeviceIF, ix + 25 - 3, SCALE_Y + 4, "25");
        DrawScaleMark(pstDeviceIF, ix + 50 - 3, SCALE_Y + 4, "50");
        DrawScaleMark(pstDeviceIF, ix + 75 - 3, SCALE_Y + 4, "75");
        DrawScaleMark(pstDeviceIF, ix + 100 - 5, SCALE_Y + 4, "100");
    }

    /*清除一下*/
    if (NextPort != 0xffff)
        SGUI_Basic_FillRectangleArea(pstDeviceIF, stArea.iX, stArea.iY, stArea.iWidth, stArea.iHeight, SGUI_COLOR_BKGCLR);

    /*画新的*/
    NextPort = (ix - 2) + scale;
    stArea.iX = NextPort;
    SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &stCharBitmap, SGUI_DRAW_NORMAL);
}

static void DrawModeIcon(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 mode)
{
    static SGUI_UINT8 flash = 0;
    static uint32_t time = 0;
    time++;
    if (time % 5 == 0)
    {
        if (flash == 0)
        {
            flash = 1;
            Draw_buff_Byte(9, 8, 19, 24, 0x00);
        }
        else
        {
            flash = 0;
            if (mode == CONTROL_MODE_CURRENT_PERCENT)
                Draw_buff(9, 8, 19, 24, gImage_image_mode_auto, 0);
            else if (mode == CONTROL_MODE_BUTTON_PERCENT)
                Draw_buff(10, 8, 16, 24, gImage_image_manual, 0);
            else if (mode == CONTROL_MODE_RS485_PERCENT)
                Draw_buff(10, 8, 16, 16, gImage_image_485, 0);
            else if (mode == CONTROL_MODE_HART_PERCENT)
                Draw_buff(10, 8, 16, 16, gImage_image_485, 0);
            else if (mode == CONTROL_MODE_BUTTON_MOVE)
                Draw_buff(10, 8, 16, 24, gImage_image_mode_move, 0);
        }
    }
}

static void DrawScaleMark(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, SGUI_SZSTR srt)
{
    SGUI_RECT stDisplayArea;
    SGUI_POINT stInnerPos;
    stInnerPos.iX = 0;
    stInnerPos.iY = 0;

    stDisplayArea.iX = x;
    stDisplayArea.iY = y;
    stDisplayArea.iWidth = 3 * SGUI_DEFAULT_FONT_MiniNum.iHalfWidth;
    stDisplayArea.iHeight = SGUI_DEFAULT_FONT_MiniNum.iHeight + 1;
    SGUI_Text_DrawText(pstDeviceIF, srt, &SGUI_DEFAULT_FONT_MiniNum, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);
}

static void DrawBigNumber(SGUI_SCR_DEV *pstDeviceIF, float32_t num)
{
    static uint8_t clear_flage;
    static uint8_t old_clear_flage;
    int mid = (pstDeviceIF->stSize.iWidth / 2) + 10;
    int font_width = 24 / 2;
    const uint8_t number_y_spot = 5;
    const uint8_t radix_point_width = 4;
    const uint8_t radix_point[] = {0xF0, 0xF0, 0xF0, 0xF0};

    /*擦除数字区域*/
    if (clear_flage != old_clear_flage)
    {
        old_clear_flage = clear_flage;
        Draw_buff_Byte((mid - font_width) - 24 - radix_point_width, number_y_spot, 80, 24, 0x00);
    }

#if 1
    /*绘制数字*/
    if (num >= 0)
    {
        int disp_num = num;
        if (disp_num < 100)
        {
            clear_flage = 1;
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2) - font_width, number_y_spot, disp_num / 10, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width - font_width, number_y_spot, disp_num % 10, 0);
            Draw_buff((mid + font_width) - (radix_point_width / 2) - font_width, number_y_spot + 16, 4, 8, radix_point, 0); /*小数点*/
        }
        else if (disp_num < 1000)
        {
            clear_flage = 2;
            Draw_Number_24x24((mid - font_width) - 24 - radix_point_width, number_y_spot, disp_num / 100, 0);
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2), number_y_spot, disp_num % 100 / 10, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width, number_y_spot, disp_num % 10, 0);
            Draw_buff((mid + font_width) - (radix_point_width / 2), number_y_spot + 16, 4, 8, radix_point, 0); /*小数点*/
        }
        else if (disp_num < 10000)
        {
            clear_flage = 3;
            Draw_Number_24x24((mid - font_width) - 24 - radix_point_width, number_y_spot, disp_num / 1000, 0);
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2), number_y_spot, disp_num % 1000 / 100, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width, number_y_spot, disp_num % 100 / 10, 0);
        }
        else
        {
            clear_flage = 4;
            Draw_Number_24x24((mid - font_width) - 24 - radix_point_width, number_y_spot, 9, 0);
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2), number_y_spot, 9, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width, number_y_spot, 9, 0);
        }
    }
    else
    {
        int disp_num = to_abs(num);
        if (disp_num < 100)
        {
            clear_flage = 5;
            Draw_Number_24x24((mid - font_width) - 24 - radix_point_width, number_y_spot, 10, 0);
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2), number_y_spot, disp_num % 100 / 10, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width, number_y_spot, disp_num % 10, 0);
            Draw_buff((mid + font_width) - (radix_point_width / 2), number_y_spot + 16, 4, 8, radix_point, 0); /*小数点*/
        }
        else if (disp_num < 1000)
        {
            clear_flage = 6;
            Draw_Number_24x24((mid - font_width) - 24 - radix_point_width, number_y_spot, 10, 0);
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2), number_y_spot, disp_num % 1000 / 100, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width, number_y_spot, disp_num % 100 / 10, 0);
        }
        else
        {
            clear_flage = 7;
            Draw_Number_24x24((mid - font_width) - 24 - radix_point_width, number_y_spot, 10, 0);
            Draw_Number_24x24((mid - font_width) - (radix_point_width / 2), number_y_spot, 9, 0);
            Draw_Number_24x24((mid + font_width) + radix_point_width, number_y_spot, 9, 0);
        }
    }
#endif

    /*小百分号*/
    SGUI_RECT stDisplayArea;
    SGUI_POINT stInnerPos;
    stInnerPos.iX = 0;
    stInnerPos.iY = 0;

    stDisplayArea.iX = mid + font_width * 3 + radix_point_width;
    stDisplayArea.iY = number_y_spot + 15;
    stDisplayArea.iWidth = SGUI_DEFAULT_FONT_12.iHalfWidth;
    stDisplayArea.iHeight = SGUI_DEFAULT_FONT_12.iHeight;
    SGUI_Text_DrawText(pstDeviceIF, "%", &SGUI_DEFAULT_FONT_12, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);
}

static void Disp_InValue(SGUI_SCR_DEV *pstDeviceIF, bool disp, int num)
{
#define zeof_add ('0' - ' ')
    int x = InValue_rect.iX;
    int y = InValue_rect.iY;
    const uint8_t color = 1;
    if (disp)
    {
        if (num <= 999)
        {
            Draw_buff(x + 16, y, 8, 16, FontAscii_w8h16[zeof_add + (num % 1000 / 100)], color);
            Draw_buff(x + 16 + 8, y, 8, 16, FontAscii_w8h16[zeof_add + (num % 100 / 10)], color);
            Draw_buff(x + 16 + 16, y, 8, 16, FontAscii_w8h16[14], color);
            Draw_buff(x + 16 + 24, y, 8, 16, FontAscii_w8h16[zeof_add + (num % 10)], color);
        }
        else if (num <= 9999)
        {
            Draw_buff_Byte(x + 16, y, 4, 16, color ? 0xff : 0);
            Draw_buff(x + 16 + 4, y, 8, 16, FontAscii_w8h16[zeof_add + (num % 10000 / 1000)], color);
            Draw_buff(x + 16 + 8 + 4, y, 8, 16, FontAscii_w8h16[zeof_add + (num % 1000 / 100)], color);
            Draw_buff(x + 16 + 16 + 4, y, 8, 16, FontAscii_w8h16[zeof_add + (num % 100 / 10)], color);
            Draw_buff_Byte(x + 16 + 24 + 4, y, 4, 16, color ? 0xff : 0);
        }
        else
        {
            Draw_buff_Byte(x + 16, y, 4, 16, color ? 0xff : 0);
            Draw_buff(x + 16 + 4, y, 8, 16, FontAscii_w8h16['-' - ' '], color);
            Draw_buff(x + 16 + 8 + 4, y, 8, 16, FontAscii_w8h16['-' - ' '], color);
            Draw_buff(x + 16 + 16 + 4, y, 8, 16, FontAscii_w8h16['-' - ' '], color);
            Draw_buff_Byte(x + 16 + 24 + 4, y, 4, 16, color ? 0xff : 0);
        }
    }
    else
    {
        Draw_buff(x + 16, y, 8, 16, FontAscii_w8h16[0], color);
        Draw_buff(x + 16 + 8, y, 8, 16, FontAscii_w8h16[0], color);
        Draw_buff(x + 16 + 16, y, 8, 16, FontAscii_w8h16[0], color);
        Draw_buff(x + 16 + 24, y, 8, 16, FontAscii_w8h16[0], color);
    }

    /*输入电流显示*/
    num = Port_GetFloat(DATA_F_INPUT_CURRENT) * 10;
    Draw_buff(x + 16, y + 16, 8, 16, FontAscii_w8h16[zeof_add + (num % 1000 / 100)], color);
    Draw_buff(x + 16 + 8, y + 16, 8, 16, FontAscii_w8h16[zeof_add + (num % 100 / 10)], color);
    Draw_buff(x + 16 + 16, y + 16, 8, 16, FontAscii_w8h16[14], color);
    Draw_buff(x + 16 + 24, y + 16, 8, 16, FontAscii_w8h16[zeof_add + (num % 10)], color);
}

static void Disp_Temp(SGUI_SCR_DEV *pstDeviceIF, bool refresh, int temp, int humi)
{
#define zeof_add ('0' - ' ')
    static int old_temp = 0, old_humi = 0;
    const uint8_t color = 1;
    if ((old_temp != temp) || refresh)
    {
        old_temp = temp;
        temp = (temp > 99 ? 99 : (temp < -9 ? -9 : temp));
        if (temp < 0)
        {
            temp = -temp;
            Draw_buff(Temp_rect.iX + 2 + 16, Temp_rect.iY, 8, 16, FontAscii_w8h16[13], color); //"-"
        }
        else
            Draw_buff(Temp_rect.iX + 2 + 16, Temp_rect.iY, 8, 16, FontAscii_w8h16[zeof_add + (temp % 100 / 10)], color);

        Draw_buff(Temp_rect.iX + 2 + 24, Temp_rect.iY, 8, 16, FontAscii_w8h16[zeof_add + (temp % 10)], color);
    }

    if ((old_humi != humi) || refresh)
    {
        old_humi = humi;
        humi = (humi > 99 ? 99 : (humi < 0 ? 0 : humi));
        Draw_buff(Temp_rect.iX + 2 + 16, Temp_rect.iY + 16, 8, 16, FontAscii_w8h16[zeof_add + (humi % 100 / 10)], color);
        Draw_buff(Temp_rect.iX + 2 + 24, Temp_rect.iY + 16, 8, 16, FontAscii_w8h16[zeof_add + (humi % 10)], color);
    }
}

static void diso_ip_current(bool drawBase, int data)
{
    static int old_data = 0;
    if (drawBase)
    {
        /*中心刻度*/
        Draw_buff_Byte(0, 48, 1, 8, 0x01);
        Draw_buff_Byte(0, 40, 1, 8, 0x80);

        /*左右边*/
        Draw_buff_Byte(1, 8, 1, 80, 0xff);
        Draw_buff_Byte(5, 8, 1, 80, 0xff);

        /*上下边*/
        Draw_buff_Byte(0, 0, 6, 8, 0x80);
        Draw_buff_Byte(0, 88, 6, 8, 0x01);
    }
    if (old_data != data)
    {
        old_data = data;

        Draw_buff_Byte(2, 48, 3, 40, 0x00);
        Draw_buff_Byte(2, 8, 3, 40, 0x00);

        if (data > 0)
        {
            Draw_buff_Byte(2, 40, 3, 8, data < 8 ? 0xFF << (8 - data) : 0xFF);
            Draw_buff_Byte(2, 32, 3, 8, data < 8 ? 0x00 : (data < 16 ? 0xFF << (8 - (data % 8)) : 0xFF));
            Draw_buff_Byte(2, 24, 3, 8, data < 16 ? 0x00 : (data < 24 ? 0xFF << (8 - (data % 8)) : 0xFF));
            Draw_buff_Byte(2, 16, 3, 8, data < 24 ? 0x00 : (data < 32 ? 0xFF << (8 - (data % 8)) : 0xFF));
            Draw_buff_Byte(2, 8, 3, 8, data < 32 ? 0x00 : (data < 40 ? 0xFF << (8 - (data % 8)) : 0xFF));
        }
        else
        {
            data = -data;
            Draw_buff_Byte(2, 48, 3, 8, data < 8 ? 0xFF >> (8 - data) : 0xFF);
            Draw_buff_Byte(2, 56, 3, 8, data < 8 ? 0x00 : (data < 16 ? 0xFF >> (8 - (data % 8)) : 0xFF));
            Draw_buff_Byte(2, 64, 3, 8, data < 16 ? 0x00 : (data < 24 ? 0xFF >> (8 - (data % 8)) : 0xFF));
            Draw_buff_Byte(2, 72, 3, 8, data < 24 ? 0x00 : (data < 32 ? 0xFF >> (8 - (data % 8)) : 0xFF));
            Draw_buff_Byte(2, 80, 3, 8, data < 32 ? 0x00 : (data < 40 ? 0xFF >> (8 - (data % 8)) : 0xFF));
        }
    }
}
