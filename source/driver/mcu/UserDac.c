/*
 * @Author: XinXin
 * @Date: 2023-03-24 20:30:30
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-31 10:59:02
 * @FilePath: \source\driver\mcu\UserDac.c
 * @Description:
 */

#include "UserDac.h"
#include "sysctrl.h"
#include "dac.h"
#include "flash.h"
#include "gpio.h"

void UserDacInit()
{
    stc_dac_cfg_t dac_initstruct;

    Sysctrl_SetPeripheralGate(SysctrlPeripheralDac, TRUE);  ///< 使能DAC模块的时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE); // 使能GPIO模块的外设时钟

    Gpio_SetAnalogMode(GpioPortA, GpioPin4); // PA04作为DAC的模拟输出

    dac_initstruct.boff_t = DacBoffDisable;
    dac_initstruct.ten_t = DacTenEnable;
    dac_initstruct.sref_t = DacVoltageAvcc;
    dac_initstruct.wave_t = DacWaveDisable; // 禁止生产波
    dac_initstruct.mamp_t = DacMenp4095;
    dac_initstruct.tsel_t = DacSwTriger;  ///< 软件触发方式
    dac_initstruct.align = DacRightAlign; ///< 右对齐
    dac_initstruct.dhr12 = 0;

    Dac_Init(&dac_initstruct);
    Dac_Cmd(TRUE);
    UserDacSetData(0xfff);
}

void UserDacSetData(uint16_t data)
{
    static uint16_t old_dac = 0;
    uint16_t dac = data;

    if (dac > 0xFFF)
        dac = 0xFFF;

    if (dac < 0)
        dac = 0;

    if (old_dac != dac)
    {
        old_dac = dac;
        Dac_SetChannelData(DacRightAlign, DacBit12, dac);
        Dac_SoftwareTriggerCmd();
    }
}

uint16_t UserDacGetData(void)
{
    return Dac_GetDataOutputValue();
}
