/*
 * @Description: 5.Control parameter（第 5 页）—— PID 控制参数整定
 *   分上行程（充气）/ 下行程（排气）两套 Kp/Ki/Kd：
 *   - 弹框按实际增益调节（Kp 0.1~50.0 步长 0.1，Ki/Kd 0~5.00 步长 0.01）
 *   - 确认后弹出保存框，"是"才写 Flash（缩放整型：Kp×10、Ki/Kd×100）
 *     并热更新 ValveParam —— ValveControl_Run 每拍按误差符号装载增益，
 *     无需重启、无需复位 PID
 */
#include "GUIPublic.h"
#include "Box.h"
#include "Save.h"
#include "ValveParam.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_CONTROLPARAMETER

/*唯一对象名*/
#define GUI_OBJECT g_stGUIControlParameter

/*日志标识*/
static char *TAG = "ControlParameter";

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

static SGUI_LIST *ActivedObject;
static SGUI_LIST ControlListObject;

enum
{
    ID_IP_PARAMETER,
    ID_TRAVEL_PARAMETER,
    ID_ADD_GAS_SPEED_AVERAGE,
    ID_OUT_GAS_SPEED_AVERAGE,
    ID_ADD_GAS_SPEED_RATIO,
    ID_OUT_GAS_SPEED_RATIO,
    ID_ADD_GAS_OVER_DISTANCE,
    ID_OUT_GAS_OVER_DISTANCE,
    ID_ADD_GAS_DEAD_ZONE,
    ID_OUT_GAS_DEAD_ZONE,
    ID_KP_UP,
    ID_KI_UP,
    ID_KD_UP,
    ID_KP_DOWN,
    ID_KI_DOWN,
    ID_KD_DOWN,
    ID_COUNT,
};

static SGUI_ITEMS_ITEM ListItems[ID_COUNT] = {
    {"IP Parameter", NULL},                 // IP参数
    {"Travel Parameter", NULL},             // 行程参数
    {"Average inflation velocity", NULL},   // 充气速度平均值
    {"Average exhaust velocity", NULL},     // 排气速度平均值
    {"Inflation control column", NULL},     // 充气控制比列
    {"Exhaust control column", NULL},       // 排气控制比列
    {"Inflation overshoot distance", NULL}, // 充气过冲距离
    {"Exhaust overshoot distance", NULL},   // 排气过冲距离
    {"Inflation dead band", NULL},          // 充气死区
    {"Exhaust dead band", NULL},            // 排气死区
    {"Kp up", NULL},                        // 上行 Kp
    {"Ki up", NULL},                        // 上行 Ki
    {"Kd up", NULL},                        // 上行 Kd
    {"Kp down", NULL},                      // 下行 Kp
    {"Ki down", NULL},                      // 下行 Ki
    {"Kd down", NULL},                      // 下行 Kd
};

/* 弹框绑定变量（实际增益；存储为缩放整型：Kp×10、Ki/Kd×100） */
static float32_t val_kp_up = 5.0f;
static float32_t val_ki_up = 0.05f;
static float32_t val_kd_up = 0.0f;
static float32_t val_kp_down = 5.0f;
static float32_t val_ki_down = 0.05f;
static float32_t val_kd_down = 0.0f;

