/*
 * @Author: XinXin
 * @Date: 2023-09-10 22:53:07
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-14 11:49:42
 * @FilePath: \source\app\System\Save.h
 * @Description: 参数持久化存储模块（内部 Flash 实现）
 * 修改说明: 移除EEPROM依赖，改用内部Flash存储
 *
 * ============================== 模块总述 ==============================
 * 本模块管理掉电不丢失的用户参数，物理载体为 MCU 内部 Flash 末尾
 * 2KB（4 个 512B 扇区），逻辑上划分为三个互不干扰的存储区域：
 *
 *   1. 系统区（512B）—— 同构参数：13 个 float，整体 CRC32 校验
 *      适用：屏幕亮度/对比度、语言、PID 参数等"数值型"参数
 *      API：Save_System / Read_System（enum 索引寻址）
 *
 *   2. 校准区（512B）—— 异构参数：5 个 32B 槽位，各槽自带 validFlag
 *      适用：角度标定、电流标定等"结构体型"参数（长短不一）
 *      API：Save_System_block / Read_System_block（区内偏移寻址）
 *
 *   3. 日志区（512B）—— 预留
 *
 * ============================ 上电调用链 =============================
 *   main → system_HAL_Init → Save_System_Init（统一分发入口）
 *     ├─ 第零步：ValveParam_SetDefault()  灌硬编码兜底默认值
 *     ├─ 第一步：Save_Flash_Init()        Flash 硬件初始化 + 整区载入 RAM
 *     └─ 第二~五步：把 RAM 缓存分发到各模块运行参数（含合法性检查）
 *
 * ============================ 使用守则 ==============================
 *   - 写参数：运行中随时 Save_System / Save_System_block（自动同步 RAM+Flash）
 *   - 读参数：优先 Read_System / Read_System_block（纯 RAM 读，不碰 Flash）
 *   - 新增参数必须同步改两处：Save_Flash_Init 默认值 + Save_System_Init 分发
 *   - Flash 擦写有寿命（约 1 万次/扇区），避免在高频循环里调 Save
 */

#ifndef __SAVE_H__
#define __SAVE_H__

#include "common.h"
#include "flash.h"

/* ============================ Flash 物理布局 ============================
 * 256KB Flash 的最末 2KB，起始地址 0x3F800，按 512B/扇区独占划分：
 *
 *   0x3F800 ┌─────────────────────┐
 *           │ 系统区 (512B) 扇区A  │  validFlag + CRC32 + 13×float
 *   0x3FA00 ├─────────────────────┤
 *           │ 校准区 (512B) 扇区B  │  5 个 32B 槽位（角度/电流/速度/过冲/输入电流）
 *   0x3FC00 ├─────────────────────┤
 *           │ 日志区 (512B) 扇区C  │  预留
 *   0x3FE00 ├─────────────────────┤
 *           │ 剩余 (512B) 扇区D   │  预留（SAVE_TOTAL_SIZE=1536，实际占 3 个扇区）
 *   0x40000 └─────────────────────┘
 *
 * 选末尾的原因：程序代码从 0 开始向上增长，放末尾可最大限度避免
 * 代码膨胀覆盖参数区（链接脚本固定代码上限时尤其安全）。
 * ====================================================================== */
#define SAVE_FLASH_START_ADDR       (0x40000 - 0x800) // 参数区起始地址 = Flash 末尾 2KB 处
#define SAVE_FLASH_SECTOR_SIZE      0x200             // 扇区大小(0.5KB) = 本 MCU 最小擦除单位

/* 每个区域独占一个扇区（512B）：擦除以扇区为最小单位，
 * 若两个区域共用一扇区，擦任一个都会连带擦掉另一个。
 * （教训：曾因区域大小与扇区不匹配出现"当次会话正常、断电数据丢失"） */
#define SAVE_SYSTEM_DATA_SIZE       512              // 系统区大小（独占扇区A）
#define SAVE_ADJUST_DATA_SIZE       512              // 校准区大小（独占扇区B）
#define SAVE_LOG_DATA_SIZE          512              // 日志区大小（独占扇区C）
#define SAVE_TOTAL_SIZE             (SAVE_SYSTEM_DATA_SIZE + SAVE_ADJUST_DATA_SIZE + SAVE_LOG_DATA_SIZE) // 三区总大小

