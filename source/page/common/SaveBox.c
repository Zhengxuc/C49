/*
 * @Author: XinXin
 * @Date: 2023-09-08 13:31:10
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-08 23:24:15
 * @FilePath: \source\simplegui\App\src\SaveBox.c
 * @Description:
 */

#include "SaveBox.h"

static bool in_use_Save_box = false; // 保存窗口是否打开
static bool Save_box_select = false; // 光标

int32_t Save_box(SGUI_SCR_DEV *pstDeviceIF, bool select)
{
    in_use_Save_box = true;
    Save_box_select = select;
    uint8_t x_select = 1;
    static SGUI_NOTICT_BOX s_stDemoNoticeBox;
    s_stDemoNoticeBox.cszNoticeText = tr("Whether to save?");
    s_stDemoNoticeBox.pstIcon = &SGUI_RES_ICON_QUESTION_16;
    s_stDemoNoticeBox.stLayout.iX = 30;
    s_stDemoNoticeBox.stLayout.iY = 20;
    s_stDemoNoticeBox.stLayout.iWidth = pstDeviceIF->stSize.iWidth - (s_stDemoNoticeBox.stLayout.iX * 2);
    s_stDemoNoticeBox.stLayout.iHeight = pstDeviceIF->stSize.iHeight - (s_stDemoNoticeBox.stLayout.iY * 2);
    SGUI_Notice_Repaint(pstDeviceIF, &s_stDemoNoticeBox, &UFT8_FZXS12, 0);

    if (select == true)
        x_select = 1;
    else
        x_select = 3;

    SGUI_Basic_DrawRectangle(pstDeviceIF,
                             (s_stDemoNoticeBox.stLayout.iX + s_stDemoNoticeBox.stLayout.iWidth / 4 * x_select) - 15,
                             (s_stDemoNoticeBox.stLayout.iY + 30) - 4,
                             30, UFT8_FZXS12.iHeight + 8, SGUI_COLOR_FRGCLR, SGUI_COLOR_BKGCLR);

    DrawStr(pstDeviceIF,
            s_stDemoNoticeBox.stLayout.iX + s_stDemoNoticeBox.stLayout.iWidth / 4,
            s_stDemoNoticeBox.stLayout.iY + 30,
            0, STR_MEDIATE, tr("Yes"));

    DrawStr(pstDeviceIF,
            s_stDemoNoticeBox.stLayout.iX + (s_stDemoNoticeBox.stLayout.iWidth / 4 * 3),
            s_stDemoNoticeBox.stLayout.iY + 30,
            0, STR_MEDIATE, tr("No"));
    return -1;
}

static void Close_Save_box(void)
{
    in_use_Save_box = false;
}

/*处理事件*/
enum Box_state Save_box_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent)
{
    if (in_use_Save_box == false)
        return BOX_INVALID;

    KEY_PRESS_EVENT *pstKeyEvent;
    pstKeyEvent = (KEY_PRESS_EVENT *)pstEvent;

    if (EVENT_ID_KEY_PRESS == pstEvent->iID)
    {
        switch (KEY_CODE_VALUE(pstKeyEvent->Data.uiKeyValue))
        {
        case KEY_VALUE_ENTER:
            Close_Save_box();
            if (Save_box_select)
                return BOX_YES;
            else
                return BOX_NO;
        case KEY_VALUE_ESC:
            Close_Save_box();
            return BOX_RETURN;
        case KEY_VALUE_UP:
        case KEY_VALUE_DOWN:
            if (Save_box_select)
                Save_box(pstDeviceIF, false);
            else
                Save_box(pstDeviceIF, true);
            return BOX_ENABLE;
        default:
            break;
        }
    }

    return BOX_ENABLE;
}