/*参数初始化*/
HMI_ENGINE_RESULT
HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    pstDeviceIF->fnClear(); /*清屏*/

    /*语言翻译*/
    int list_count = sizeof(ListItems) / sizeof(SGUI_ITEMS_ITEM);
    for (int i = 0; i < list_count; i++)
    {
        ListItems[i].szVariableText = (SGUI_SZSTR)tr(ListItems[i].cszLabelText);
    }

    /*装载当前保存的 PID 参数（存储为缩放整型，除回来显示实际增益）*/
    val_kp_up   = (float32_t)Read_System(SAVE_CONTROL_KP)      / PID_KP_SCALE;
    val_ki_up   = (float32_t)Read_System(SAVE_CONTROL_KI)      / PID_KI_SCALE;
    val_kd_up   = (float32_t)Read_System(SAVE_CONTROL_KD)      / PID_KD_SCALE;
    val_kp_down = (float32_t)Read_System(SAVE_CONTROL_KP_DOWN) / PID_KP_SCALE;
    val_ki_down = (float32_t)Read_System(SAVE_CONTROL_KI_DOWN) / PID_KI_SCALE;
    val_kd_down = (float32_t)Read_System(SAVE_CONTROL_KD_DOWN) / PID_KD_SCALE;

    /*菜单列表*/
    SGUI_RECT stListLayout;
    SGUI_SystemIF_MemorySet(&ControlListObject, 0x00, sizeof(SGUI_LIST));
    stListLayout.iX = 0;
    stListLayout.iY = 0;
    stListLayout.iWidth = pstDeviceIF->stSize.iWidth;
    stListLayout.iHeight = pstDeviceIF->stSize.iHeight;
    SGUI_List_Initialize(&ControlListObject, &stListLayout, &UFT8_FZXS12, tr("5.Control parameter"), ListItems, list_count);

    ActivedObject = &ControlListObject;

    if (pstParameters != NULL)
        SGUI_List_Selecte(ActivedObject, (int)pstParameters);

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
        if ((select >= ID_ADD_GAS_SPEED_AVERAGE) && (select <= ID_OUT_GAS_SPEED_RATIO))
        {
//            Save_System_block(SAVE_ADJUST_SPEED, (uint8_t *)get_adj_speed(), sizeof(adj_speed_t));
        }
        /* PID 六项确认保存：写 Flash（缩放整型）并热更新 ValveParam */
        {
            valve_param_t *p = ValveParam_Get();
            switch (select)
            {
            case ID_KP_UP:
                Save_System(SAVE_CONTROL_KP, (int32_t)(val_kp_up * PID_KP_SCALE + 0.5f));
                p->kp = val_kp_up;
                break;
            case ID_KI_UP:
                Save_System(SAVE_CONTROL_KI, (int32_t)(val_ki_up * PID_KI_SCALE + 0.5f));
                p->ki = val_ki_up;
                break;
            case ID_KD_UP:
                Save_System(SAVE_CONTROL_KD, (int32_t)(val_kd_up * PID_KD_SCALE + 0.5f));
                p->kd = val_kd_up;
                break;
            case ID_KP_DOWN:
                Save_System(SAVE_CONTROL_KP_DOWN, (int32_t)(val_kp_down * PID_KP_SCALE + 0.5f));
                p->kp_down = val_kp_down;
                break;
            case ID_KI_DOWN:
                Save_System(SAVE_CONTROL_KI_DOWN, (int32_t)(val_ki_down * PID_KI_SCALE + 0.5f));
                p->ki_down = val_ki_down;
                break;
            case ID_KD_DOWN:
                Save_System(SAVE_CONTROL_KD_DOWN, (int32_t)(val_kd_down * PID_KD_SCALE + 0.5f));
                p->kd_down = val_kd_down;
                break;
            default:
                break;
            }
            DEBUG_LOGI(TAG, "PID saved: up(%.2f %.3f %.3f) down(%.2f %.3f %.3f)",
                       p->kp, p->ki, p->kd, p->kp_down, p->ki_down, p->kd_down);
        }
        SGUI_List_Repaint(pstDeviceIF, &ControlListObject);
        if (get_setValue_Box_State() == true)
            Close_setValveBox(true);
        return HMI_RET_NORMAL;
    case BOX_NO:
        SGUI_List_Repaint(pstDeviceIF, &ControlListObject);
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
        return HMI_RET_NORMAL;
    case BOX_YES:
        Save_box(pstDeviceIF, true);
        return HMI_RET_NORMAL;
    case BOX_NO:
        if (get_setValue_Box_State() == true)
            Close_setValveBox(false);
        SGUI_List_Repaint(pstDeviceIF, ActivedObject);
        return HMI_RET_NORMAL;
    default:
        return HMI_RET_NORMAL;
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
    int select = SGUI_List_GetSelection(ActivedObject)->iIndex;
    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        if (select == ID_IP_PARAMETER)
            HMI_SwitchScreen(HMI_SCREEN_ID_IP_PARAMETER, &ListItems[select]);
        else if (select == ID_TRAVEL_PARAMETER)
            HMI_SwitchScreen(HMI_SCREEN_ID_TRAVEL_PARAMETER, &ListItems[select]);
//        else if (select == ID_ADD_GAS_SPEED_AVERAGE)
//        {
//            SetValueBoxArea(0.1, 500, 0.1);
//            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &get_adj_speed()->averageSpeedAddGas);
//        }
//        else if (select == ID_OUT_GAS_SPEED_AVERAGE)
//        {
//            SetValueBoxArea(0.1, 500, 0.1);
//            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &get_adj_speed()->averageSpeedOutGas);
//        }
//        else if (select == ID_ADD_GAS_SPEED_RATIO)
//        {
//            SetValueBoxArea(0.1, 500, 0.1);
//            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &get_adj_speed()->proportionSpeedAddGas);
//        }
//        else if (select == ID_OUT_GAS_SPEED_RATIO)
//        {
//            SetValueBoxArea(0.1, 500, 0.1);
//            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &get_adj_speed()->proportionSpeedOutGas);
//        }
//        else if (select == ID_ADD_GAS_OVER_DISTANCE)
//        {
//            SetValueBoxArea(0, get_angle_range(), 1);
//            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &get_overshoot()->dist_addGas);
//        }
//        else if (select == ID_OUT_GAS_OVER_DISTANCE)
//        {
//            SetValueBoxArea(0, get_angle_range(), 1);
//            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &get_overshoot()->dist_outGas);
//        }
        else
        {
            switch (select)
            {
        case ID_KP_UP:
            SetValueBoxArea(0.1f, 50.0f, 0.1f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_kp_up);
            break;
        case ID_KI_UP:
            SetValueBoxArea(0.0f, 5.0f, 0.01f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_ki_up);
            break;
        case ID_KD_UP:
            SetValueBoxArea(0.0f, 5.0f, 0.01f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_kd_up);
            break;
        case ID_KP_DOWN:
            SetValueBoxArea(0.1f, 50.0f, 0.1f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_kp_down);
            break;
        case ID_KI_DOWN:
            SetValueBoxArea(0.0f, 5.0f, 0.01f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_ki_down);
            break;
        case ID_KD_DOWN:
            SetValueBoxArea(0.0f, 5.0f, 0.01f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_kd_down);
            break;
        default:
            break;
            }
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
