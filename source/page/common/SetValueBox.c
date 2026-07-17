/*
 * @Author: XinXin
 * @Date: 2023-09-08 16:20:32
 * @LastEditors: XinXin
 * @LastEditTime: 2024-12-24 13:30:26
 * @FilePath: \source\page\common\SetValueBox.c
 * @Description:
 */
#include "SetValueBox.h"
#include "SGUI_ProcessBar.h"

static const char *TAG = "SetValueBox";

enum
{
    MODE_VALUE = 0,
    MODE_LIST,
};

static bool in_use_box = false; // 保存窗口是否打开
static float32_t button_press_timer;
static bool enable_button_press_timer;
static float32_t *value = NULL;
static float32_t value_back = 0;
static float32_t stepping = 0;
static float32_t value_max = 100;
static float32_t value_min = 0;
static uint8_t box_mode;

static SGUI_SCR_DEV *BoxDeviceIF;
static SGUI_CSZSTR BoxText;
static SGUI_PROCBAR_STRUCT pstProcessBarData;
static SGUI_LIST SelectListObject;

static void Refresh_Value(SGUI_SCR_DEV *pstDeviceIF, float32_t dat);

void Close_setValveBox(bool save)
{
    if (!save)
    {
        *value = value_back;
    }
    in_use_box = false;
}

bool get_setValue_Box_State(void)
{
    return in_use_box;
}

void SetValueBoxArea(float32_t min, float32_t max, float32_t step)
{
    if (!in_use_box)
    {
        value_min = min;
        value_max = max;
        stepping = step;
    }
}

void SetValueBoxRepaint(void)
{
    /*框*/
    SGUI_Basic_DrawRectangle(BoxDeviceIF, 30, 12, BoxDeviceIF->stSize.iWidth - 60, BoxDeviceIF->stSize.iHeight - 24, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);
    SGUI_Basic_DrawRectangle(BoxDeviceIF, 32, 14, BoxDeviceIF->stSize.iWidth - 64, BoxDeviceIF->stSize.iHeight - 28, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);

    pstProcessBarData.Parameter.PosX = 40;
    pstProcessBarData.Parameter.PosY = 40;
    pstProcessBarData.Parameter.Width = BoxDeviceIF->stSize.iWidth - 80;
    pstProcessBarData.Parameter.Height = 16;
    pstProcessBarData.Parameter.Direction = SGUI_PROCBAR_RIGHT;
    pstProcessBarData.Parameter.MaxValue = 100;
    pstProcessBarData.Data.Value = 0;
    SGUI_ProcessBar_Repaint(BoxDeviceIF, &pstProcessBarData);

    DrawStr(BoxDeviceIF,
            BoxDeviceIF->stSize.iWidth / 2,
            pstProcessBarData.Parameter.PosY - 20,
            0, STR_MEDIATE, tr(BoxText));

    Refresh_Value(BoxDeviceIF, *value);
}

int32_t SetValueBox(SGUI_SCR_DEV *pstDeviceIF, SGUI_CSZSTR text, float32_t *num)
{
    if (!in_use_box)
    {
        box_mode = MODE_VALUE;
        in_use_box = true;
        value = (float32_t *)num;
        value_back = *value;
        enable_button_press_timer = true;
        button_press_timer = 1.0f;

        BoxDeviceIF = pstDeviceIF;
        BoxText = text;

        /*框*/
        SGUI_Basic_DrawRectangle(pstDeviceIF, 30, 12, pstDeviceIF->stSize.iWidth - 60, pstDeviceIF->stSize.iHeight - 24, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);
        SGUI_Basic_DrawRectangle(pstDeviceIF, 32, 14, pstDeviceIF->stSize.iWidth - 64, pstDeviceIF->stSize.iHeight - 28, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);

        pstProcessBarData.Parameter.PosX = 40;
        pstProcessBarData.Parameter.PosY = 40;
        pstProcessBarData.Parameter.Width = pstDeviceIF->stSize.iWidth - 80;
        pstProcessBarData.Parameter.Height = 16;
        pstProcessBarData.Parameter.Direction = SGUI_PROCBAR_RIGHT;
        pstProcessBarData.Parameter.MaxValue = 100;
        pstProcessBarData.Data.Value = 0;
        SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);

        DrawStr(pstDeviceIF,
                pstDeviceIF->stSize.iWidth / 2,
                pstProcessBarData.Parameter.PosY - 20,
                0, STR_MEDIATE, tr(text));

        Refresh_Value(pstDeviceIF, *value);
        return 0;
    }
    return -1;
}

