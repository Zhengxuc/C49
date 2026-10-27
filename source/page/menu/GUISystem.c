#include "GUIPublic.h"
#include "Box.h"
#include "save.h"
#include "GUIMain.h"
#include "Disp.h"
#include "OutputDriver.h"   /* 故障模式保存后热更新 PF05 */

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_SYSTEM

/*唯一对象名*/
#define GUI_OBJECT g_stGUISystem

/*日志标识*/
static char *TAG = "GUISystem";

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

static SGUI_LIST *ActivedObject;    // 当前选中的对象
static SGUI_LIST AutoSetListObject; // 主菜单
static float32_t backlight = 0;
static float32_t contrast_ratio = 0;
static float32_t Language = 0;
static float32_t DisplayMirror = 1;
static float32_t WorkingMode = 0;
static float32_t exec_timeout = EXEC_TIMEOUT_DEFAULT;   /* 行程时间极限（秒） */

/* 安全位置（故障模式）选项：索引即 SAVE_SAFE_POSITION 存储值，
 * 0=故障安全(FAIL_SAFE, PF05=高) 1=故障闭锁(FAIL_LOCK, PF05=低)，与 drive_mode_t 一致 */
static SGUI_ITEMS_ITEM Working_Mode_Itme[] = {
    {"Fail-safe", NULL},
    {"Fail-lock", NULL},
};

static SGUI_ITEMS_ITEM Language_Itme[] = {
    {"中文", NULL},
    {"English", NULL},
};

static SGUI_ITEMS_ITEM Display_mirror[] = {
    {"Auto", NULL},
    {"Normal", NULL},
    {"Reversal", NULL},
};

enum //现在实际用了的参数索引
{
    ID_Execution_timeout,   // 行程时间
    ID_Working_mode,        // 安全位置选择
    ID_Current_calibration, // 输入电流
    ID_Language,            // 语言
    ID_Screen_brightness,   // 屏幕亮度
    ID_CONTRAST_RATIO,      // 对比度
    ID_Display_mirror,      // 屏幕镜像
    ID_Restore,             // 恢复出厂设置

    ID_COUNT,
};

static SGUI_ITEMS_ITEM ListItems[ID_COUNT] = {
    {"Execution timeout", NULL},     // 行程时间
    {"Working mode", NULL},          // 安全位置选择
    {"Current calibration", NULL},   // 输入电流
    {"Language", NULL},              // 语言
    {"Screen brightness", NULL},     // 屏幕亮度
    {"Screen Contrast ratio", NULL}, // 对比度
    {"Display mirror", NULL},        // 屏幕镜像
    {"Restore", NULL},               // 恢复出厂设置
};

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    pstDeviceIF->fnClear(); /*清屏*/

    backlight = (float)Read_System(SAVE_BACKLIGHT);
    contrast_ratio = (float)Read_System(SAVE_CONTRAST_RATIO);
    Language = (float)Read_System(SAVE_LANGUAGE);
    DisplayMirror = (float)Read_System(SAVE_DISPLAY_MIRROR);
    WorkingMode = (float)Read_System(SAVE_SAFE_POSITION);   /* 故障模式档位 0/1 */
    exec_timeout = (float)Read_System(SAVE_EXECUTION_TIMEOUT); /* 行程时间极限（秒） */

    /*语言翻译*/
    int list_count = sizeof(ListItems) / sizeof(SGUI_ITEMS_ITEM);
    for (int i = 0; i < list_count; i++)
    {
        ListItems[i].szVariableText = (SGUI_SZSTR)tr(ListItems[i].cszLabelText);
    }

    /*菜单列表*/
    SGUI_RECT stListLayout;
    SGUI_SystemIF_MemorySet(&AutoSetListObject, 0x00, sizeof(SGUI_LIST));
    stListLayout.iX = 0;
    stListLayout.iY = 0;
    stListLayout.iWidth = pstDeviceIF->stSize.iWidth;
    stListLayout.iHeight = pstDeviceIF->stSize.iHeight;
    SGUI_List_Initialize(&AutoSetListObject, &stListLayout, &UFT8_FZXS12, tr("8.System"), ListItems, list_count);

    if (pstParameters != NULL)
        SGUI_List_Selecte(ActivedObject, (int)pstParameters);

    ActivedObject = &AutoSetListObject;

    HMI_GUI_RefreshScreen(pstDeviceIF, pstParameters); /*绘制*/
    SysTickTimerEnable(true);                          /*启动定时器*/
    return HMI_RET_NORMAL;
}

