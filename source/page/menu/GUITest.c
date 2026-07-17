#include "GUIMain.h"
#include "SGUI_Text.h"
#include "SGUI_FontResource.h"
#include "SGUI_Typedef.h"
#include "SGUI_Notice.h"
#include "SGUI_VariableBox.h"
#include "SGUI_IconResource.h"
#include "DataPort.h"
#include "Page.h"
#include "fontuft8.h"

static HMI_ENGINE_RESULT HMI_GUITest_Initialize(SGUI_SCR_DEV *pstDeviceIF);
static HMI_ENGINE_RESULT HMI_GUITest_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUITest_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters);
static HMI_ENGINE_RESULT HMI_GUITest_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID);
static HMI_ENGINE_RESULT HMI_GUITest_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID);

static SGUI_NUM_VARBOX_STRUCT TempVariableBox;
static SGUI_NUM_VARBOX_STRUCT HumiVariableBox;
static SGUI_NUM_VARBOX_STRUCT AngleVariableBox;
static SGUI_NUM_VARBOX_STRUCT CurrentVariableBox;
static SGUI_NUM_VARBOX_STRUCT CurrentOUTVariableBox;

static HMI_SCREEN_ACTION s_stGUITestActions =
    {
        HMI_GUITest_Initialize,
        HMI_GUITest_Prepare,
        HMI_GUITest_RefreshScreen,
        HMI_GUITest_ProcessEvent,
        HMI_GUITest_PostProcess};

HMI_SCREEN_OBJECT g_stGUITest = {HMI_SCREEN_ID_TEST,
                                 &s_stGUITestActions};

HMI_ENGINE_RESULT HMI_GUITest_Initialize(SGUI_SCR_DEV *pstDeviceIF)
{
    /*----------------------------------*/
    /* Variable Declaration             */
    /*----------------------------------*/
    SGUI_NUM_VARBOX_PARAM stNumBoxInitParam;

    stNumBoxInitParam.eAlignment = SGUI_CENTER;
    stNumBoxInitParam.iMin = -50;
    stNumBoxInitParam.iMax = 0xffff;
    stNumBoxInitParam.pstFontRes = &UFT8_FZXS12;
    stNumBoxInitParam.stLayout.iX = 40;
    stNumBoxInitParam.stLayout.iY = 0;
    stNumBoxInitParam.stLayout.iWidth = 40;
    stNumBoxInitParam.stLayout.iHeight = UFT8_FZXS12.iHeight;
    SGUI_NumberVariableBox_Initialize(&TempVariableBox, &stNumBoxInitParam);
    stNumBoxInitParam.stLayout.iY = 20;
    SGUI_NumberVariableBox_Initialize(&HumiVariableBox, &stNumBoxInitParam);
    stNumBoxInitParam.stLayout.iY = 40;
    SGUI_NumberVariableBox_Initialize(&AngleVariableBox, &stNumBoxInitParam);

    stNumBoxInitParam.stLayout.iX += 90;
    stNumBoxInitParam.stLayout.iY = 0;
    SGUI_NumberVariableBox_Initialize(&CurrentVariableBox, &stNumBoxInitParam);
    stNumBoxInitParam.stLayout.iY = 20;
    SGUI_NumberVariableBox_Initialize(&CurrentOUTVariableBox, &stNumBoxInitParam);

    return HMI_RET_NORMAL;
}

HMI_ENGINE_RESULT HMI_GUITest_Prepare(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    if (NULL != pstDeviceIF->fnClear)
    {
        pstDeviceIF->fnClear();
    }
    else
    {
        SGUI_Basic_DrawRectangle(pstDeviceIF, 0, 0, pstDeviceIF->stSize.iWidth, pstDeviceIF->stSize.iHeight, SGUI_COLOR_BKGCLR, SGUI_COLOR_BKGCLR);
    }
    HMI_GUITest_RefreshScreen(pstDeviceIF, pstParameters);

    SysTickTimerEnable(true);

    return HMI_RET_NORMAL;
}

HMI_ENGINE_RESULT HMI_GUITest_RefreshScreen(SGUI_SCR_DEV *pstDeviceIF, const void *pstParameters)
{
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &TempVariableBox, SGUI_DRAW_NORMAL);
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &HumiVariableBox, SGUI_DRAW_NORMAL);
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &AngleVariableBox, SGUI_DRAW_NORMAL);
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &CurrentVariableBox, SGUI_DRAW_NORMAL);
    SGUI_NumberVariableBox_Repaint(pstDeviceIF, &CurrentOUTVariableBox, SGUI_DRAW_NORMAL);

    SGUI_RECT stDisplayArea;
    SGUI_POINT stInnerPos;
    stInnerPos.iX = 0;
    stInnerPos.iY = 0;

    stDisplayArea.iX = 0;
    stDisplayArea.iY = 0;
    stDisplayArea.iWidth = 6 * 8;
    stDisplayArea.iHeight = UFT8_FZXS12.iHeight + 1;
    SGUI_Text_DrawText(pstDeviceIF, "温度:", &UFT8_FZXS12, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);

    stDisplayArea.iY = 20;
    SGUI_Text_DrawText(pstDeviceIF, "湿度:", &UFT8_FZXS12, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);

    stDisplayArea.iY = 40;
    SGUI_Text_DrawText(pstDeviceIF, "角度:", &UFT8_FZXS12, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);

    stDisplayArea.iX += 96;
    stDisplayArea.iY = 0;
    SGUI_Text_DrawText(pstDeviceIF, "电流:", &UFT8_FZXS12, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);
    stDisplayArea.iY = 20;
    SGUI_Text_DrawText(pstDeviceIF, "输出:", &UFT8_FZXS12, &stDisplayArea, &stInnerPos, SGUI_DRAW_NORMAL);

    return HMI_RET_NORMAL;
}

