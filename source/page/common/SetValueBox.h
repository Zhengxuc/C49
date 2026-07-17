/*
 * @Author: XinXin
 * @Date: 2023-09-08 16:20:39
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-16 15:09:17
 * @FilePath: \source\page\common\SetValueBox.h
 * @Description:
 */

#ifndef __SETVALUE_H__
#define __SETVALUE_H__

#include "GUIPublic.h"
#include "Box.h"

int32_t SelectValueBox(SGUI_SCR_DEV *pstDeviceIF, SGUI_CSZSTR cszTitle, SGUI_ITEMS_ITEM *pstItemsData, SGUI_INT iItemsCount, void *num);
int32_t SetValueBox(SGUI_SCR_DEV *pstDeviceIF, SGUI_CSZSTR text, float32_t *num);
void SetValueBoxArea(float32_t min, float32_t max, float32_t step);
enum Box_state SetValueBox_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent);
void Close_setValveBox(bool save);
bool get_setValue_Box_State(void);
void SetValueBoxRepaint(void);

#endif