int32_t SelectValueBox(SGUI_SCR_DEV *pstDeviceIF, SGUI_CSZSTR cszTitle, SGUI_ITEMS_ITEM *pstItemsData, SGUI_INT iItemsCount, void *num)
{
    if (!in_use_box)
    {
        box_mode = MODE_LIST;
        in_use_box = true;
        value = (float32_t *)num;
        value_back = *value;
        enable_button_press_timer = false;
        button_press_timer = 1.0f;
        BoxDeviceIF = pstDeviceIF;
        value_min = 0;
        value_max = iItemsCount - 1;
        stepping = 1;
        /*翻译*/
        for (int i = 0; i < iItemsCount; i++)
        {
            pstItemsData[i].szVariableText = (SGUI_SZSTR)tr(pstItemsData[i].cszLabelText);
        }

        /*框*/
        SGUI_Basic_DrawRectangle(pstDeviceIF, 30, 12, pstDeviceIF->stSize.iWidth - 60, pstDeviceIF->stSize.iHeight - 24, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);

        SGUI_RECT stListLayout;
        SGUI_SystemIF_MemorySet(&SelectListObject, 0x00, sizeof(SGUI_LIST));
        stListLayout.iX = 32;
        stListLayout.iY = 14;
        stListLayout.iWidth = pstDeviceIF->stSize.iWidth - 64;
        stListLayout.iHeight = pstDeviceIF->stSize.iHeight - 28;
        SGUI_List_Initialize(&SelectListObject, &stListLayout, &UFT8_FZXS12, tr(cszTitle), pstItemsData, iItemsCount);
        Refresh_Value(pstDeviceIF, *value);
        return 0;
    }
    return -1;
}

static void Refresh_Value(SGUI_SCR_DEV *pstDeviceIF, float32_t dat)
{
    if (box_mode == MODE_VALUE)
    {
        pstProcessBarData.Data.Value = (100.0f / (value_max - value_min) * (dat - value_min)) + 0.5f;
        SGUI_ProcessBar_Repaint(pstDeviceIF, &pstProcessBarData);

        DrawStr(pstDeviceIF,
                pstDeviceIF->stSize.iWidth / 2,
                pstProcessBarData.Parameter.PosY + 20,
                0, STR_MEDIATE, "                  ");
        if (stepping <= 0.011f)
        {
            DrawStr(pstDeviceIF,
                    pstDeviceIF->stSize.iWidth / 2,
                    pstProcessBarData.Parameter.PosY + 20,
                    0, STR_MEDIATE, "%.2f", dat);
        }
        else if (stepping <= 0.11f)
        {
            DrawStr(pstDeviceIF,
                    pstDeviceIF->stSize.iWidth / 2,
                    pstProcessBarData.Parameter.PosY + 20,
                    0, STR_MEDIATE, "%.1f", dat);
        }
        else
        {
            DrawStr(pstDeviceIF,
                    pstDeviceIF->stSize.iWidth / 2,
                    pstProcessBarData.Parameter.PosY + 20,
                    0, STR_MEDIATE, "%d", (int)dat);
        }
    }
    else if (box_mode == MODE_LIST)
    {
        if ((int)dat >= 0 && (int)dat <= SelectListObject.stItems.iCount)
            SGUI_List_Selecte(&SelectListObject, (int)dat);
        SGUI_List_Repaint(pstDeviceIF, &SelectListObject);
    }
}

/*处理事件*/
enum Box_state SetValueBox_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent)
{
    if (in_use_box == false)
        return BOX_INVALID;

    KEY_PRESS_EVENT *pstKeyEvent;
    pstKeyEvent = (KEY_PRESS_EVENT *)pstEvent;

    if (EVENT_ID_KEY_PRESS == pstEvent->iID)
    {
        switch (KEY_CODE_VALUE(pstKeyEvent->Data.uiKeyValue))
        {
        case KEY_VALUE_ENTER:
        case KEY_VALUE_ESC:
            if (*value == value_back)
                return BOX_NO;
            else
                return BOX_YES;
        case KEY_VALUE_UP:
            if ((*value + stepping) < value_max)
                *value += (stepping * (int32_t)button_press_timer);
            else
                *value = value_max;
            Refresh_Value(pstDeviceIF, *value);
            if (enable_button_press_timer)
                button_press_timer += button_press_timer * 0.1f;
            return BOX_VARIATION;
        case KEY_VALUE_DOWN:
            if ((*value - stepping) > value_min)
                *value -= (stepping * (int32_t)button_press_timer);
            else
                *value = value_min;
            Refresh_Value(pstDeviceIF, *value);
            if (enable_button_press_timer)
                button_press_timer += button_press_timer * 0.1f;
            return BOX_VARIATION;
        case KEY_VALUE_NONE:
            button_press_timer = 1.0f;
            break;
        default:
            break;
        }
    }

    return BOX_ENABLE;
}
