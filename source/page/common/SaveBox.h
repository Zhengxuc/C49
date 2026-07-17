/*
 * @Author: XinXin
 * @Date: 2023-09-08 16:16:30
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-08 20:07:35
 * @FilePath: \source\simplegui\App\inc\SaveBox.h
 * @Description:
 */
#ifndef __SAVEBOX_H__
#define __SAVEBOX_H__

#include "GUIPublic.h"
#include "Box.h"

int32_t Save_box(SGUI_SCR_DEV *pstDeviceIF, bool select);
enum Box_state Save_box_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent);

#endif
