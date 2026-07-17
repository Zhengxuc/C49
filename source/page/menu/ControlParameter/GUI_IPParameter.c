#include "GUIPublic.h"
#include "Box.h"
#include "Save.h"

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_IP_PARAMETER

/*唯一对象名*/
#define GUI_OBJECT g_stGUIIPParameter

/*日志标识*/
static char *TAG = "IP Parameter";

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
static float32_t add_gas_min_value;
static float32_t out_gas_min_value;
static float32_t add_gas_max_value;
static float32_t out_gas_max_value;
static float32_t mid_gas_value;

enum
{
    ID_ADD_GAS_MIN,
    ID_OUT_GAS_MIN,
    ID_ADD_GAS_MAX,
    ID_OUT_GAS_MAX,
    ID_MID_GAS,

    ID_COUNT,
};

static SGUI_ITEMS_ITEM ListItems[ID_COUNT] = {
    {"Inflation minimum", NULL}, // 充气最小值
    {"Exhaust minimum", NULL},   // 排气最小值
    {"Inflation maximum", NULL}, // 充气最大值
    {"Exhaust maximum", NULL},   // 排气最大值
    {"Stable value", NULL},      // 稳定值
};

/*参数初始化*/
HMI_ENGINE_RESULT HMI_GUI_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

/*进入页面*/
HMI_ENGINE_RESULT HMI_GUI_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
//    current_t *current = get_current_data();
//    add_gas_min_value = (float32_t)current->min_AddGas;
//    out_gas_min_value = (float32_t)current->min_outGas;
//    add_gas_max_value = (float32_t)current->max_AddGas;
//    out_gas_max_value = (float32_t)current->max_outGas;
//    mid_gas_value = (float32_t)current->mid;

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
    SGUI_ITEMS_ITEM *itemTxt = (SGUI_ITEMS_ITEM *)pstParameters;
    SGUI_List_Initialize(&AutoSetListObject, &stListLayout, &UFT8_FZXS12, tr(itemTxt->cszLabelText), ListItems, list_count);

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
            SetValueBoxRepaint();
        else
            HMI_GUI_RefreshScreen(pstDeviceIF, NULL);
        return HMI_RET_NORMAL;
    case BOX_YES:
//        if (select == ID_ADD_GAS_MIN)
//            get_current_data()->min_AddGas = add_gas_min_value;
//        else if (select == ID_OUT_GAS_MIN)
//            get_current_data()->min_outGas = out_gas_min_value;
//        else if (select == ID_ADD_GAS_MAX)
//            get_current_data()->max_AddGas = add_gas_max_value;
//        else if (select == ID_OUT_GAS_MAX)
//            get_current_data()->max_outGas = out_gas_max_value;
//        else if (select == ID_MID_GAS)
//            get_current_data()->mid = mid_gas_value;

//        Save_System_block(SAVE_ADJUST_CURRENT, get_current_data(), sizeof(current_t));

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
    switch (uiKeyValue)
    {
    case KEY_VALUE_ENTER:
        if (select == ID_ADD_GAS_MIN)
        {
            SetValueBoxArea(0, 4096, 1);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &add_gas_min_value);
        }
        else if (select == ID_OUT_GAS_MIN)
        {
            SetValueBoxArea(0, 4096, 1);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &out_gas_min_value);
        }
        else if (select == ID_ADD_GAS_MAX)
        {
            SetValueBoxArea(0, 4096, 1);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &add_gas_max_value);
        }
        else if (select == ID_OUT_GAS_MAX)
        {
            SetValueBoxArea(0, 4096, 1);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &out_gas_max_value);
        }
        else if (select == ID_MID_GAS)
        {
            SetValueBoxArea(0, 4096, 1);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &mid_gas_value);
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
