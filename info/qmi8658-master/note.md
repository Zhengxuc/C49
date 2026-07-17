I2C接口：I2C3
base_moduke
hd_i2c

芯片SCL max 400kHz

## 复位
上电复位：  
通过将 VDD 和 VDDIO 线从断电状态（VDD = 0V，VDDIO = 0V）驱动到有效工作范围来初始化上电复位。 详见 3.2。 上电复位过程从 VDDIO 开始，并且 VDD 在最终电平的 1% 以内。    
触发复位（上电复位和软件复位）后，QMI8658 将运行复位过程。 UI 寄存器、内部 RAM、FIFO 将设置为默认值，模拟和数字电路将被禁用。

3.2 推荐工作条件
|符号|      参数      |MIN|TYPE|MAX|单位|
|:--:|:--:|:--:|:--:|:--:|:--:|
|VDD|供电电压|1.71|1.8|3.6|V|
|VDDIO|IO脚供电电压|1.71|1.8|3.6|V|
|V(IL)|数字低电平输入电压| | |0.3*VDDIO|V|
|V(IH)|数字高电平输入电压|0.7*VDDIO| |0.3+VDDIO|V|
|V(OL)|数字低电平输出电压| | |0.1*VDDIO|V|
|V(OH)|数字高电平输出电压|0.9*VDDIO| | |V|

## 校准(COD)
支持陀螺仪X轴和Y轴的按需校准。 基于内部集成功能，QMI8658A可以校准陀螺仪X轴和Y轴的内部增益，从而获得更精确的灵敏度，并在QMI8658A芯片上更紧密地分布X轴和Y轴灵敏度。
请注意，<font color="red"> 陀螺仪的 Z 轴不受 COD 影响。</font>
### 校准步骤
1. 设置 CTRL7.aEN = 0 和 CTRL7.gEN = 0，禁用加速度计和陀螺仪。
2. 通过 CTRL9 命令发出 CTRL_CMD_ON_DEMAND_CALIBRATION (0xA2)。
3. 等待约 1.5 秒让 QMI8658A 完成 CTRL9 命令。
4. 读取 COD_STATUS 寄存器 (0x46) 以检查 COD 实现的结果/状态。    

### 校准注意事项：  
在此过程中，建议将设备置于安静状态，否则 COD 可能会失败并报错。
如果成功，之后重新校准的增益参数将应用于传感器数据。 更新后的增益输出到 UI 寄存器，主机可以读取，参见 14.3。 如果执行上电复位或软复位，重新校准的增益参数将丢失，QMI8658A 将使用片上默认增益参数。
如果失败，不影响陀螺仪的运行，QMI8658A 将继续使用之前的可用参数（最后一次成功的 COD 参数或片内默认参数）。  

### 校准状态指示
如果 COD 命令成功执行，COD_STATUS 寄存器将输出 0x00 进行指示
### 校准参数更新
成功 COD 后，经过 COD 校正的新增益将应用于陀螺仪 X 和 Y 轴的未来数据。 同时，新的参数会更新到下面的寄存器中，供主机读取和保存。
1.	Gyro-X gain (16 bits) will be in dVX_L and dVX_H registers (0x51, 0x52)
2.	Gyro-Y gain (16 bits) will be in dVY_L and dVY_H registers (0x53, 0x54)
3.	Gyro-Z gain (16 bits) will be in dVZ_L and dVZ_H registers (0x55, 0x56)

如果主机保存了这些增益参数，则可以使用 CTRL9 命令 CTRL_CMD_APPLY_GYRO_GAINS (0xAA) 将它们传回 QMI8658A（无需再次调用 COD 例程），如下所示：
1. 通过设置 CTRL7.aEN = 0 和 CTRL7.gEN = 0 禁用加速度计和陀螺仪
2. 将 Gyro-X 增益（16 位）写入寄存器 CAL1_L 和 CAL1_H 寄存器（0x0B，0x0C）
3. 将 Gyro-Y 增益（16 位）写入寄存器 CAL2_L 和 CAL2_H 寄存器（0x0D，0x0E）
4. 将 Gyro-Z 增益（16 位）写入寄存器 CAL3_L 和 CAL3_H 寄存器（0x0F，0x10）
5. 将 0xAA 写入 CTRL9 并遵循 CTRL9 协议