HMI_ENGINE_RESULT HMI_GUITest_ProcessEvent(SGUI_SCR_DEV *pstDeviceIF, const HMI_EVENT_BASE *pstEvent, SGUI_INT *piActionID)
{
    HMI_ENGINE_RESULT eProcessResult;
    SGUI_INT iProcessAction;
    KEY_PRESS_EVENT *pstKeyEvent;
    DATA_EVENT *pstDataEvent;
    SGUI_UINT16 uiKeyCode;
    SGUI_UINT16 uiKeyValue;

    /*----------------------------------*/
    /* Initialize                       */
    /*----------------------------------*/
    eProcessResult = HMI_RET_NORMAL;
    iProcessAction = HMI_DEMO_PROC_NO_ACT;
    pstKeyEvent = (KEY_PRESS_EVENT *)pstEvent;
    switch (pstEvent->iID)
    {
    case EVENT_ID_KEY_PRESS:
        if (SGUI_FALSE == HMI_EVENT_SIZE_CHK(*pstKeyEvent, KEY_PRESS_EVENT))
        {
            // Event data is invalid.
            eProcessResult = HMI_RET_INVALID_DATA;
        }
        else if (EVENT_ID_KEY_PRESS == pstEvent->iID)
        {
            uiKeyCode = pstKeyEvent->Data.uiKeyValue;
            uiKeyValue = KEY_CODE_VALUE(uiKeyCode);
            switch (uiKeyValue)
            {
            case KEY_VALUE_ENTER:
                break;
            case KEY_VALUE_ESC:
                g_stGUITest.pstPrevious = NULL;
                HMI_SwitchScreen(HMI_SCREEN_ID_HOME, NULL);
                break;
            case KEY_VALUE_UP:
                SGUI_NumberVariableBox_SetValue(&CurrentOUTVariableBox, SGUI_NumberVariableBox_GetValue(&CurrentOUTVariableBox) - 10);
                SGUI_NumberVariableBox_Repaint(pstDeviceIF, &CurrentOUTVariableBox, SGUI_DRAW_NORMAL);
                // Port_SetFloat(DATA_I_OUTPUT_CURRENT, SGUI_NumberVariableBox_GetValue(&CurrentOUTVariableBox));
                break;
            case KEY_VALUE_DOWN:
                SGUI_NumberVariableBox_SetValue(&CurrentOUTVariableBox, SGUI_NumberVariableBox_GetValue(&CurrentOUTVariableBox) + 10);
                SGUI_NumberVariableBox_Repaint(pstDeviceIF, &CurrentOUTVariableBox, SGUI_DRAW_NORMAL);
                // Port_SetFloat(DATA_I_OUTPUT_CURRENT, SGUI_NumberVariableBox_GetValue(&CurrentOUTVariableBox));
                break;
            default:
                break;
            }
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
            // Event data is invalid.
            eProcessResult = HMI_RET_INVALID_DATA;
        }
        else
        {
            SGUI_NumberVariableBox_SetValue(&TempVariableBox, Port_GetFloat(DATA_F_TEMP));
            SGUI_NumberVariableBox_Repaint(pstDeviceIF, &TempVariableBox, SGUI_DRAW_NORMAL);
            SGUI_NumberVariableBox_SetValue(&HumiVariableBox, Port_GetFloat(DATA_F_HUMI));
            SGUI_NumberVariableBox_Repaint(pstDeviceIF, &HumiVariableBox, SGUI_DRAW_NORMAL);

            float anagle = 125 - Port_GetFloat(DATA_F_ANGLE);

            SGUI_NumberVariableBox_SetValue(&AngleVariableBox, (100.0f / 30.0f * anagle)*10);
            SGUI_NumberVariableBox_Repaint(pstDeviceIF, &AngleVariableBox, SGUI_DRAW_NORMAL);

            SGUI_NumberVariableBox_SetValue(&CurrentVariableBox, Port_GetFloat(DATA_F_INPUT_CURRENT));
            SGUI_NumberVariableBox_Repaint(pstDeviceIF, &CurrentVariableBox, SGUI_DRAW_NORMAL);
        }
        break;
    default:
        break;
    }

    return eProcessResult;
}

HMI_ENGINE_RESULT HMI_GUITest_PostProcess(SGUI_SCR_DEV *pstDeviceIF, HMI_ENGINE_RESULT eProcResult, SGUI_INT iActionID)
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