// 各区域相对 SAVE_FLASH_START_ADDR 的偏移（用于计算物理地址）
#define SAVE_SYSTEM_DATA_OFFSET     0                                        // 系统区偏移 → 0x3F800
#define SAVE_ADJUST_DATA_OFFSET     SAVE_SYSTEM_DATA_SIZE                    // 校准区偏移 → 0x3FA00
#define SAVE_LOG_DATA_OFFSET        (SAVE_SYSTEM_DATA_SIZE + SAVE_ADJUST_DATA_SIZE) // 日志区偏移 → 0x3FC00

/* ============================ 有效性校验 ============================
 * 双层校验：validFlag 快判"是否写过"，CRC32 细判"数据是否损坏"。
 *
 * v2 说明：系统参数新增 SAVE_ANGLE_MIN/SAVE_ANGLE_MAX 后 float 数组
 * 变长 → 旧数据的 CRC 必然失效。直接换有效标志值（5A→5B），让旧块
 * 按"首次使用"走默认值流程，相当于零成本的版本升级机制：
 *   - 枚举变长 → 数组变长 → 旧 CRC 必炸 → 换标志强制走默认值路径
 *   - 以后每次给系统区扩容参数，都应同步更新此标志值
 * v3：追加 SAVE_ACTUATOR_TYPE/BALANCE_LEVEL/REDUNDANT_ERROR/FRICTION_LEVEL
 *   共 4 项（13→17 float），标志 5B→5C，旧块按首次使用重建
 * v4：追加 SAVE_MECHANICAL_DIRECTION（17→18 float），标志 5C→5D
 * v5：追加 SAVE_MECHANICAL_MIN/MAX（18→20 float），标志 5D→5E
 * v6：追加 SAVE_FEEDBACK_DIRECTION（20→21 float），标志 5E→5F
 * v7：Kp/Ki/Kd 改为缩放整型语义（×10/×100），旧浮点值会被错误缩放 → 必须换标志重建
 * v8：存储数组 float → int32_t，全部参数统一整型语义（角度 ×100），标志 60→61
 * v9：追加 SAVE_PST_PERIOD（21→22 int32），标志 61→62
 * v10：追加 SAVE_SAFE_POSITION（22→23 int32），标志 62→63
 * v11：追加 SAVE_EXECUTION_TIMEOUT（23→24 int32），标志 63→64
 * v12：追加 SAVE_CONTROL_KP/KI/KD_DOWN（24→27 int32），标志 64→65 */
#define SAVE_VALID_FLAG             0x5A5A5A65  // 有效标志（写入头部第一个字段）
#define SAVE_INVALID_FLAG           0xFFFFFFFF  // 无效标志（= Flash 擦除后的空白态）

// 兼容旧版地址定义（实际映射到Flash偏移）
#define SELECT_ADDRESS(page, adder) ((page) + (adder))

/* ============================ 校准区槽位表 ============================
 * 以下为校准区"内部偏移"（0 ~ SAVE_ADJUST_DATA_SIZE-1），
 * 因为参数大小不一（结构体各异），用固定 32B 槽位的块存储方式。
 *
 * ★ 注意：Save/Read_System_block 内部会再加上区域基地址，
 *   此处是区内偏移，禁止写成 Flash 绝对地址或双重偏移
 *   （教训：双重偏移曾导致写入错误区域且 RAM 缓存不更新） */
#define SAVE_ADJUST_ANGLE           0      // 槽位0：角度标定     区内偏移 0-31   32B
#define SAVE_ADJUST_CURRENT         32     // 槽位1：IP电流参数   区内偏移 32-63  32B
#define SAVE_ADJUST_SPEED           64     // 槽位2：速度参数     区内偏移 64-95  32B
#define SAVE_ADJUST_OVERSHOOT       96     // 槽位3：过冲参数     区内偏移 96-127 32B
#define SAVE_INPUT_CURRENT          128    // 槽位4：输入电流校准 区内偏移 128-159 32B

/* 输入电流校准有效标志：独立于系统区 CRC，标定槽位自校验用 */
#define INPUT_CALIB_VALID_FLAG     0xA5A5A5A5u