一旦 CTRL9 命令成功完成，恢复的增益将对 Gyroscope 的未来数据生效。
请注意，始终建议不时运行 COD 以应用 Gyro-X 和 Gyro-Y 灵敏度的精确和最新校正。 设计人员应小心恢复过时的增益参数，尤其是当 PCB 应力发生显着变化时。

## 自检
### 加速度计自检
加速度计自检 (Check-Alive) 用于确定加速度计是否正常工作并在可接受的参数范围内工作。
它是通过施加静电力来驱动加速度计的三个 X、Y 和 Z 轴中的每一个来实现的。 如果加速度计机械结构通过感应至少 200 mg 来响应此输入刺激，则可以认为加速度计是正常工作的。
加速度计自检数据可在寄存器 dVX_L、dVX_H、dVY_L、dVY_H、dVZ_L 和 dVZ_H 中读取。 主机可以通过以下程序随时启动自检。
加速度计自检程序：
1. 禁用传感器 (CTRL7 = 0x00)。
2. 将适当的加速度计 ODR (CTRL2.aODR) 和位 CTRL2.aST (bit7) 设置为 1 以触发自检。
3. 等待 QMI8658A 将 INT2 驱动为高电平，如果 INT2 已启用（CTRL1.bit4 = 1），或者 STATUSINT.bit0 设置为 1。
4. 将 CTRL2.aST(bit7) 设置为 0，以清除 STATUSINT1.bit0 和/或 INT2。
5. 检查 QMI8658A 是否将 INT2 驱动回低电平，并将 STATUSINT1.bit0 设置为 0。
6. 阅读 Accel 自检结果：
 * X channel: dVX_L and dVX_H (registers 0x51 and 0x52)  
 * Y channel: dVY_L and dVY_H (registers 0x53 and 0x54)   
 * Z channel: dVZ_L and dVZ_H (registers 0x55 and 0x56)

 结果为 16 位，格式为 U5.11，分辨率为 0.5mg (1 / 2^11 g)。
  如果所有三个轴的绝对结果都高于 200mg，则可以认为加速度计功能正常。 否则，加速度计不能被认为是有效的。
请注意，自检功能会自动将 full-scall 设置为 16g，并使用用户设置的 aODR (CTRL2.aODR)。 在自检结束时，QMI8658A 将在启动 Check-Alive) 例程之前使用用户设置的原始值更新 CTR2。
自检的典型时间（从将 aST 设置为 1，直到启用 INT2 的上升沿，或 STATUSINT.bit0 设置为 1）大约需要 25 个 ODR：
* 25ms @ 1KHz ODR   
* 800ms @ 32Hz ODR   
* 2.2s @ 11Hz ODR 

### 陀螺仪自检
陀螺仪自检 (Check-Alive) 用于确定陀螺仪是否正常工作。
它是通过施加静电力来驱动陀螺仪的三个 X、Y 和 Z 轴中的每一个并测量相应 X、Y 和 Z 轴上的机械响应来实现的。 如果陀螺仪输出的等效幅度对于每个轴大于 300dps，则可以认为该陀螺仪正常工作。
陀螺仪自检数据可在输出寄存器 dVX_L、dVX_H、dVY_L、dVY_H、dVZ_L 和 dVZ_H 中读取。 主机可以通过以下程序随时启动自检。
陀螺仪自检程序：
1. 禁用传感器 (CTRL7 = 0x00)。
2. 将位 gST 设置为 1。（CTRL3.bit7 = 1'b1）。
3. 等待 QMI8658A 将 INT2 驱动为高电平，如果 INT2 已启用，或者 STATUSINT.bit0 设置为 1。
4. 将 CTRL3.aST(bit7) 设置为 0，以清除 STATUSINT1.bit0 和/或 INT2。
5. 检查 QMI8658A 是否将 INT2 驱动回低电平，或将 STATUSINT1.bit0 设置为 0。
6. 读取陀螺仪自检结果：
* X channel: dVX_L and dVX_H (registers 0x51 and 0x52)   
* Y channel: dVY_L and dVY_H (registers 0x53 and 0x54)   
* Z channel: dVZ_L and dVZ_H (registers 0x55 and 0x56) 

