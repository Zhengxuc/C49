/*
 * @Author: XinXin
 * @Date: 2023-09-05 00:00:10
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-15 10:48:57
 * @FilePath: \source\page\common\GUIPublic.h
 * @Description:
 */

#ifndef __GUIPUBLIC_H__
#define __GUIPUBLIC_H__

#include "HMI_Engine.h"
#include "GUIMain.h"
#include "SGUI_Text.h"
#include "SGUI_FontResource.h"
#include "SGUI_Typedef.h"
#include "SGUI_Notice.h"
#include "SGUI_VariableBox.h"
#include "SGUI_IconResource.h"
#include "SGUI_List.h"
#include "DataPort.h"
#include "fontuft8.h"
#include "language.h"
#include "page.h"

#define LOOP(a) (a < 0 ? MAX_LIST + a : (a >= (MAX_LIST) ? a - MAX_LIST : a))

#define get_ItemsCount(control_mode) (sizeof(control_mode) / sizeof(SGUI_ITEMS_ITEM))

enum STR_MODE
{
    STR_MEDIATE,
    STR_LEFT,
    STR_RIGHT,
};

struct ListTable
{
    SGUI_LIST Object;
    SGUI_ITEMS_ITEM *Itme;
    char const *Title;
    char const *Title_replace;
    SGUI_INT Len;
};

void DrawRt(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, SGUI_UINT8 Width, SGUI_INT Height);
void DrawStr(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, uint8_t color, enum STR_MODE strMode, const char *format, ...);
void DrawStr_reverse(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, enum STR_MODE strMode, const char *format, ...);
void DrawList(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, struct ListTable *List, SGUI_UINT16 select);
#endif
