/*
 * @Author: XinXin
 * @Date: 2023-04-04 19:29:24
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-13 18:36:35
 * @FilePath: \source\page\main\GUIMain.h
 * @Description:
 */
#ifndef __GUIMAIN_H__
#define __GUIMAIN_H__

#include <stdlib.h>
#include <stdio.h>
#include "SGUI_Typedef.h"
#include "SGUI_Common.h"
#include "HMI_Engine.h"
#include "GUIActions.h"

//=======================================================================//
//= Data type definition.                                               =//
//=======================================================================//
// HMI action type
typedef enum
{
    EVENT_ID_UNKNOW = 0,
    // User defined action type.
    EVENT_ID_KEY_PRESS,
    EVENT_ID_TIMER,
    EVENT_ID_RTC,
    // User defined action above.
    EVENT_ID_MAX
} EVENT_ID;

// HMI event type.
typedef enum
{
    EVENT_TYPE_ANY = 0,
    EVENT_TYPE_ACTION,
    EVENT_TYPE_DATA,
} EVENT_TYPE;

enum DEMO_PROC_ACTION_ID
{
    HMI_DEMO_PROC_NO_ACT = 0,
    HMI_DEMO_PROC_CONFIRM = 1,
    HMI_DEMO_PROC_CANCEL = 2,
};

typedef struct
{
    SGUI_UINT16 uiKeyValue;
} KEY_EVENT_DATA;

typedef struct
{
    SGUI_INT iValue;
} DUMMY_DATA_EVENT_DATA;

HMI_EVENT_TYPE_DECLARE(KEY_PRESS_EVENT, KEY_EVENT_DATA)
HMI_EVENT_TYPE_DECLARE(DATA_EVENT, DUMMY_DATA_EVENT_DATA);

#define HEART_BEAT_INTERVAL_MS (200)
#define INS_SOURCE(a) extern const unsigned char a[]

/* HMI engine object. */
extern HMI_ENGINE_OBJECT g_stDemoEngine;

HMI_ENGINE_RESULT GUIMainInit(void);
void GUIMainProcess(void);
bool SysTickTimerTriggered(void);
bool UserEventTriggered(void);
void SysTickTimerEnable(bool bEnable);
void disp_hal_init(void);
uint8_t auto_disp_dir(void);
SGUI_SCR_DEV *gui_get_DeviceIF();
#endif // __INCLUDE_DEMO_PROC_H__
