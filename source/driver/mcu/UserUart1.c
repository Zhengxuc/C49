
#include "UserUart1.h"
#include "uart.h"
#include "gpio.h"
#include "sysctrl.h"
#include "dmac.h"

#include "common.h"
#include "adt.h"
#include "ddl.h"

uint8_t u8TxData[128] = {0};

static void App_UartCfg(void);
void App_DmaCfg(void);
void App_UartPortInit(void);

void UserUart1_init(void)
{
    // uart端口配置
    App_UartPortInit();

    // UART模块配置
    App_UartCfg();

    // DMA参数配置
    App_DmaCfg();

    /*首先发送一次，这里发送一次是为了使后面的Dma_GetStat函数获取到的值为5（DmaTransferComplete）*/
    memset(u8TxData, 0x00, sizeof(u8TxData)); // 数据拷贝
    Dma_SetTransferCnt(DmaCh1, 10);           // 设置长度
    Dma_EnableChannel(DmaCh1);                // 使能通道1

    UserUart1_send_data("\r\n", 3);
    UserUart1_send_data(" _____ ___  ____  _     _____ ___         ____ _  _   ___  \r\n", 62);
    UserUart1_send_data("|_   _/ _ \\|  _ \\| |   | ____/ _ \\       / ___| || | / _ \\ \r\n", 62);
    UserUart1_send_data("  | || | | | |_) | |   |  _|| | | |_____| |   | || || (_) |\r\n", 62);
    UserUart1_send_data("  | || |_| |  _ <| |___| |__| |_| |_____| |___|__   _\\__, |\r\n", 62);
    UserUart1_send_data("  |_| \\___/|_| \\_\\_____|_____\\___/  C49  \\____|  |_|   /_/ \r\n", 62);
}

void UserUart1_send_data(char *pData, int u16Len)
{
    uint16_t len = u16Len;

    /*等待发送完成*/
    while (1)
    {
        if (DmaTransferComplete == Dma_GetStat(DmaCh1)) // 完成一次通道传输（RAM缓存 -> UART1_TX  2字节）
        {
            break;
        }
    }

    if (u16Len > sizeof(u8TxData))
    {
        len = sizeof(u8TxData);
    }

    memcpy(u8TxData, pData, len);    // 数据拷贝
    Dma_DisableChannel(DmaCh1);      // 禁用通道1
    Dma_ClrStat(DmaCh1);             // 清除通道状态值
    Dma_SetTransferCnt(DmaCh1, len); // 设置长度
    Dma_EnableChannel(DmaCh1);       // 使能通道1
}

void UserUart1_log(char *format, ...)
{
    char buffer[64];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    buffer[sizeof(buffer) - 1] = '\0';

    /*发送到队列*/
    UserUart1_send_data(buffer, strlen(buffer));
}

// UART模块配置
void App_UartCfg(void)
{
    stc_uart_cfg_t stcCfg;
    stc_uart_baud_t stcBaud;

    DDL_ZERO_STRUCT(stcCfg);  // 初始化变量
    DDL_ZERO_STRUCT(stcBaud); // 初始化变量

    Sysctrl_SetPeripheralGate(SysctrlPeripheralUart1, TRUE); // 使能UART1外设时钟门控开关

    stcCfg.enRunMode = UartMskMode3;                // 模式3
    stcCfg.enStopBit = UartMsk1bit;                 // 1位停止位
    stcCfg.enMmdorCk = UartMskEven;                 // 偶校验
    stcCfg.stcBaud.u32Baud = 115200;                // 波特率9600
    stcCfg.stcBaud.enClkDiv = UartMsk8Or16Div;      // 通道采样分频配置
    stcCfg.stcBaud.u32Pclk = Sysctrl_GetPClkFreq(); // 获得外设时钟（PCLK）频率值
    Uart_Init(M0P_UART1, &stcCfg);                  // 串口初始化

    Uart_ClrStatus(M0P_UART1, UartRC); // 清接收请求
    Uart_ClrStatus(M0P_UART1, UartTC); // 清发送请求

    Uart_EnableFunc(M0P_UART1, UartDmaTxFunc); // 使能DMA发送, DMA相关通道使能后，如果Tx Buff为空，会立马启动传输
    Uart_EnableFunc(M0P_UART1, UartDmaRxFunc); // 使能DMA接收
}

