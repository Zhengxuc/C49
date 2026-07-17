#include "GUIMain.h"
#include "SGUI_Text.h"
#include "SGUI_FontResource.h"
#include "SGUI_Typedef.h"
#include "SGUI_Notice.h"
#include "SGUI_VariableBox.h"
#include "SGUI_IconResource.h"
#include "SGUI_Basic.h"
#include "FontBigNumber.h"
#include "DataPort.h"
#include "FontUft8.h"
#include "language.h"
#include "GUIPublic.h"
#include "Page.h"
#include "Draw.h"

#define SCALE_Y 70

static int menu_list_select;

INS_SOURCE(gImage_ImageAuto);
INS_SOURCE(gImage_ImageInput);
INS_SOURCE(gImage_ImageMechanicalState);
INS_SOURCE(gImage_ImageTravel);
INS_SOURCE(gImage_ImageControl);
INS_SOURCE(gImage_ImagePST);
INS_SOURCE(gImage_ImageFeedback);
INS_SOURCE(gImage_ImageSystem);
INS_SOURCE(gImage_image_auto_set);

const unsigned char *image_list[] = {
    gImage_image_auto_set,
    gImage_ImageInput,
    gImage_ImageMechanicalState,
    gImage_ImageTravel,
    gImage_ImageControl,
    gImage_ImagePST,
    gImage_ImageFeedback,
    gImage_ImageSystem,
    gImage_ImageAuto,
};

const char *menu_neme[] = {
    "1.Auto",
    "2.Input",
    "3.Mechanical State",
    "4.Travel",
    "5.Control parameter",
    "6.PST",
    "7.Feedback",
    "8.System",
    "9.Auto ADJ"};

static const uint8_t MAX_LIST = sizeof(image_list) / sizeof(image_list[0]);

static HMI_ENGINE_RESULT HMI_GUIMenuAll_Initialize(SGUI_SCR_DEV *pstDeviceIF);
static HMI_ENGINE_RESULT HMI_GUIMenuAll_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUIMenuAll_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUIMenuAll_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID);
static HMI_ENGINE_RESULT HMI_GUIMenuAll_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID);

static void HMI_PaintMenu(SGUI_SCR_DEV *pstDeviceIF, int scale);
static void Menu_DrawList(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, const unsigned char *image, bool select);

static const HMI_SCREEN_ACTION s_stGUIMenuAll =
    {
        HMI_GUIMenuAll_Initialize,
        HMI_GUIMenuAll_Prepare,
        HMI_GUIMenuAll_RefreshScreen,
        HMI_GUIMenuAll_ProcessEvent,
        HMI_GUIMenuAll_PostProcess};

HMI_SCREEN_OBJECT g_stGUIMenuAll = {HMI_SCREEN_ID_MENU_ALL,
                                    &s_stGUIMenuAll};

HMI_ENGINE_RESULT HMI_GUIMenuAll_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    return HMI_RET_NORMAL;
}

HMI_ENGINE_RESULT HMI_GUIMenuAll_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    if (NULL != pstDeviceIF->fnClear)
        pstDeviceIF->fnClear();
    else
        SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 0, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight, SGUI_COLOR_BKGCLR, SGUI_COLOR_BKGCLR);

    if (pstParameters != NULL)
        menu_list_select = 0;

    HMI_GUIMenuAll_RefreshScreen(pstDeviceIF, pstParameters);
    SysTickTimerEnable(true);

    return HMI_RET_NORMAL;
}

HMI_ENGINE_RESULT HMI_GUIMenuAll_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    /*边框*/
    SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 0, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight, SGUI_COLOR_FRGCLR, SGUI_COLOR_TRANS);

    /*绘制*/
    HMI_PaintMenu(pstDeviceIF, menu_list_select);

    return HMI_RET_NORMAL;
}

