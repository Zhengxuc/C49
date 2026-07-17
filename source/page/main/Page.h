/*
 * @Author: XinXin
 * @Date: 2023-09-05 21:12:11
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-16 11:12:10
 * @FilePath: \source\page\main\Page.h
 * @Description:
 */

#ifndef __PAGE_H__
#define __PAGE_H__

#include "SGUI_Typedef.h"
#include "SGUI_Common.h"
#include "HMI_Engine.h"
#include "GUIActions.h"
#include "GUIMain.h"

#define EXPORT_PAGE(name, iScreenID, pstActions) HMI_SCREEN_OBJECT name = {iScreenID, &pstActions}

//=======================================================================//
//= Macro definition.                                                   =//
//=======================================================================//
enum HMI_ID
{
    HMI_SCREEN_ID_SYSTEM = 1001,
    HMI_SCREEN_ID_HOME,
    HMI_SCREEN_ID_MENU_ALL,
    HMI_SCREEN_ID_SYSTEM_SET,
    HMI_SCREEN_ID_GUI_MODULE,
    HMI_SCREEN_ID_DEBUG_PAGE1,
    HMI_SCREEN_ID_AUTO2,
    HMI_SCREEN_ID_AUTO_SET,
    HMI_SCREEN_ID_INPUT_SET,
    HMI_SCREEN_ID_GUI_CURVE,
    HMI_SCREEN_ID_MECHANICAL,
    HMI_SCREEN_ID_TRAVEL,
    HMI_SCREEN_ID_CONTROLPARAMETER,
    HMI_SCREEN_ID_GUI_COUNT_DOWN,
    HMI_SCREEN_ID_TEST,
    HMI_SCREEN_ID_GUI_AUTO_ADJUST,
    HMI_SCREEN_ID_GUI_CURRENT_CALIB,
    HMI_SCREEN_ID_IP_PARAMETER,
    HMI_SCREEN_ID_TRAVEL_PARAMETER,

};

/*所有页面*/
extern HMI_SCREEN_OBJECT g_stGUISystem;
extern HMI_SCREEN_OBJECT g_stGUIHomeSet;
extern HMI_SCREEN_OBJECT g_stGUIMenuAll;
extern HMI_SCREEN_OBJECT g_stGUIModule;
extern HMI_SCREEN_OBJECT g_stGUIDebugPage1;
extern HMI_SCREEN_OBJECT g_stGUIAuto2;
extern HMI_SCREEN_OBJECT g_stGUIMAutoSet;
extern HMI_SCREEN_OBJECT g_stGUIInputSet;
extern HMI_SCREEN_OBJECT g_stGUICurve;
extern HMI_SCREEN_OBJECT g_stGUIMechanical;
extern HMI_SCREEN_OBJECT g_stGUITravel;
extern HMI_SCREEN_OBJECT g_stGUIControlParameter;
extern HMI_SCREEN_OBJECT g_stGUICountDown;
extern HMI_SCREEN_OBJECT g_stGUITest;
extern HMI_SCREEN_OBJECT g_stGUIAutoAdjust;
extern HMI_SCREEN_OBJECT g_stGUICurrentCalib;
extern HMI_SCREEN_OBJECT g_stGUIIPParameter;
extern HMI_SCREEN_OBJECT g_stGUITravelParameter;

#endif
