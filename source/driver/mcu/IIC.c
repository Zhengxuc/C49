
#include "i2c.h"
#include "gpio.h"
#include "common.h"

static void IIC_Gpio_Cfg();
static void IIC_Cfg();

void IIC_Init(void)
{
    ///< I2C 模块配置
    IIC_Gpio_Cfg();
    IIC_Cfg();
}

/*< IO端口配置PA9为SCL PA10为SDA (KTH5702)
							PA11为SCL PA12为SDA(ADS1115)
*/
static void IIC_Gpio_Cfg(void)
{
    stc_gpio_cfg_t stcGpioCfg;

    DDL_ZERO_STRUCT(stcGpioCfg); ///< 初始化结构体变量的值为0

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE); ///< 开启GPIO时钟门控

    stcGpioCfg.enDir = GpioDirOut;   ///< 端口方向配置->输出
    stcGpioCfg.enOD = GpioOdEnable;  ///< 端口开漏输出配置->开漏输出使能
    stcGpioCfg.enPu = GpioPuEnable;  ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable; ///< 端口下拉配置->禁止
    stcGpioCfg.bOutputVal = TRUE;    ///< 默认端口输出电平

    // IIC0
    Gpio_Init(GpioPortA, GpioPin9, &stcGpioCfg);   ///< 初始化PA9
    Gpio_Init(GpioPortA, GpioPin10, &stcGpioCfg);  ///< 初始化PA10
    Gpio_SetAfMode(GpioPortA, GpioPin9, GpioAf4);  ///< 配置PA9为SCL
    Gpio_SetAfMode(GpioPortA, GpioPin10, GpioAf4); ///< 配置PA10为SDA

    // IIC1
    Gpio_Init(GpioPortA, GpioPin11, &stcGpioCfg);  ///< 初始化PA11
    Gpio_Init(GpioPortA, GpioPin12, &stcGpioCfg);  ///< 初始化PA12
    Gpio_SetAfMode(GpioPortA, GpioPin11, GpioAf3); ///< 配置PA11为SCL
    Gpio_SetAfMode(GpioPortA, GpioPin12, GpioAf3); ///< 配置PA12为SDA
}

///< I2C 模块配置
static void IIC_Cfg(void)
{
    stc_i2c_cfg_t stcI2cCfg;

    DDL_ZERO_STRUCT(stcI2cCfg); //< 初始化结构体变量的值为0

    Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c0, TRUE); ///< 开启I2C0时钟门控
    Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c1, TRUE); ///< 开启I2C1时钟门控

    stcI2cCfg.u32Pclk = Sysctrl_GetPClkFreq(); //< 获取PCLK时钟
    stcI2cCfg.u32Baud = 100000;                //< 波特率100kHz,CPU 主频只有4M的时候 跑不到400K频率
    stcI2cCfg.enMode = I2cMasterMode;          //< I2C主机模式
    I2C_Init(M0P_I2C0, &stcI2cCfg);            //< 模块初始化
    I2C_Init(M0P_I2C1, &stcI2cCfg);            //< 模块初始化
}

/**
 ******************************************************************************
 ** \brief  主机接收函数
 **
 ** \param u8Addr从机内存地址，pu8Data读数据存放缓存，u32Len读数据长度
 **
 ** \retval 读数据是否成功
 **
 ******************************************************************************/