HMI_ENGINE_RESULT HMI_GUIMenuAll_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID)
{
    HMI_ENGINE_RESULT eProcessResult;
    SGUI_INT iProcessAction;
    KEY_PRESS_EVENT *pstKeyEvent;
    DATA_EVENT *pstDataEvent;

    eProcessResult = HMI_RET_NORMAL;
    iProcessAction = HMI_DEMO_PROC_NO_ACT;
    pstKeyEvent = (KEY_PRESS_EVENT *)pstEvent;
    switch (pstEvent->iID)
    {
    case EVENT_ID_KEY_PRESS:
        if (SGUI_FALSE == HMI_EVENT_SIZE_CHK(*pstKeyEvent, KEY_PRESS_EVENT))
            eProcessResult = HMI_RET_INVALID_DATA;
        else if (EVENT_ID_KEY_PRESS == pstEvent->iID)
        {
            switch (KEY_CODE_VALUE(pstKeyEvent->Data.uiKeyValue))
            {
            case KEY_VALUE_ENTER:
                if (menu_list_select == 0)
                    HMI_SwitchScreen(HMI_SCREEN_ID_AUTO_SET, NULL);
                else if (menu_list_select == 1)
                    HMI_SwitchScreen(HMI_SCREEN_ID_INPUT_SET, NULL);
                else if (menu_list_select == 2)
                    HMI_SwitchScreen(HMI_SCREEN_ID_MECHANICAL, NULL);
                else if (menu_list_select == 3)
                    HMI_SwitchScreen(HMI_SCREEN_ID_TRAVEL, NULL);
                else if (menu_list_select == 4)
                    HMI_SwitchScreen(HMI_SCREEN_ID_CONTROLPARAMETER, NULL);
                else if (menu_list_select == 5)
                    ;
                else if (menu_list_select == 6)
                    ;
                else if (menu_list_select == 7)
                    HMI_SwitchScreen(HMI_SCREEN_ID_SYSTEM, NULL);
                else if (menu_list_select == 8)
                    HMI_SwitchScreen(HMI_SCREEN_ID_GUI_AUTO_ADJUST, NULL);

                break;
            case KEY_VALUE_ESC:
                iProcessAction = HMI_DEMO_PROC_CANCEL;
                break;
            case KEY_VALUE_UP:
                menu_list_select++;
                menu_list_select = scope(menu_list_select, 0, MAX_LIST - 1);
                HMI_PaintMenu(pstDeviceIF, menu_list_select);
                break;
            case KEY_VALUE_DOWN:
                menu_list_select--;
                menu_list_select = scope(menu_list_select, 0, MAX_LIST - 1);
                HMI_PaintMenu(pstDeviceIF, menu_list_select);
                break;
            default:
                break;
            }
        }
        if (NULL != piActionID)
            *piActionID = iProcessAction;
        break;
    case EVENT_ID_TIMER:
        pstDataEvent = (DATA_EVENT *)pstEvent;
        if (SGUI_FALSE == HMI_EVENT_SIZE_CHK(*pstDataEvent, DATA_EVENT))
            eProcessResult = HMI_RET_INVALID_DATA;
        else
        {
        }
        break;
    default:
        break;
    }

    return eProcessResult;
}

HMI_ENGINE_RESULT HMI_GUIMenuAll_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID)
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

static void HMI_PaintMenu(SGUI_SCR_DEV *pstDeviceIF, int scale)
{
    scale = scope(scale, 0, MAX_LIST - 1);
    if (scale > 0)
        Menu_DrawList(pstDeviceIF, 32, 26, image_list[LOOP(scale - 1)], false);
    else
        Draw_buff_Byte(32, 26, 32, 32, 0x00);

    Menu_DrawList(pstDeviceIF, 48 + 32, 26, image_list[LOOP(scale)], true);
    if (scale < (MAX_LIST - 1))
        Menu_DrawList(pstDeviceIF, 96 + 32, 26, image_list[LOOP(scale + 1)], false);
    else
        Draw_buff_Byte(96 + 32, 26, 32, 32, 0x00);

    Draw_buff_Byte(1, 26 + 48, pstDeviceIF->stSize.iWidth - 2, 16, 0x00); // 擦除文字区域
    DrawStr(pstDeviceIF, pstDeviceIF->stSize.iWidth / 2, 26 + 48, 0, STR_MEDIATE, tr(menu_neme[LOOP(scale)]));
}

static void Menu_DrawList(SGUI_SCR_DEV *pstDeviceIF, SGUI_UINT8 x, SGUI_UINT8 y, const unsigned char *image, bool select)
{
    SGUI_RECT stDisplayArea;
    const SGUI_POINT stInnerPos = {0, 0};

    if (select)
    { /*边框*/
        SGUI_RECT rect;
        rect.iX = x - 6;
        rect.iY = y - 7;
        rect.iWidth = 45;
        rect.iHeight = 48;
        SGUI_Basic_DrawRoundedRectangle(pstDeviceIF, rect.iX, rect.iY, rect.iWidth, rect.iHeight, 5, 1, 1);
        SGUI_Basic_DrawRoundedRectangle(pstDeviceIF, rect.iX + 2, rect.iY + 2, rect.iWidth - 4, rect.iHeight - 4, 5, 1, 0);
    }

    /*图标*/
    SGUI_RECT stArea;
    SGUI_BMP_RES ImageSrc = {32, 32, (const SGUI_BYTE *)image};
    stArea.iX = x;
    stArea.iY = y;
    stArea.iWidth = 32;
    stArea.iHeight = 32;
    SGUI_Basic_DrawBitMap(pstDeviceIF, &stArea, &stInnerPos, &ImageSrc, SGUI_DRAW_NORMAL);
}