以 U12.4 格式读取 16 位结果，分辨率为 62.5mdps (1 / 2^4 dps)。
如果所有三个轴的绝对结果都高于 300dps，则可以认为陀螺仪功能正常。 否则，陀螺仪不能被认为是功能性的。
请注意，自检功能会自动设置 CTRL3 的满量程 (gFS) 和 ODR (gODR)。 在自检结束时，QMI8658A 将在开始自检程序之前将 CTR3 更新为用户设置的原始值。
自检过程的典型时间（从将 gST 写入 1，直到 INT2 的上升沿（如果启用，或 STATUSINT.bit0 设置为 1））的成本约为 400 毫秒。

## Ctrl9
### 写Ctrl9
1. 主机需要将此命令所需的数据提供给 QMI8658A。 主机通常通过将数据放置在一组称为 CAL 寄存器的寄存器中来实现这一点。 最多使用八个 CAL 寄存器。 请参阅表 29。
2. 使用适当的命令值写入 Ctrl9 寄存器 0x0A，请参见表 28。
3. 一旦根据命令值执行了相应的功能，设备将设置 STATUSINT.bit7 为 1，并提高 INT1（如果 CTRL1.bit3 = 1 & CTRL8.bit7 == 0）。
4. 主机必须通过将 CTRL_CMD_ACK (0x00) 写入 CTRL9 寄存器来确认这一点，STATUSINT.bit7 (CmdDone) 将在收到 CTRL_CMD_ACK 命令时重置为 0。 如果 CTRL1.bit3 = 1 & CTRL8.bit7 == 0，则 INT1 在寄存器读取时被拉低。
5. 如果设备需要任何数据，此时将可用。 为每个命令单独指定数据的位置。

### 读Ctrl9
1. 使用适当的命令值写入 Ctrl9 寄存器 0x0A。
2. 一旦根据命令值执行了相应的功能，设备将设置 STATUSINT.bit7 为 1，并提高 INT1（如果 CTRL1.bit3 = 1 & CTRL8.bit7 == 0）。
3. 主机必须通过将 CTRL_CMD_ACK (0x00) 写入 CTRL9 寄存器来确认这一点，STATUSINT.bit7 (CmdDone) 将在收到 CTRL_CMD_ACK 命令时复位为 0。 如果 CTRL1.bit3 = 1 & CTRL8.bit7 == 0，则 INT1 在寄存器读取时被拉低。
4. 数据可从设备的 CAL 寄存器中获得。 为每个命令单独指定数据的位置。

### Ctrl9详细命令说明
* CTRL_CMD_ACK(0x00)  
主机在收到 CmdDone 信息时确认 QMI8658A，以结束 CTRL9 协议。
* CTRL_CMD_RST_FIFO(0x04)  
将 0x04 写入 Ctrl9 寄存器 0x​​0a 的 CTRL9 命令允许主机指示设备重置 FIFO。 FIFO 数据、样本计数和标志将被清除并重置为默认状态。
* CTRL_CMD_REQ_FIFO(0x05)  
当主机想要通过 CTRL9 进程写入 0x05 从 FIFO 中获取数据时，会发出此 CTRL9 命令。
成功完成 CTRL9 过程后，将启用 FIFO 读取模式，设备将 FIFO 数据定向到 FIFO_DATA 寄存器（0x17），直到 FIFO 为空。读取 FIFO 数据后，主机必须通过写入 FIFO_CTRL 寄存器将 FIFO_CTRL.FIFO_rd_mode 设置为 0，这将导致 FIFO_STATUS.FIFO_WTM/FIFO_FULL 被清除和/或 INT 引脚（如果启用）被取消断言。请参阅错误！未找到参考来源。错误！未找到参考来源。 CTRL9 操作，详见 8.8。
* CTRL_CMD_WRITE_WOM_SETTING(0x08)  
当主机想要启用/修改设备的运动唤醒功能的触发阈值或消隐间隔时，会发出此 CTRL9 命令。有关设置此功能的详细信息，请参阅 12 运动唤醒 (WoM)。一旦指定的 CAL 寄存器加载了适当的数据，就会通过将 0x08 写入 CTRL9 寄存器 0x​​0A 来发出命令。
* CTRL_CMD_ACCEL_HOST_DELTA_OFFSET(0x09) 
当主机想要手动更改加速度计偏移时，会发出此 CTRL9 命令。 每个增量偏移值应包含 16 位，格式为有符号 4.12（12 小数位，单位为 1 / 2^12）。 用户必须将偏移量写入以下寄存器：
  * Accel_Delta_X : {CAL1_H, CAL1_L} 
  * Accel_Delta_Y : {CAL2_H, CAL2_L} 
  * Accel_Delta_Z : {CAL3_H, CAL3_L}  
  接下来，通过将 0x09 写入 CTRL9 寄存器 0x0A 来发出命令。 请注意，当传感器重新上电或系统重置时，此偏移更改会丢失。