/* ============================ 系统区参数索引 ============================
 * g_systemParams[] 数组下标，同时决定参数在 Flash 系统区中的排布顺序。
 *
 * ★ 扩容规则（重要）：
 *   1. 只能在 COUNT 之前追加，不能插入中间、不能删除（否则旧 Flash
 *      数据错位——虽然换 validFlag 能兜底走默认值，但会丢全部用户参数）
 *   2. 追加后必须同步三处：
 *      - Save_Flash_Init 里的默认值赋值（首次上电路径）
 *      - Save_System_Init 里的分发读取（上电分发路径）
 *      - data_Restore 恢复出厂默认（如需要）
 *   3. 系统区走"整块读写"：改一个参数 = 擦整个扇区重写 13 个，
 *      所以系统区适合低频修改的配置型参数 */
enum Save_System
{
    SAVE_BACKLIGHT = 0,         // 屏幕亮度（0~100，暂无 PWM 硬件接口）
    SAVE_BACKLIGHT_MODE,        // 背光模式（常亮/自动，半成品暂未分发）
    SAVE_CONTRAST_RATIO,        // LCD 对比度（lcd_set_contrast_ratio 生效）
    SAVE_LANGUAGE,              // 语言（0=中文，GUI 显示用）
    SAVE_DISPLAY_MIRROR,        // 屏幕镜像（0=正常）
    SAVE_ERROR_RANGE,           // 【遗留】允许误差范围 ×10（存 3 = 0.3%）；deadband 已改由 SAVE_REDUNDANT_ERROR 提供，本槽位不再分发
    SAVE_CONTROL_MODE,          // 控制模式（control_mode_t，非法值回落电流模式）
    SAVE_ANGLE_MIN,             // 阀位零位角度 ×100，单位 0.01°（存 1237 = 12.37°，自整定标定写入）
    SAVE_ANGLE_MAX,             // 阀位满位角度 ×100，单位 0.01°（默认存 9000 = 90.00°）
    /* v7 起 PID 参数为缩放整型语义（无 FPU 平台约定：小数值放大存储，分发时还原）：
     * KP ×10（存 50 = 实际 5.0）、KI/KD ×100（存 5 = 实际 0.05） */
    SAVE_CONTROL_KP,            // PID 比例系数 ×10（上行程，默认存 50 = 5.0）
    SAVE_CONTROL_KI,            // PID 积分系数 ×100（上行程，默认存 5 = 0.05）
    SAVE_CONTROL_KD,            // PID 微分系数 ×100（上行程，默认存 0 = 0.0）
    SAVE_CONTROL_KP_DOWN,       // PID 比例系数 ×10（下行程）
    SAVE_CONTROL_KI_DOWN,       // PID 积分系数 ×100（下行程）
    SAVE_CONTROL_KD_DOWN,       // PID 微分系数 ×100（下行程）
    /* ---- v3 追加（"1.Auto"页四项，只能在 COUNT 前追加，禁止插入/删除 ---- */
    SAVE_ACTUATOR_TYPE,         // 执行器类型：0=LT(直行程) 1=ROTS(角行程)
    SAVE_BALANCE_LEVEL,         // 性能平衡：0最快速度 1速度优先 2适中 3稳定优先 4最佳稳定
    SAVE_REDUNDANT_ERROR,       // 冗余误差：整型语义 3~100，对应 0.3%~10.0%（= deadband，单位 0.1%）
    SAVE_FRICTION_LEVEL,        // 阀门摩擦系数：0=低摩擦 1=高摩擦
    /* ---- v4 追加（"3.Mechanical State"页） ---- */
    SAVE_MECHANICAL_DIRECTION,  // 机械行程方向：0=正向 1=反向（→ ValveParam.reverse_fb）
    /* ---- v5 追加（"3.Mechanical State"页行程限位） ---- */
    SAVE_MECHANICAL_MIN,        // 机械行程最小值：0~1000（默认 0）
    SAVE_MECHANICAL_MAX,        // 机械行程最大值：0~1000（默认 1000）
    /* ---- v6 追加（"7.Feedback"页） ---- */
    SAVE_FEEDBACK_DIRECTION,    // 阀位反馈方向：0=正向 1=反向（→ ValveParam.feedback_dir）
    /* ---- v9 追加（"6.PST"页） ---- */
    SAVE_PST_PERIOD,            // 定时任务周期：打包为总分钟数（月×43200 + 时×60 + 分）
    /* ---- v10 追加（"8.System"页安全位置选择） ---- */
    SAVE_SAFE_POSITION,         // 故障模式：0=故障安全(FAIL_SAFE) 1=故障闭锁(FAIL_LOCK)
    /* ---- v11 追加（"8.System"页行程时间极限） ---- */
    SAVE_EXECUTION_TIMEOUT,     // 行程时间极限：10~200 秒直存（超时保护逻辑待挂接）
    SAVE_SYSTEM_PARAM_COUNT     // 参数个数（必须放最后，数组长度+CRC 覆盖范围）
};