en_result_t I2C_MasterReadData(M0P_I2C_TypeDef *I2CX, uint8_t slaveaddr, uint8_t u8Addr, uint8_t *pu8Data, uint32_t u32Len)
{
    int timeout = 0;
    en_result_t enRet = Error;
    uint8_t u8i = 0, u8State;

    I2C_SetFunc(I2CX, I2cStart_En);

    while (1)
    {
        while (0 == I2C_GetIrq(I2CX))
        {
            timeout++;
            if (timeout > 10000)
                return Error;
        }
        u8State = I2C_GetState(I2CX);
        switch (u8State)
        {
        case 0x08: ///< 已发送起始条件，将发送SLA+W
            I2C_ClearFunc(I2CX, I2cStart_En);
            if (u8Addr == 0xff)                               // 如果没有指定寄存器地址则直接发送读取命令
                I2C_WriteByte(I2CX, (slaveaddr << 1) | 0x01); ///< 发送SLA+R，开始从从机读取数据
            else
                I2C_WriteByte(I2CX, (slaveaddr << 1));
            break;
        case 0x18:                       ///< 已发送SLA+W,并接收到ACK
            I2C_WriteByte(I2CX, u8Addr); ///< 发送从机内存地址
            break;
        case 0x20:
            I2C_SetFunc(I2CX, I2cStop_En); ///< 发送停止条件
            I2C_ClearIrq(I2CX);            ///< 清除中断状态标志位
            return enRet;
        case 0x28:                          ///< 已发送数据，接收到ACK, 此处是已发送从机内存地址u8Addr并接收到ACK
            I2C_SetFunc(I2CX, I2cStart_En); ///< 发送重复起始条件
            break;
        case 0x10: ///< 已发送重复起始条件
            I2C_ClearFunc(I2CX, I2cStart_En);
            I2C_WriteByte(I2CX, (slaveaddr << 1) | 0x01); ///< 发送SLA+R，开始从从机读取数据
            break;
        case 0x40: ///< 已发送SLA+R，并接收到ACK
            if (u32Len > 1)
            {
                I2C_SetFunc(I2CX, I2cAck_En); ///< 使能主机应答功能
            }
            break;
        case 0x50: ///< 已接收数据字节，并已返回ACK信号
            pu8Data[u8i++] = I2C_ReadByte(I2CX);
            if (u8i == u32Len - 1)
            {
                I2C_ClearFunc(I2CX, I2cAck_En); ///< 已接收到倒数第二个字节，关闭ACK应答功能
            }
            break;
        case 0x58: ///< 已接收到最后一个数据，NACK已返回
            pu8Data[u8i++] = I2C_ReadByte(I2CX);
            I2C_SetFunc(I2CX, I2cStop_En); ///< 发送停止条件
            break;
        case 0x38:                          ///< 在发送地址或数据时，仲裁丢失
            I2C_SetFunc(I2CX, I2cStart_En); ///< 当总线空闲时发起起始条件
            break;
        case 0x48:                          ///< 发送SLA+R后，收到一个NACK
            I2C_SetFunc(I2CX, I2cStop_En);  ///< 发送停止条件
            I2C_SetFunc(I2CX, I2cStart_En); ///< 发送起始条件
            break;
        default:
            I2C_SetFunc(I2CX, I2cStart_En); ///< 其他错误状态，重新发送起始条件
            break;
        }
        I2C_ClearIrq(I2CX); ///< 清除中断状态标志位
        if (u8i == u32Len)  ///< 数据全部读取完成，跳出while循环
        {
            break;
        }
    }
    enRet = Ok;
    return enRet;
}

/**
 ******************************************************************************
 ** \brief  主机发送函数
 **
 ** \param u8Addr从机内存地址，pu8Data写数据，u32Len写数据长度
 **
 ** \retval 写数据是否成功
 **
 ******************************************************************************/
en_result_t I2C_MasterWriteData(M0P_I2C_TypeDef *I2CX, uint8_t slaveaddr, uint8_t u8Addr, uint8_t *pu8Data, uint32_t u32Len)
{
    int timeout = 0;
    en_result_t enRet = Error;
    uint8_t u8i = 0, u8State, u8Flag = FALSE;
    I2C_SetFunc(I2CX, I2cStart_En);
    while (1)
    {
        while (0 == I2C_GetIrq(I2CX))
        {
            timeout++;
            if (timeout > 10000)
                return Error;
        }
        u8State = I2C_GetState(I2CX);
        switch (u8State)
        {
        case 0x08: ///< 已发送起始条件
            I2C_ClearFunc(I2CX, I2cStart_En);
            I2C_WriteByte(I2CX, (slaveaddr << 1)); ///< 从设备地址发送
            break;
        case 0x18:                       ///< 已发送SLA+W，并接收到ACK
            I2C_WriteByte(I2CX, u8Addr); ///< 从设备内存地址发送
            // if (pu8Data == NULL)         // 发送的数据为空
            // {
            //     I2C_SetFunc(I2CX, I2cStop_En); ///< 出停止条件
            //     u8Flag = TRUE;
            // }
            break;
        case 0x28:                                  ///< 上一次发送数据后接收到ACK
            if ((pu8Data == NULL) || (u32Len == 0)) // 发送的数据为空
            {
                I2C_SetFunc(I2CX, I2cStop_En); ///< 出停止条件
                u8Flag = TRUE;
                break;
            }
            if (u8i < u32Len)
            {
                I2C_WriteByte(I2CX, pu8Data[u8i++]); ///< 继续发送数据
            }
            else
            {
                I2C_SetFunc(I2CX, I2cStop_En); ///< 出停止条件
                u8Flag = TRUE;
            }
            break;
        case 0x20:                         ///< 上一次发送SLA+W后，收到NACK
            I2C_SetFunc(I2CX, I2cStop_En); ///< 发送停止条件
            I2C_ClearIrq(I2CX);            ///< 清除中断状态标志位
            return enRet;
        case 0x38:                          ///< 上一次在SLA+读或写时丢失仲裁
            I2C_SetFunc(I2CX, I2cStart_En); ///< 当I2C总线空闲时发送起始条件
            break;
        case 0x30:                         ///< 已发送I2Cx_DATA中的数据，收到NACK，将传输一个STOP条件
            I2C_SetFunc(I2CX, I2cStop_En); ///< 发送停止条件
            break;
        default:
            break;
        }

        I2C_ClearIrq(I2CX); ///< 清除中断状态标志位

        if (u8Flag == TRUE) ///< 数据发送完成
        {
            break;
        }
    }
    enRet = Ok;
    return enRet;
}