* CTRL_CMD_GYRO_HOST_DELTA_OFFSET(0x0A)  
当主机想要手动更改陀螺仪偏移时发出此 CTRL9 命令。 每个增量偏移值应包含 16 位，格式为有符号 11.5（5 个小数位，单位为 1 / 2^5）。 用户必须将偏移量写入以下寄存器：
  * Gyro_Delta_X : {CAL1_H, CAL1_L} 
  * Gyro_Delta_Y : {CAL2_H, CAL2_L} 
  * Gyro_Delta_Z : {CAL3_H, CAL3_L}  
接下来，通过将 0x0A 写入 CTRL9 寄存器 0x0A 来发出命令。 请注意，当传感器重新上电或系统重置时，此偏移更改会丢失。
* CTRL_CMD_CONFIGURE_TAP(0x0C)  
此 CTRL9 命令用于配置 Tap 检测参数。 有关详细信息，请参阅 10.3 配置 Tap。
* CTRL_CMD_CONFIGURE_PEDOMETER(0x0D)  
发出此 CTRL9 命令以配置计步器检测的参数。 有关详细信息，请参阅 11.2 配置计步器。
* CTRL_CMD_CONFIGURE_MOTION(0x0E)  
发出此 CTRL9 命令以配置运动检测的参数。 请参阅 9.4 配置移动侦测。
* CTRL_CMD_RESET_PEDOMETER(0x0F)
发出此 CTRL9 命令以清除计步器的步数。 有关详细信息，请参阅 11.6 重置步数。
* CTRL_CMD_COPY_USID(0x10)
USID 是每个 QMI8658A 部件的唯一 ID。
此 CTRL9 命令将以下数据复制到 UI 寄存器中。 它由主机将 0x10 写入 CTRL9 来启动。 发出命令后，主机可以从如下所示的寄存器中读取数据：  
FW_Version byte 0 → dQW_L  
FW_Version byte 1 → dQW_H  
FW_Version byte 2 → dQX_L  
USID_Byte_0 → dVX_L  
USID_Byte_1 → dVX_H  
USID_Byte_2 → dVY_L  
USID_Byte_3 → dVY_H  
USID_Byte_4 → dVZ_L  
USID_Byte_5 → dVZ_H  
请注意，在上电复位或软复位成功后，FW_Version 和 USID 将自动复制到相应的寄存器一次，以供主机读取。 这些寄存器可以在启用传感器后更改，因此在读取之前应执行 CTRL_CMD_COPY_USID 命令将 FW_Version 和 USID 复制到相应的寄存器中。
* TRL_CMD_SET_RPU(0x11)
此 CTRL9 命令在主机配置 IO 上拉电阻时发出。 每个位控制一个电阻器组合，如表 30 所示：  

|Bit|名称|pin脚|说明|默认值|
|:--:|:--:|:--:|:--:|:--:|
|0|aux_rpu_dis|SDx,SCx,RESV-NC(pin10)|0:启用上拉 1：禁用上拉|0|
|1|icm_rpu_dis|SDx|0:启用上拉 1：禁用上拉|0|
|2|cs_rpu_dis|CS|0:启用上拉 1：禁用上拉|0|
|3|i2c_rpu_dis|SCL, SDA|0:启用上拉 1：禁用上拉|0|
|4:7|保留|NA|||

主机通过发出带有 0x11 的 WCtrl9 命令写入适当的 CAL1_L 位。
默认情况下，启用所有上拉电阻。 向该位写入 1 将相应地禁用上拉电阻，而写入 0 将启用上拉电阻。
* CTRL_CMD_AHB_CLOCK_GATING(0x12)  
当设置了锁定机制（CTRL7.bit7 == 1(syncSmpl)）时，应该禁用CTRL_CMD_AHB_CLOCK_GATING，以保证数据读取的锁定机制，防止可能的错位。 有关详细信息，请参阅 14 按需校准 (COD)。
* CTRL_CMD_ON_DEMAND_CALIBRATION(0xA2)  
此 CTRL9 命令使主机能够不时重新校准陀螺仪灵敏度。 请参阅 14 按需校准 (COD)。
* CTRL_CMD_APPLY_GYRO_GAINS(0xAA)  
此 CTRL9 命令使主机能够将保存的陀螺仪增益恢复到 QMI8658A，以避免再次运行 COD。 当环境发生显着变化时不建议这样做，例如显着的 PCB 应力变化。 请参阅 14.4 保存和恢复新的增益参数。