// DMA参数配置
void App_DmaCfg(void)
{
    stc_dma_cfg_t stcDmaCfg;
    DDL_ZERO_STRUCT(stcDmaCfg); // 初始化变量

    Sysctrl_SetPeripheralGate(SysctrlPeripheralDma, TRUE); // 使能DMAC外设时钟门控开关

    // tx dma配置
    stcDmaCfg.u32SrcAddress = (uint32_t)&u8TxData[0];          // 接收数据缓存
    stcDmaCfg.u32DstAddress = 0x40000100;                      // 发送数据寄存器地址
    stcDmaCfg.enSrcAddrReloadCtl = DmaMskSrcAddrReloadEnable;  // 使能DMA源地址重载
    stcDmaCfg.enSrcBcTcReloadCtl = DmaMskBcTcReloadEnable;     // 使能BC[3:0]和CONFA:TC[15:0]的重载功能
    stcDmaCfg.enDestAddrReloadCtl = DmaMskDstAddrReloadEnable; // 使能DMA目的地址重载
    stcDmaCfg.enTransferMode = DmaMskOneTransfer;              // 一次传输，DMAC传输完成时清除CONFA:ENS位
    stcDmaCfg.enDstAddrMode = DmaMskDstAddrFix;                // 目的地址固定
    stcDmaCfg.enSrcAddrMode = DmaMskSrcAddrInc;                // 源地址自增
    stcDmaCfg.u16BlockSize = 1;                                // 块传输个数
    stcDmaCfg.u16TransferCnt = 8;                              // 块传输次数，一次传输一个字节，传输两次
    stcDmaCfg.enMode = DmaMskBlock;                            // 块(Block)传输
    stcDmaCfg.enTransferWidth = DmaMsk8Bit;                    // 8 bit  字节传输
    stcDmaCfg.enRequestNum = DmaUart1TxTrig;                   // DMA硬件触发源位Uart1Tx
    stcDmaCfg.enPriority = DmaMskPriorityFix;                  // DMA 各通道优先级固定 (CH0>CH1)

    Dma_Enable();                        // DMA模块使能
    Dma_InitChannel(DmaCh1, &stcDmaCfg); // DMA通道1初始化
}

// uart端口配置
void App_UartPortInit(void)
{
    stc_gpio_cfg_t stcGpioCfg;

    DDL_ZERO_STRUCT(stcGpioCfg); // 初始化变量

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE); // 使能GPIO外设时钟门控开关

    stcGpioCfg.enDir = GpioDirOut;
    Gpio_Init(GpioPortA, GpioPin2, &stcGpioCfg);
    Gpio_SetAfMode(GpioPortA, GpioPin2, GpioAf1); // PA02 配置为UART1 TX
    stcGpioCfg.enDir = GpioDirIn;
    Gpio_Init(GpioPortA, GpioPin3, &stcGpioCfg);
    Gpio_SetAfMode(GpioPortA, GpioPin3, GpioAf1); // PA03 配置为UART1 RX
}

// 传送数据给匿名四轴上位机软件
//  fun:功能字. 0XA0~0XAF
//  data:数据缓存区,最多28字节!!
//  len:data区有效数据个数
void UserUart1_send_report(unsigned char fun, unsigned char *data, unsigned char len)
{
    unsigned char send_buf[32];
    unsigned char i;
    if (len > 27)
        return;            // 最多28字节数据
    send_buf[len + 4] = 0; // 校验数置零
    send_buf[0] = 0XAA;    // 帧头
    send_buf[1] = 0XAA;    // 帧头
    send_buf[2] = fun;     // 功能字
    send_buf[3] = len;     // 数据长度
    for (i = 0; i < len; i++)
        send_buf[4 + i] = data[i]; // 复制数据
    for (i = 0; i < len + 4; i++)
        send_buf[len + 4] += send_buf[i]; // 计算校验和

    UserUart1_send_data((char *)send_buf, len + 5);
}

void UserUart1_send_reportV2(unsigned char fun, unsigned char *data, unsigned char len)
{
    uint8_t send_buf[32];
    uint8_t i;
    if (len > 28)
        return;            // 最多28字节数据
    send_buf[len + 3] = 0; // 校验数置零
    send_buf[0] = 0X88;    // 帧头
    send_buf[1] = fun;     // 功能字
    send_buf[2] = len;     // 数据长度
    for (i = 0; i < len; i++)
        send_buf[3 + i] = data[i]; // 复制数据
    for (i = 0; i < len + 3; i++)
        send_buf[len + 3] += send_buf[i]; // 计算校验和

    UserUart1_send_data((char *)send_buf, len + 4);
}