/*
 * @Author: XinXin
 * @Date: 2023-09-04 23:58:56
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-15 10:48:42
 * @FilePath: \source\page\common\GUIPublic.c
 * @Description:
 */

#include "GUIPublic.h"
#include "SGUI_FontResource.h"
#include "HMI_Engine.h"
#include "FontUft8.h"
#include <stdio.h>

/*绘制一个空白矩形，用于擦除界面*/
void DrawRt(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, SGUI_UINT8 Width, SGUI_INT Height)
{
    SGUI_Basic_FillRectangleArea(pstDeviceIF, x, y, Width, Height, SGUI_COLOR_BKGCLR);
}

/*绘制字符串*/
void DrawStr(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, uint8_t color, enum STR_MODE strMode, const char *format, ...)
{
    char buffer[32]; // 这里设置大一点，以免vsprintf溢出

    if (format == NULL)
        return;

    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    buffer[31] = '\0';

    SGUI_RECT stDisplayArea;
    const SGUI_POINT stInnerPos = {0, 0};

    int textLen = SGUI_Text_GetTextWidth(buffer, &UFT8_FZXS12);

    switch (strMode)
    {
    case STR_MEDIATE:
        stDisplayArea.iX = x - (textLen / 2);
        break;
    case STR_LEFT:
        stDisplayArea.iX = x;
        break;
    case STR_RIGHT:
        stDisplayArea.iX = x - textLen;
        break;
    default:
        break;
    }

    stDisplayArea.iY = y;
    stDisplayArea.iWidth = textLen;
    stDisplayArea.iHeight = UFT8_FZXS12.iHeight + 1;
    SGUI_Text_DrawText(pstDeviceIF, buffer, &UFT8_FZXS12, &stDisplayArea, &stInnerPos, color);
}

/*绘制字符串*/
void DrawStr_reverse(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, enum STR_MODE strMode, const char *format, ...)
{
    char buffer[32]; // 这里设置大一点，以免vsprintf溢出

    if (format == NULL)
        return;

    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    buffer[31] = '\0';

    SGUI_RECT stDisplayArea;
    const SGUI_POINT stInnerPos = {0, 0};

    int textLen = SGUI_Text_GetTextWidth(buffer, &UFT8_FZXS12);

    switch (strMode)
    {
    case STR_MEDIATE:
        stDisplayArea.iX = x - (textLen / 2);
        break;
    case STR_LEFT:
        stDisplayArea.iX = x;
        break;
    case STR_RIGHT:
        stDisplayArea.iX = x - textLen;
        break;
    default:
        break;
    }

    stDisplayArea.iY = y;
    stDisplayArea.iWidth = textLen;
    stDisplayArea.iHeight = UFT8_FZXS12.iHeight + 1;
    SGUI_Text_DrawText(pstDeviceIF, buffer, &UFT8_FZXS12, &stDisplayArea, &stInnerPos, SGUI_DRAW_REVERSE);
}

void DrawList(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, struct ListTable *List, SGUI_UINT16 select)
{
    SGUI_RECT RCR_SetLayout;

    if ((List->Itme == NULL) || (List == NULL))
        return;

    SGUI_SystemIF_MemorySet(&List->Object, 0x00, sizeof(SGUI_LIST));
    RCR_SetLayout.iX = x;
    RCR_SetLayout.iY = y;
    RCR_SetLayout.iWidth = pstDeviceIF->stSize.iWidth - (x * 2);
    RCR_SetLayout.iHeight = pstDeviceIF->stSize.iHeight - (y * 2);

    /*翻译*/
    for (int i = 0; i < List->Len; i++)
    {
        List->Itme[i].szVariableText = (SGUI_SZSTR)tr(List->Itme[i].cszLabelText);
        List->Title_replace = tr(List->Title);
    }

    SGUI_List_Initialize(&List->Object, &RCR_SetLayout, &UFT8_FZXS12, List->Title_replace, List->Itme, List->Len);

    if ((select > 0) && (select <= (List->Object.stItems.iCount - 1)))
        SGUI_List_Selecte(&List->Object, select);

    SGUI_List_Repaint(pstDeviceIF, &List->Object);
}