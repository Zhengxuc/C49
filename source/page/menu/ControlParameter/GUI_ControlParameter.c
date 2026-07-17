#include "GUIPublic.h"
#include "Box.h"
#include "Save.h"

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
static SGUI_LIST AutoSetListObject;

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
};

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

    /*菜单列表*/
    SGUI_RECT stListLayout;
    SGUI_SystemIF_MemorySet(&AutoSetListObject, 0x00, sizeof(SGUI_LIST));
    stListLayout.iX = 0;
    stListLayout.iY = 0;
    stListLayout.iWidth = pstDeviceIF->stSize.iWidth;
    stListLayout.iHeight = pstDeviceIF->stSize.iHeight;
    SGUI_List_Initialize(&AutoSetListObject, &stListLayout, &UFT8_FZXS12, tr("5.Control parameter"), ListItems, list_count);

    ActivedObject = &AutoSetListObject;

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
    static float32_t dat = 0;
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