/*绘制页面*/
HMI_ENGINE_RESULT HMI_GUI_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    SGUI_List_Repaint(pstDeviceIF, ActivedObject);
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
    int select = SGUI_List_GetSelection(ActivedObject)->iIndex;

    /*保存框事件*/
    switch (Save_box_ProcessEvent(pstDeviceIF, pstEvent))
    {
    case BOX_INVALID:
        break;
    case BOX_ENABLE:
        return HMI_RET_NORMAL;
    case BOX_RETURN:
        if (get_setValue_Box_State() == true)
        {
            SetValueBoxRepaint();
        }
        else
            HMI_GUI_RefreshScreen(pstDeviceIF, NULL);
        return HMI_RET_NORMAL;
    case BOX_YES:
        if (select == ID_Language) /*语言*/
        {
            if (Language == 0)
                set_language(LANGUAGE_CH);
            else if (Language == 1)
                set_language(LANGUAGE_EN);
            Save_System(SAVE_LANGUAGE, (int32_t)Language);
            HMI_GUI_Prepare(pstDeviceIF, (void *)ID_Language);
        }
        else if (select == ID_Display_mirror) /*屏幕镜像*/
        {
            Save_System(SAVE_DISPLAY_MIRROR, (int32_t)DisplayMirror);
            disp_hal_init();
            HMI_GUI_Prepare(pstDeviceIF, (void *)ID_Display_mirror);
        }
        else if (select == ID_CONTRAST_RATIO)
        {
            DEBUG_LOGI(TAG, "保存对比度");
            Save_System(SAVE_CONTRAST_RATIO, (int32_t)contrast_ratio);
        }
        else if (select == ID_Working_mode) /*安全位置（故障模式）*/
        {
            Save_System(SAVE_SAFE_POSITION, (int32_t)WorkingMode);
            /* 热更新 PF05 电平，无需重启 */
            OutputDriver_SetMode((WorkingMode == SAFE_POS_FAIL_LOCK) ? DRIVE_MODE_FAIL_LOCK
                                                                  : DRIVE_MODE_FAIL_SAFE);
            DEBUG_LOGI(TAG, "save safe position=%d", (int)WorkingMode);
        }
        else if (select == ID_Execution_timeout) /*行程时间极限*/
        {
            Save_System(SAVE_EXECUTION_TIMEOUT, (int32_t)exec_timeout);
            DEBUG_LOGI(TAG, "save execution timeout=%d s", (int)exec_timeout);
        }
        SGUI_List_Repaint(pstDeviceIF, &AutoSetListObject);
        if (get_setValue_Box_State() == true)
            Close_setValveBox(true);
        return HMI_RET_NORMAL;
    case BOX_NO:
        SGUI_List_Repaint(pstDeviceIF, &AutoSetListObject);
        if (get_setValue_Box_State() == true)
            Close_setValveBox(false);
        return HMI_RET_NORMAL;
    default:
        break;
    }

    /*数值调整框*/
    switch (SetValueBox_ProcessEvent(pstDeviceIF, pstEvent))
    {
    case BOX_INVALID:
        break;
    case BOX_ENABLE:
        break;
    case BOX_VARIATION:
        if (SGUI_List_GetSelection(&AutoSetListObject)->iIndex == ID_CONTRAST_RATIO)
        {
            lcd_set_contrast_ratio(contrast_ratio);
        }
        return HMI_RET_NORMAL;
    case BOX_YES:
        Save_box(pstDeviceIF, true);
        return HMI_RET_NORMAL;
    case BOX_NO:
        if (get_setValue_Box_State() == true)
            Close_setValveBox(false);
        SGUI_List_Repaint(pstDeviceIF, ActivedObject);
    default:
        return HMI_RET_NORMAL;
        break;
    }

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
    static int old_pwm = 0;
    if (HMI_PROCESS_SUCCESSFUL(eProcResult))
    {
        if (HMI_DEMO_PROC_CANCEL == iActionID)
        {
            HMI_GoBack(NULL);
        }
    }
    // UserTime2SetPwm(backlight);

    // if (old_pwm != backlight)
    // {
    //     old_pwm = backlight;
    //     Save_System(SAVE_BACKLIGHT, backlight);
    // }

    return HMI_RET_NORMAL;
}

/*按键事件*/
static HMI_ENGINE_RESULT GUI_Key_Event(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT16 uiKeyValue)
{
    SGUI_INT iProcessAction = HMI_DEMO_PROC_NO_ACT;
    int select = SGUI_List_GetSelection(ActivedObject)->iIndex;

    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        if (select == ID_Working_mode)
        {
            SelectValueBox(pstDeviceIF, SGUI_List_GetSelection(ActivedObject)->pstItem->cszLabelText,
                           Working_Mode_Itme, get_ItemsCount(Working_Mode_Itme), &WorkingMode);
        }
        else if (select == ID_Language)
        {
            SelectValueBox(pstDeviceIF, SGUI_List_GetSelection(ActivedObject)->pstItem->cszLabelText,
                           Language_Itme, get_ItemsCount(Language_Itme), &Language);
        }
        else if (select == ID_Display_mirror)
        {
            SelectValueBox(pstDeviceIF, SGUI_List_GetSelection(ActivedObject)->pstItem->cszLabelText,
                           Display_mirror, get_ItemsCount(Display_mirror), &DisplayMirror);
        }
        else if (select == ID_Execution_timeout)
        {
            SetValueBoxArea(EXEC_TIMEOUT_MIN, EXEC_TIMEOUT_MAX, 1);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &exec_timeout);
        }
        else if (select == ID_Screen_brightness)
        {
            SetValueBoxArea(0.0f, 100.0f, 10.0f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &backlight);
        }
        else if (select == ID_CONTRAST_RATIO)
        {
            SetValueBoxArea(1.0f, 30.0f, 1.0f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &contrast_ratio);
        }
        else if (select == ID_Current_calibration)
        {
            HMI_SwitchScreen(HMI_SCREEN_ID_GUI_CURRENT_CALIB, NULL);
        }

        break;
    case KEY_VALUE_ESC:
        iProcessAction = HMI_DEMO_PROC_CANCEL;
        break;
    case KEY_VALUE_UP:
        if (SGUI_List_GetSelection(ActivedObject)->iIndex < (ActivedObject->stItems.iCount - 1))
            SGUI_List_Selecte(ActivedObject, SGUI_List_GetSelection(ActivedObject)->iIndex + 1);
        SGUI_List_Repaint(pstDeviceIF, ActivedObject);
        break;
    case KEY_VALUE_DOWN:
        if (SGUI_List_GetSelection(ActivedObject)->iIndex > 0)
            SGUI_List_Selecte(ActivedObject, SGUI_List_GetSelection(ActivedObject)->iIndex - 1);
        SGUI_List_Repaint(pstDeviceIF, ActivedObject);
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