/* ---- v3 参数的合法取值（存于 float 槽位，语义为整数档位） ---- */
/* 执行器类型（SAVE_ACTUATOR_TYPE） */
#define ACTUATOR_TYPE_LT        0   // 直行程（Linear Travel）
#define ACTUATOR_TYPE_ROTS      1   // 角行程（Rotary）
/* 性能平衡（SAVE_BALANCE_LEVEL） */
#define BALANCE_MAX_SPEED       0   // 最快速度
#define BALANCE_SPEED_FIRST     1   // 速度优先
#define BALANCE_DEFAULT         2   // 适中
#define BALANCE_STABLE_FIRST    3   // 稳定优先
#define BALANCE_MAX_STABLE      4   // 最佳稳定
#define BALANCE_COUNT           5
/* 冗余误差（SAVE_REDUNDANT_ERROR）：整型 3~100 ↔ 0.3%~10.0%，数值上直接等于 deadband */
#define REDUNDANT_ERROR_MIN     3
#define REDUNDANT_ERROR_MAX     100
/* 摩擦系数（SAVE_FRICTION_LEVEL） */
#define FRICTION_LOW            0   // 低摩擦
#define FRICTION_HIGH           1   // 高摩擦
/* 机械行程方向（SAVE_MECHANICAL_DIRECTION） */
#define MECH_DIR_POSITIVE       0   // 正向
#define MECH_DIR_REVERSED       1   // 反向（反馈阀位 0~1000 翻转）
/* 机械行程限位（SAVE_MECHANICAL_MIN/MAX）：阀位刻度 0~1000 */
#define MECH_TRAVEL_MIN         0       // 可调下限/最小值默认
#define MECH_TRAVEL_MAX         1000    // 可调上限/最大值默认
/* 阀位反馈方向（SAVE_FEEDBACK_DIRECTION） */
#define FB_DIR_POSITIVE         0   // 正向
#define FB_DIR_REVERSED         1   // 反向
/* PID 参数缩放（SAVE_CONTROL_KP/KI/KD）：存整型，用时下放 */
#define PID_KP_SCALE            10.0f    // Kp 放大倍数（1 位小数精度）
#define PID_KI_SCALE            100.0f   // Ki 放大倍数（2 位小数精度）
#define PID_KD_SCALE            100.0f   // Kd 放大倍数（2 位小数精度）
#define PID_KP_DEFAULT          50       // 5.0 ×10
#define PID_KI_DEFAULT          5        // 0.05 ×100
#define PID_KD_DEFAULT          0        // 0.0 ×100
/* 角度参数缩放（SAVE_ANGLE_MIN/MAX）：真实角度 ×100 存整数（0.01° 分辨率） */
#define ANGLE_SCALE             100.0f
#define ANGLE_MIN_DEFAULT       0        // 0.00°
#define ANGLE_MAX_DEFAULT       9000     // 90.00°
/* 定时任务周期（SAVE_PST_PERIOD）：总分钟数打包，月按 30 天 = 43200 分钟计 */
#define PST_MINUTES_PER_MONTH   43200    // 30 × 24 × 60
#define PST_MONTH_MAX           12       // 月可调上限
#define PST_HOUR_MAX            23
#define PST_MINUTE_MAX          59
#define PST_PERIOD_DEFAULT      0        // 默认 0 = 未启用
/* 故障模式（SAVE_SAFE_POSITION）：与 OutputDriver.h 的 drive_mode_t 取值一致 */
#define SAFE_POS_FAIL_SAFE      0        // 故障安全：PF05=高（默认）
#define SAFE_POS_FAIL_LOCK      1        // 故障闭锁：PF05=低
/* 行程时间极限（SAVE_EXECUTION_TIMEOUT）：秒，直存整数 */
#define EXEC_TIMEOUT_MIN        10
#define EXEC_TIMEOUT_MAX        200
#define EXEC_TIMEOUT_DEFAULT    60

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