/**
 ******************************************************************************
 ** \brief  主机写后紧读（Sr，无中间 STOP），适用于 KTH5702 等
 **
 ** \param  pu8WrData  SLA+W 之后连续发出的数据；pu8RdData 为 Sr+SLA+R 后读入
 **
 ******************************************************************************/
en_result_t I2C_MasterWriteThenRead(M0P_I2C_TypeDef *I2CX, uint8_t slaveaddr, const uint8_t *pu8WrData, uint32_t u32WrLen, uint8_t *pu8RdData, uint32_t u32RdLen)
{
    int timeout = 0;
    en_result_t enRet = Error;
    uint32_t u8WrIdx = 0u;
    uint32_t u8RdIdx = 0u;
    uint8_t u8State;

	if ((pu8WrData == NULL) || (u32WrLen == 0u) || (pu8RdData == NULL) || (u32RdLen == 0u)) //写入数据为空  数据长度为空 读长度为空都返回错误
    {
        return Error;
    }

    I2C_SetFunc(I2CX, I2cStart_En);//使能iic模块

    while (1)
    {
        while (0 == I2C_GetIrq(I2CX))
        {
            timeout++;
            if (timeout > 10000)
            {
                return Error;
            }
        }
        u8State = I2C_GetState(I2CX);//返回状态，这个状态通过查数据手册来判断
        switch (u8State)
        {
        case 0x08: ///< 起始已发，发 SLA+W
            I2C_ClearFunc(I2CX, I2cStart_En);
            I2C_WriteByte(I2CX, (uint8_t)(slaveaddr << 1));
            break;
        case 0x18: ///< SLA+W 已 ACK，发首字节
            I2C_WriteByte(I2CX, pu8WrData[u8WrIdx++]);
            break;
        case 0x20: ///< SLA+W NACK
            I2C_SetFunc(I2CX, I2cStop_En);
            I2C_ClearIrq(I2CX);
            return enRet;
        case 0x28: ///< 已发一字节并 ACK
            if (u8WrIdx < u32WrLen)
            {
                I2C_WriteByte(I2CX, pu8WrData[u8WrIdx++]);
            }
            else
            {
                I2C_SetFunc(I2CX, I2cStart_En); ///< 重复起始，进入读相
            }
            break;
        case 0x10: ///< 重复起始已发
            I2C_ClearFunc(I2CX, I2cStart_En);
            I2C_WriteByte(I2CX, (uint8_t)((slaveaddr << 1) | 0x01u));
            break;
        case 0x40: ///< SLA+R 已 ACK
            if (u32RdLen > 1u)
            {
                I2C_SetFunc(I2CX, I2cAck_En);
            }
            break;
        case 0x50: ///< 收到数据并已 ACK
            pu8RdData[u8RdIdx++] = I2C_ReadByte(I2CX);
            if (u8RdIdx == (u32RdLen - 1u))
            {
                I2C_ClearFunc(I2CX, I2cAck_En);
            }
            break;
        case 0x58: ///< 最后一字节，NACK 已发
            pu8RdData[u8RdIdx++] = I2C_ReadByte(I2CX);
            I2C_SetFunc(I2CX, I2cStop_En);
            break;
        case 0x38:
            I2C_SetFunc(I2CX, I2cStart_En);
            break;
        case 0x48:
            I2C_SetFunc(I2CX, I2cStop_En);
            I2C_SetFunc(I2CX, I2cStart_En);
            break;
        default:
            I2C_SetFunc(I2CX, I2cStart_En);
            break;
        }
        I2C_ClearIrq(I2CX);
        if (u8RdIdx == u32RdLen)
        {
            break;
        }
    }
    enRet = Ok;
    return enRet;
}






























