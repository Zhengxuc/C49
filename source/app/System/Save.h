/*
 * @Author: XinXin
 * @Date: 2023-09-10 22:53:07
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-14 11:49:42
 * @FilePath: \source\app\System\Save.h
 * @Description: 
 * 修改说明: 移除EEPROM依赖，改用内部Flash存储
 */

#ifndef __SAVE_H__
#define __SAVE_H__

#include "common.h"
#include "flash.h"

// Flash参数存储区域配置
#define SAVE_FLASH_START_ADDR       (0x40000 -0x800) // 使用Flash末尾区域，根据实际MCU调整
#define SAVE_FLASH_SECTOR_SIZE      0x200            // 扇区大小(0.5KB)，根据实际MCU调整
#define SAVE_SYSTEM_DATA_SIZE       256              // 系统参数大小
#define SAVE_ADJUST_DATA_SIZE       256              // 校准参数大小
#define SAVE_LOG_DATA_SIZE          256              // 日志数据大小
#define SAVE_TOTAL_SIZE             (SAVE_SYSTEM_DATA_SIZE + SAVE_ADJUST_DATA_SIZE + SAVE_LOG_DATA_SIZE)//总大小 系统+调整参数+日志

// 参数偏移地址
#define SAVE_SYSTEM_DATA_OFFSET     0           // 系统参数偏移
#define SAVE_ADJUST_DATA_OFFSET     SAVE_SYSTEM_DATA_SIZE   // 校准参数偏移
#define SAVE_LOG_DATA_OFFSET        (SAVE_SYSTEM_DATA_SIZE + SAVE_ADJUST_DATA_SIZE)  // 日志偏移

// 参数有效标志
#define SAVE_VALID_FLAG             0x5A5A5A5A  // 有效标志
#define SAVE_INVALID_FLAG           0xFFFFFFFF  // 无效标志

// 兼容旧版地址定义（实际映射到Flash偏移）
#define SELECT_ADDRESS(page, adder) ((page) + (adder))

//所有的参数是否在这里定义？
#define SAVE_ADJUST_ANGLE           (SAVE_ADJUST_DATA_OFFSET + 0)      // 角度保存地址0-31 32byte
#define SAVE_ADJUST_CURRENT         (SAVE_ADJUST_DATA_OFFSET + 32)     // IP电流保存地址32-63 32byte
#define SAVE_ADJUST_SPEED           (SAVE_ADJUST_DATA_OFFSET + 64)     // 速度保存地址64-95 32byte
#define SAVE_ADJUST_OVERSHOOT       (SAVE_ADJUST_DATA_OFFSET + 96)     // 过冲参数保存地址96-127 32byte
#define SAVE_INPUT_CURRENT          (SAVE_ADJUST_DATA_OFFSET + 128)    // 输入电流校准地址128-159 32byte

//
#define SAVE_INPUT_CURRENT          (SAVE_ADJUST_DATA_OFFSET + 128)    // 输入电流校准地址128-159 32byte








// 系统参数索引（对应Flash中的偏移）
enum Save_System
{
    SAVE_BACKLIGHT = 0,         // 亮度
    SAVE_BACKLIGHT_MODE,        // 背光模式
    SAVE_CONTRAST_RATIO,        // 对比度
    SAVE_LANGUAGE,              // 语言
    SAVE_DISPLAY_MIRROR,        // 屏幕镜像
    SAVE_ERROR_RANGE,           // 误差范围
    SAVE_CONTROL_MODE,          // 控制模式
    SAVE_SYSTEM_PARAM_COUNT     // 参数个数（必须放在最后）
};

/* 控制模式（存储于 SAVE_CONTROL_MODE） */
typedef enum
{
    CONTROL_MODE_CURRENT_PERCENT = 0, // 4-20mA 电流百分比（自动）
    CONTROL_MODE_BUTTON_PERCENT,      // 按键百分比设定
    CONTROL_MODE_BUTTON_MOVE,         // 按键充排气移动
    CONTROL_MODE_RS485_PERCENT,       // RS485 设定（预留）
    CONTROL_MODE_HART_PERCENT,        // HART 设定（预留）
    CONTROL_MODE_END
} control_mode_t;

/* 控制模式读写（含合法性检查，非法值回落 CURRENT_PERCENT） */
int get_inpu_mode(void);
void save_inpu_mode(int mode);

// 系统参数头结构（用于校验）
typedef struct
{
    uint32_t validFlag;                         // 有效标志
    uint32_t crc32;                             // CRC校验
    float systemParams[SAVE_SYSTEM_PARAM_COUNT]; // 系统参数
} system_param_header_t;

void Save_System_Init(void);
void Save_System(enum Save_System index, float32_t dat);
float32_t Read_System(enum Save_System index);
int8_t Save_System_block(uint32_t addr, void *dat, uint32_t len);
int8_t Read_System_block(uint32_t addr, void *dat, uint32_t len);

/* 输入电流校准参数（4mA / 20mA 两点标定） */
#define INPUT_CALIB_VALID_FLAG     0xA5A5A5A5u

typedef struct
{
    uint32_t validFlag;      /* 有效标志 INPUT_CALIB_VALID_FLAG */
    int32_t  raw_4ma;        /* 4mA 对应的 ADX111 raw 值 */
    int32_t  raw_20ma;       /* 20mA 对应的 ADX111 raw 值 */
    uint8_t  calibrated;     /* 1=已校准, 0=未校准 */
    uint8_t  reserved[3];
} input_calibration_t;

void    save_input_calibration(int32_t raw_4ma, int32_t raw_20ma);
void    set_input_calibration(const input_calibration_t *cal);
void    get_input_calibration(input_calibration_t *cal);
int32_t input_current_raw_to_ma(int32_t raw);  /* raw → mA 整数 */
float   input_current_raw_to_ma_float(int32_t raw);  /* raw → mA 浮点 */

// Flash初始化
int8_t Save_Flash_Init(void);
// 保存所有系统参数到Flash
int8_t Save_System_Params_To_Flash(void);
// 从Flash读取所有系统参数
int8_t Load_System_Params_From_Flash(void);

#endif