/* ==================== 系统区 Flash 落盘结构（92B） ====================
 * Save_System_Params_To_Flash 按此结构整体写入 0x3F800：
 *
 *   偏移    字段             大小   说明
 *   0x00    validFlag        4B     SAVE_VALID_FLAG，快速判"是否写过"
 *   0x04    crc32            4B     覆盖下方参数数组的 CRC32
 *   0x08    systemParams[]   84B    21 个 int32，enum Save_System 顺序
 *
 * 上电校验：标志不等 → 首次上电灌默认值；CRC 不等 → 数据损坏同样灌默认值 */
typedef struct
{
    uint32_t validFlag;                            // 有效标志（SAVE_VALID_FLAG）
    uint32_t crc32;                                // CRC 校验值（只覆盖参数数组）
    int32_t systemParams[SAVE_SYSTEM_PARAM_COUNT]; // 系统参数（整型语义，顺序 = enum Save_System）
} system_param_header_t;

/* ==================== 对外 API（按功能分组） ==================== */

/* ---- 初始化链（上电顺序调用，通常只需调 Save_System_Init） ---- */
void Save_System_Init(void);                     // 统一分发入口：默认值→Flash载入→分发各模块
int8_t Save_Flash_Init(void);                    // Flash 硬件初始化 + 双区载入 RAM（含首次上电默认值）
int8_t Save_System_Params_To_Flash(void);        // 系统区 RAM 缓存 → Flash（整块写）
int8_t Load_System_Params_From_Flash(void);      // Flash → 系统区 RAM 缓存（带 CRC 校验）

/* ---- 系统区参数读写（enum 索引，运行期随时调用） ---- */
void Save_System(enum Save_System index, int32_t dat);   // 写单个参数（RAM+Flash 同步）
int32_t Read_System(enum Save_System index);             // 读单个参数（纯 RAM 读，不碰 Flash）

/* ---- 校准区块读写（区内偏移，运行期随时调用） ---- */
int8_t Save_System_block(uint32_t addr, void *dat, uint32_t len); // 写块（addr 为区内偏移）
int8_t Read_System_block(uint32_t addr, void *dat, uint32_t len);// 读块（纯 RAM 读，不碰 Flash）



/* ==================== 输入电流校准（校准区槽位4） ====================
 * "系统→电流标定"页面两点标定：信号源给 4mA / 20mA 各采一组 raw 均值，
 * 存入 Flash 校准区 SAVE_INPUT_CURRENT 偏移处（32B 槽位）。
 * 运行时 raw → mA 用两点线性插值换算。
 * 槽位自带 validFlag + calibrated 双标志，无需依赖整区 CRC。 */
typedef struct
{
    uint32_t validFlag;      /* 有效标志 INPUT_CALIB_VALID_FLAG */
    int32_t  raw_4ma;        /* 4mA 对应的 ADX111 raw 值 */
    int32_t  raw_20ma;       /* 20mA 对应的 ADX111 raw 值 */
    uint8_t  calibrated;     /* 1=已校准, 0=未校准 */
    uint8_t  reserved[3];    /* 对齐填充（凑 8 字节边界，留扩展） */
} input_calibration_t;

/* ---- 输入电流校准 API ---- */
void    save_input_calibration(int32_t raw_4ma, int32_t raw_20ma);   /* 标定完成：写 RAM+Flash */
void    set_input_calibration(const input_calibration_t *cal);      /* 只写 RAM 缓存（上电载入用） */
void    get_input_calibration(input_calibration_t *cal);            /* 拷出当前 RAM 缓存 */
int32_t input_current_raw_to_ma(int32_t raw);       /* raw → mA 整数（定点运算版） */
float   input_current_raw_to_ma_float(int32_t raw); /* raw → mA 浮点（显示用） */

#endif