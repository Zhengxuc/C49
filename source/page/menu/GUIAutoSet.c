#include "GUIPublic.h"
#include "Box.h"
#include "Save.h"
#include "ValveParam.h"    /* 保存后热更新 deadband 用 */

/*页面唯一ID*/
#define GUI_ID HMI_SCREEN_ID_AUTO_SET

/*唯一对象名*/
#define GUI_OBJECT g_stGUIMAutoSet

/*日志标识*/
static char *TAG = "AutoSet";

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

/* 弹框绑定的本地变量（进入页面时从 Flash RAM 缓存装载，保存时写回） */
static float32_t val_actuator;   /* 执行器类型档位 0/1 */
static float32_t val_balance;    /* 性能平衡档位 0~4 */
static float32_t val_error_pct;  /* 冗余误差，单位 %（0.3~10.0）；存储为 ×10 整型 3~100 */
static float32_t val_friction;   /* 摩擦系数档位 0/1 */

enum
{
    ID_ACTUATOR_TYPE,   // 执行器类型
    ID_BALANCE,         // 性能平衡
    ID_REDUNDANT_ERROR, // 冗余误差
    ID_FRICTIONMETER,   // 摩擦系数

    ID_COUNT,
};

static SGUI_ITEMS_ITEM ListItems[ID_COUNT] = {
    {"Actuator type", NULL},
    {"Balance", NULL},
    {"Redundant error", NULL},
    {"frictionmeter", NULL},   /* 翻译键，中文显示为"阀门摩擦系数" */
};

/* 执行器类型选项：索引即 SAVE_ACTUATOR_TYPE 存储值（中文显示"直行程LT"/"角行程ROTS"） */
static SGUI_ITEMS_ITEM actuator_type_item[] = {
    {"Line Mode", NULL},   /* 0 = LT 直行程 */
    {"Angle Mode", NULL},  /* 1 = ROTS 角行程 */
};

/* 性能平衡选项：索引即 SAVE_BALANCE_LEVEL 存储值（原 frictionmeter_Itme 命名有误，实为平衡档） */
static SGUI_ITEMS_ITEM balance_item[] = {
    {"MAX Speed", NULL},       /* 0 最快速度 */
    {"Priority Speed", NULL},  /* 1 速度优先 */
    {"Default", NULL},         /* 2 适中 */
    {"Priority Stable", NULL}, /* 3 稳定优先 */
    {"MAX Stable", NULL},      /* 4 最佳稳定 */
};

/* 摩擦系数选项：索引即 SAVE_FRICTION_LEVEL 存储值 */
static SGUI_ITEMS_ITEM friction_item[] = {
    {"Low", NULL},   /* 0 低摩擦 */
    {"High", NULL},  /* 1 高摩擦 */
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

    /* 装载当前参数值（弹框绑定的初始值） */
    val_actuator  = (float)Read_System(SAVE_ACTUATOR_TYPE);
    val_balance   = (float)Read_System(SAVE_BALANCE_LEVEL);
    val_error_pct = (float)Read_System(SAVE_REDUNDANT_ERROR) / 10.0f; /* 存储 3~100 → 显示 0.3~10.0% */
    val_friction  = (float)Read_System(SAVE_FRICTION_LEVEL);

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
    SGUI_List_Initialize(&AutoSetListObject, &stListLayout, &UFT8_FZXS12, tr("1.Auto"), ListItems, list_count);

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
        /* 确认保存：按当前列表选中项把弹框结果写入 Flash */
        switch (SGUI_List_GetSelection(&AutoSetListObject)->iIndex)
        {
        case ID_ACTUATOR_TYPE:
            Save_System(SAVE_ACTUATOR_TYPE, (int32_t)val_actuator);
            ValveParam_Get()->actuator_type = (uint8_t)val_actuator;
            DEBUG_LOGI(TAG, "save actuator type=%d", (int)val_actuator);
            break;
        case ID_BALANCE:
            Save_System(SAVE_BALANCE_LEVEL, (int32_t)val_balance);
            ValveParam_Get()->balance_level = (uint8_t)val_balance;
            DEBUG_LOGI(TAG, "save balance=%d", (int)val_balance);
            break;
        case ID_REDUNDANT_ERROR: /* 冗余误差：% → ×10 整型 3~100 存储，数值即 deadband */
        {
            int re = (int)(val_error_pct * 10.0f + 0.5f);
            Save_System(SAVE_REDUNDANT_ERROR, re);
            ValveParam_Get()->deadband = re; /* 热更新，无需重启生效 */
            DEBUG_LOGI(TAG, "save redundant error=%d (0.1%%)", re);
            break;
        }
        case ID_FRICTIONMETER:
            Save_System(SAVE_FRICTION_LEVEL, (int32_t)val_friction);
            ValveParam_Get()->friction_level = (uint8_t)val_friction;
            DEBUG_LOGI(TAG, "save friction=%d", (int)val_friction);
            break;
        default:
            break;
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
        SGUI_List_Repaint(pstDeviceIF, &AutoSetListObject);
        if (get_setValue_Box_State() == true)
            Close_setValveBox(false);
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
        if (select == ID_ACTUATOR_TYPE)
            SelectValueBox(pstDeviceIF, SGUI_List_GetSelection(ActivedObject)->pstItem->cszLabelText,
                           actuator_type_item, get_ItemsCount(actuator_type_item), &val_actuator);
        else if (select == ID_BALANCE)
            SelectValueBox(pstDeviceIF, SGUI_List_GetSelection(ActivedObject)->pstItem->cszLabelText,
                           balance_item, get_ItemsCount(balance_item), &val_balance);
        else if (select == ID_REDUNDANT_ERROR)
        {
            /* 界面按 % 调整（0.3~10.0，步长 0.1），保存时 ×10 转整型 3~100 */
            SetValueBoxArea(0.3f, 10.0f, 0.1f);
            SetValueBox(pstDeviceIF, ListItems[select].cszLabelText, &val_error_pct);
        }
        else if (select == ID_FRICTIONMETER)
            SelectValueBox(pstDeviceIF, SGUI_List_GetSelection(ActivedObject)->pstItem->cszLabelText,
                           friction_item, get_ItemsCount(friction_item), &val_friction);
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