## 中断
QMI8658A 有两条中断线，INT1 和 INT2。
通过配置 CTRL1.bit3(INT1) 或 CTRL1.bit4(INT2)，可以将 INT1 和 INT2 配置为 High-Z 模式或 Push-Pull 模式。如果 CTRL1.bit3 (CTRL1.bit4) 设置为 0，则 INT1(INT2) 将相应地设置为 High-Z 模式。而如果 CTRL1.bit3 (CTRL1.bit4) 设置为 1，则 INT1(INT2) 将相应地设置为 Push-Pull 模式。默认情况下，INT1 和 INT2 处于高阻模式。
如果 QMI8658A 配置为运动唤醒 (WoM) 模式，则不会生成传感器数据。 INT 引脚行为遵循 WoM 的配置，请参阅 12 运动唤醒 (WoM)。
如果 QMI8658A 未处于运动唤醒模式，则中断映射有两种模式，如下所述。主机可以将多个内部信号/中断源配置到 INT 引脚（INT1 和/或 INT2）。如果驱动到一个 INT 引脚，则多个源在 LOGIC-OR 中起作用。
### 同步采样模式
SyncSample 模式支持在读取过程中锁定值。请参阅 13 锁定机构有关传感器数据寄存器的详细信息。

设置 CTRL7.bit7(SyncSample) == 1 将启用 SyncSample 模式。
如图 12 所示。在 SyncSample 模式下，CTRL9 握手信号将被路由到 INT1。详情请查看 5.10。
如果启用，运动事件中断（任何运动、无运动、显着运动、计步器、轻敲）将路由到 INT1。
该模式不支持 FIFO 功能，DRDY 信号将被路由到 INT2。
### 非同步采样模式
该模式支持 FIFO 功能和自由中断配置，如图 13 所示。
如果 CTRL7.bit7(SyncSample) == 0，则 STATUSINT 寄存器的第 1 位将具有与 INT1 相同的值，而 STATUSINT 寄存器的第 0 位将具有与 INT2 相同的值。
在 Non-SyncSample 模式下，CTRL9 握手有两种方法。如果设置 CTRL8.bit7 = 0，主机可以检查 INT1 引脚是否为握手信号的高电平；如果设置 CTRL8.bit7 = 1，则轮询 STATUSINT.bit7 进行握手。
在 Non-SyncSample 模式下，可以通过设置 CTRL8.bit6 = 1 将运动事件中断配置为 INT1，或者通过设置 CTRL8.bit6 = 0 将运动事件中断配置为 INT2。请注意，运动事件引擎可以通过 CTRL8.bit 启用[4:0]，详见表 22。
在 Non-SycnSample 模式下，传感器数据可以通过数据寄存器或 FIFO 输出。配置 FIFO_CTRL.FIFO_MODE = ‘bypass’ 模式，将启用 DRDY 功能并禁用 FIFO 功能；配置 FIFO_CTRL.FIFO_MODE = other 模式，将启用 FIFO 功能并禁用 DRDY 功能。
如果启用 FIFO 模式，如果 CTRL1.bit2 设置为 1，则 FIFO 中断可以配置到 INT1 引脚，如果 CTRL1.bit2 设置为 0，则可以将 FIFO 中断配置到 INT2 引脚。有关 FIFO 中断行为的更多详细信息，请参阅 8 FIFO 说明。
如果启用 DRDY 模式，如果 CTRL7.bit5(DRDY_DIS) 设置为 0，则 DRDY 信号将被路由到 INT2，如果 CTRL7.bit5(DRDY_DIS) 设置为 1，则 DRDY 信号将被路由到 INT2 引脚。


|cmd|value|addr|time max(ms)|flag addr|flag value|
|:--:|:--:|:--:|:--:|:--:|:--:|
|reset| 0x0B|0x60|15|0x4D|0x80|