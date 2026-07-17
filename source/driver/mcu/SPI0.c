/*
 * @Descripttion:
 * @version:
 * @Author: Youliangxin
 * @Date: 2023-03-14 10:28:24
 * @LastEditors: XinXin
 * @LastEditTime: 2023-03-16 20:22:49
 */

#include "common.h"
#include "spi.h"
#include "gpio.h"
#include "reset.h"
#include "SPI0.h"

static void SPI0_GpioInit(void)
{
    stc_gpio_cfg_t GpioInitStruct;
    DDL_ZERO_STRUCT(GpioInitStruct);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);

    ///< SPI0引脚配置:主机
    GpioInitStruct.enDrv = GpioDrvH;
    GpioInitStruct.enDir = GpioDirOut;

    Gpio_Init(STK_SPI1_SCK_PORT, STK_SPI1_SCK_PIN, &GpioInitStruct);
    Gpio_SetAfMode(STK_SPI1_SCK_PORT, STK_SPI1_SCK_PIN, GpioAf1); ///< 配置SPI0_SCK

    Gpio_Init(STK_SPI1_MOSI_PORT, STK_SPI1_MOSI_PIN, &GpioInitStruct);
    Gpio_SetAfMode(STK_SPI1_MOSI_PORT, STK_SPI1_MOSI_PIN, GpioAf1); ///< 配置SPI0_MOSI

    GpioInitStruct.enDir = GpioDirIn;
    Gpio_Init(STK_SPI1_MISO_PORT, STK_SPI1_MISO_PIN, &GpioInitStruct);
    Gpio_SetAfMode(STK_SPI1_MISO_PORT, STK_SPI1_MISO_PIN, GpioAf1); ///< 配置SPI0_MISO

    GpioInitStruct.enDir = GpioDirIn;    ///< 端口方向配置->输出
    GpioInitStruct.enOD = GpioOdDisable; ///< 端口开漏输出配置->开漏输出使能
    GpioInitStruct.enPu = GpioPuEnable;  ///< 端口上拉配置->使能
    GpioInitStruct.enPd = GpioPdDisable; ///< 端口下拉配置->禁止
    GpioInitStruct.bOutputVal = TRUE;
    Gpio_Init(GpioPortC, GpioPin6, &GpioInitStruct);
}

void SPI0_Init(void)
{
    stc_spi_cfg_t SpiInitStruct;

    SPI0_GpioInit();

    ///< 打开外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralSpi1, TRUE);

    ///< 复位模块
    Reset_RstPeripheral0(ResetMskSpi1);

    // SPI0模块配置：主机
    SpiInitStruct.enSpiMode = SpiMskMaster;  // 配置为主机模式
    SpiInitStruct.enPclkDiv = SpiClkMskDiv2; // 波特率
    SpiInitStruct.enCPHA = SpiMskCphafirst;  // 第一边沿采样
    SpiInitStruct.enCPOL = SpiMskcpollow;    // 极性为低
    Spi_Init(M0P_SPI1, &SpiInitStruct);
}
