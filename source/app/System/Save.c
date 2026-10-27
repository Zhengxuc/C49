/*
 * @Description: 参数持久化存储模块 —— Save.h 接口的实现。
 *
 * 核心思想：RAM 缓存 + Flash 镜像。
 *   - 所有读操作走 RAM（快、无寿命损耗）
 *   - 所有写操作同步更新 RAM 缓存 + 整区回写 Flash
 *   - 上电时 Flash 整区载入 RAM，之后 Flash 只在"保存"时被触碰
 *
 * 三份 RAM 缓存与 Flash 三区一一对应：
 *   g_systemParams[]  ←→ 系统区（13 个 float + 头部校验）
 *   g_adjustData[]    ←→ 校准区（5 个 32B 槽位，字节镜像）
 *   g_logData[]       ←→ 日志区（预留）
 *
 * Flash 操作模式约定（HC32L196 特有）：
 *   读=内存映射直读（memcpy 即可），但擦/写前必须 Flash_OpModeConfig
 *   切换到对应模式，完成后切回 FlashReadMode，且所在扇区需先 SLOCK 解锁。
 */
#include "Save.h"
#include "Disp.h"
#include "ValveParam.h"    /* valve_param_t *ValveParam_Get() — 控制参数 RAM 缓存 */
#include <string.h>
#include "stdlib.h"

static const char *TAG = "Save";

/* 输入电流默认校准参数（未校准时使用，基于实测 4mA→3.7mA / 11mA→10.6mA 反推） */
#define INPUT_CALIB_DEFAULT_4MA     3227
#define INPUT_CALIB_DEFAULT_20MA    15844

/* ---------------- RAM 缓存（Flash 各区的运行时镜像） ---------------- */
static int32_t g_systemParams[SAVE_SYSTEM_PARAM_COUNT]; /* 系统区镜像：21 个 int32（整型语义） */
static uint8_t g_adjustData[SAVE_ADJUST_DATA_SIZE];   /* 校准区镜像：512B 字节镜像 */
static uint8_t g_logData[SAVE_LOG_DATA_SIZE];         /* 日志区镜像（预留） */
static uint8_t g_isSystemParamsLoaded = 0;            /* 系统区已载入标志：Read_System 懒加载用 */

// CRC32计算表
static const uint32_t crc32_table[] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D
};

/* CRC32 查表法（多项式 0xEDB88320，即标准 CRC-32/ISO-HDLC 反射形式）。
 * 用于系统区参数整块校验：写入前算好存头部，上电读出后重算比对，
 * 任何一位翻转/错位都能检出（含枚举扩容导致的数据错位） */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/** Flash_Init 的编程时间参数需按当前主频选档（原厂手册要求，档位不对会导致擦写异常）。
 *  与 mcu/driver/src/flash.c 中 Flash_Init 的 u8FreqCfg 档位一致（按 HCLK 选档）。
 *  返回值：1~12（HCLK 越高编程等待周期越长） */
static uint8_t save_flash_hclk_to_freq_cfg(void)
{
    SystemCoreClockUpdate();//获取当前时钟
    uint32_t hz = SystemCoreClock;
		if (hz <= 4000000u) {//速度不能小于4mhz
        return 1u;
    }
    if (hz <= 8000000u) {
        return 2u;
    }
    if (hz <= 16000000u) {
        return 4u;
    }
    if (hz <= 24000000u) {
        return 6u;
    }
    if (hz <= 32000000u) {
        return 8u;
    }
    if (hz <= 48000000u) {
        return 12u;
    }
    return 12u;
}

/**
 * @brief 擦写前对参数区所在扇区组解锁（SLOCK 寄存器，bit=1 为解锁）。
 *
 * HC32L196 的 Flash 出厂全锁，每个 SLOCKx 寄存器管 128 个扇区、
 * 每个扇区占 1 bit（512 扇区 → SLOCK0~3 四个 32bit 寄存器）。
 * 按 SAVE 区起止地址算出涉及哪些扇区、对应哪些 bit，读回当前
 * 寄存器值后按位 OR 上去——只解锁自己要动的，不碰其它扇区状态
 * （代码区扇区保持锁定，误写指令也不会生效，多一层保护）。
 * 扇区划分与手册一致时：256KB Flash、512B/扇区下参数区落在 Sector508~510。
 */
static void save_flash_unlock_param_area(void)
{
    uint32_t first_sec = SAVE_FLASH_START_ADDR / SAVE_FLASH_SECTOR_SIZE;
    uint32_t last_sec = (SAVE_FLASH_START_ADDR + SAVE_TOTAL_SIZE - 1u) / SAVE_FLASH_SECTOR_SIZE;
    uint32_t mask0 = 0u, mask1 = 0u, mask2 = 0u, mask3 = 0u;

    for (uint32_t s = first_sec; s <= last_sec; s++) {
        if (s < 128u) {
            mask0 |= (1u << (s / 4u));
        } else if (s < 256u) {
            mask1 |= (1u << ((s - 128u) / 4u));
        } else if (s < 384u) {
            mask2 |= (1u << ((s - 256u) / 4u));
        } else {
            mask3 |= (1u << ((s - 384u) / 4u));
        }
    }

    if (mask0 != 0u && Flash_LockSet(FlashLock0, M0P_FLASH->SLOCK0 | mask0) != Ok) {
        DEBUG_LOGE(TAG, "FlashLock0 unlock failed");
    }
    if (mask1 != 0u && Flash_LockSet(FlashLock1, M0P_FLASH->SLOCK1 | mask1) != Ok) {
        DEBUG_LOGE(TAG, "FlashLock1 unlock failed");
			}//显示debug
    if (mask2 != 0u && Flash_LockSet(FlashLock2, M0P_FLASH->SLOCK2 | mask2) != Ok) {
        DEBUG_LOGE(TAG, "FlashLock2 unlock failed");
    }
    if (mask3 != 0u && Flash_LockSet(FlashLock3, M0P_FLASH->SLOCK3 | mask3) != Ok) {
        DEBUG_LOGE(TAG, "FlashLock3 unlock failed");
    }
}

/** Flash 写入（带最多 10 次重试 + 1ms 间隔，写失败抛 FLASH_WRITE_ERROR 异常并返回 -1）。
 *  调用前须已 Flash_OpModeConfig(FlashWriteMode)，调用后由外层切回读模式 */
static int8_t flash_write_with_retry(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t error = 1;
    do {
			if (Flash_Write8(addr, data, len) != Ok) {//重试10次以上 不成功
            error++;
            if (error > 10) {
                RaisingExceptions(FLASH_WRITE_ERROR);//错误
                return -1;
            }
            delay1ms(1);
        } else {
            error = 0;
        }
    } while (error);
    return 0;
}

/** Flash 读取：HC32L196 为内存映射 Flash，直接 memcpy 即可，不会失败。
 *  保留函数形式是为了与写接口对称，方便将来换存储介质（如外挂 SPI Flash） */
static int8_t flash_read_with_retry(uint32_t addr, uint8_t *data, uint32_t len)
{
    // Flash读取直接内存访问，无需特殊处理
    memcpy(data, (uint8_t *)addr, len);
    return 0;
}


/**
 * @brief 擦除 [start_addr, end_addr) 覆盖到的所有扇区（512B 对齐）。
 *
 * ★ end_addr 为开区间上界：先减 1 再对齐扇区边界。
 *   若直接对齐，当 end_addr 正好落在扇区边界上时会多擦一个扇区
 *   （教训：多擦邻区 = 邻区数据丢失，曾导致"断电重启参数丢失"）。
 *
 * 擦完每个扇区延时 5ms 等待硬件完成，最后切回 FlashReadMode。
 * 调用前需已解锁目标扇区（save_flash_unlock_param_area）。
 */
static int8_t flash_sector_erase_range(uint32_t start_addr, uint32_t end_addr)
{
    if (end_addr <= start_addr) {
        return 0;
    }
    uint32_t sector_mask = ~(SAVE_FLASH_SECTOR_SIZE - 1);
    uint32_t sector_start = start_addr & sector_mask;//start
    /* end_addr为开区间上界：先减1再对齐，否则正好落在扇区边界时会多擦一个扇区 */
    uint32_t sector_end = (end_addr - 1u) & sector_mask;//end
    
    // 配置为扇区擦除模式
    if (Flash_OpModeConfig(FlashSectorEraseMode) != Ok) {
        return -1;
    }
    
    for (uint32_t addr = sector_start; addr <= sector_end; addr += SAVE_FLASH_SECTOR_SIZE) {
        if (Flash_SectorErase(addr) != Ok) {
            return -1;
        }
        delay1ms(5); // 等待擦除完成
    }
    
    // 切换回读模式
    Flash_OpModeConfig(FlashReadMode);
    return 0;
}

/**
 * @brief Flash 存储初始化 —— 唯一直接操作 Flash 硬件的入口函数。
 *
 * 执行顺序（不可调换）：
 *   1. Flash_Init：按 HCLK 档位配置编程时序（擦写前置条件）
 *   2. SLOCK 解锁参数区扇区
 *   3. 读系统区头部 validFlag，分两条路径：
 *      - 无效（首次上电 / 数据损坏 / 版本升级）→ 灌默认值 + 整块写 Flash
 *        （默认值清单须与 enum Save_System 保持同步）
 *      - 有效 → Load_System_Params_From_Flash 载入 RAM（CRC 在其中校验）
 *   4. 校准区整区直读 Flash 填充 g_adjustData 缓存
 *      ★ 必须直读 Flash，不能走 Read_System_block（它优先读 RAM，
 *        而此刻 RAM 缓存还没填，会读到全 0——上电专用路径）
 *
 * 由 Save_System_Init 第一步调用，通常不应在别处直接调用。
 */
int8_t Save_Flash_Init(void)
{
    /* 与原厂例程一致：先 Flash_Init（编程时间等），第二参数与样例一致为 DeepSleep 下 Flash 低功耗策略 */
    if (Flash_Init(save_flash_hclk_to_freq_cfg(), TRUE) != Ok) {
        DEBUG_LOGE(TAG, "Flash_Init failed (HCLK=%lu)", (unsigned long)SystemCoreClock);
    }

    save_flash_unlock_param_area();

    // 检查Flash参数区域是否有效
    system_param_header_t header;
    flash_read_with_retry(SAVE_FLASH_START_ADDR, (uint8_t *)&header, sizeof(header));
    
    if (header.validFlag != SAVE_VALID_FLAG) {
        // 首次使用，初始化默认参数
        DEBUG_LOGI(TAG, "Flash param area invalid, init with default values");
        
        // 设置默认系统参数【★ 新增参数必须在此补默认值，与 Save_System_Init 分发配对】
        for (int i = 0; i < SAVE_SYSTEM_PARAM_COUNT; i++) {
            g_systemParams[i] = 0;
        }
        g_systemParams[SAVE_BACKLIGHT] = 100;
        g_systemParams[SAVE_CONTRAST_RATIO] = 10;
        g_systemParams[SAVE_LANGUAGE] = 0;
        g_systemParams[SAVE_DISPLAY_MIRROR] = 0;
        g_systemParams[SAVE_ERROR_RANGE] = 3;                   /* 0.3%（×10，遗留槽位不分发） */
        g_systemParams[SAVE_ANGLE_MIN] = ANGLE_MIN_DEFAULT;    /* 未标定：默认 0.00° */
        g_systemParams[SAVE_ANGLE_MAX] = ANGLE_MAX_DEFAULT;    /* 90.00°（×100） */
//        g_systemParams[SAVE_CONTROL_MODE] = CONTROL_MODE_CURRENT_PERCENT;
        g_systemParams[SAVE_CONTROL_KP] = PID_KP_DEFAULT;    /* Kp=5.0（×10 缩放整型） */
        g_systemParams[SAVE_CONTROL_KI] = PID_KI_DEFAULT;    /* Ki=0.05（×100） */
        g_systemParams[SAVE_CONTROL_KD] = PID_KD_DEFAULT;    /* Kd=0.0（×100） */
        g_systemParams[SAVE_CONTROL_KP_DOWN] = PID_KP_DEFAULT; /* 下行默认同上行 */
        g_systemParams[SAVE_CONTROL_KI_DOWN] = PID_KI_DEFAULT;
        g_systemParams[SAVE_CONTROL_KD_DOWN] = PID_KD_DEFAULT;
        /* ---- v3 四项（与 ValveParam_SetDefault 保持一致） ---- */
        g_systemParams[SAVE_ACTUATOR_TYPE]   = ACTUATOR_TYPE_LT;    /* 直行程 */
        g_systemParams[SAVE_BALANCE_LEVEL]   = BALANCE_DEFAULT;     /* 适中 */
        g_systemParams[SAVE_REDUNDANT_ERROR] = 3;                   /* 0.3% → deadband=3 */
        g_systemParams[SAVE_FRICTION_LEVEL]  = FRICTION_LOW;        /* 低摩擦 */
        /* ---- v4 ---- */
        g_systemParams[SAVE_MECHANICAL_DIRECTION] = MECH_DIR_POSITIVE; /* 正向 */
        /* ---- v5 ---- */
        g_systemParams[SAVE_MECHANICAL_MIN] = MECH_TRAVEL_MIN;      /* 行程限位默认 0~1000 全行程 */
        g_systemParams[SAVE_MECHANICAL_MAX] = MECH_TRAVEL_MAX;
        /* ---- v6 ---- */
        g_systemParams[SAVE_FEEDBACK_DIRECTION] = FB_DIR_POSITIVE;  /* 反馈方向默认正向 */
        g_systemParams[SAVE_PST_PERIOD] = PST_PERIOD_DEFAULT;       /* 定时任务默认未启用 */
        g_systemParams[SAVE_SAFE_POSITION] = SAFE_POS_FAIL_SAFE;    /* 故障模式默认故障安全 */
        g_systemParams[SAVE_EXECUTION_TIMEOUT] = EXEC_TIMEOUT_DEFAULT; /* 行程时间极限默认 60s */
        
        // 保存到Flash
        Save_System_Params_To_Flash();
        
        // 初始化校准数据区域为0xFF
        memset(g_adjustData, 0xFF, sizeof(g_adjustData));
        Save_System_block(SAVE_ADJUST_ANGLE, g_adjustData, sizeof(g_adjustData));
    } else {
        // 读取已有参数
        Load_System_Params_From_Flash();
        // 读取校准数据：必须直读Flash整区填充缓存，不能走RAM缓存（上电时缓存为空）
        flash_read_with_retry(SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE,
                              g_adjustData, sizeof(g_adjustData));
    }
    
    return 0;
}

/** 系统区整块落盘：g_systemParams → system_param_header_t → 擦扇区A → 写回。
 *  组头部（validFlag + CRC32）在函数内现算，调用方只需保证 RAM 缓存已更新。
 *  触发时机：Save_Flash_Init 首次上电默认值 / Save_System 单参数保存 / data_Restore */
int8_t Save_System_Params_To_Flash(void)
{
    system_param_header_t header;//定义一个存储实例
    /* 注意：buffer 尺寸偏大——header_t 里已含 systemParams[]（60B），
     * 又多加了 sizeof(g_systemParams)（52B），实际只有前 60B 被填充。
     * 下方 sizeof(buffer) 写入会把后 52B 未初始化栈数据带进 Flash
     * （读回只取 sizeof(header)，功能不受影响；TODO 可改成只写 sizeof(header)） */
    uint8_t buffer[sizeof(system_param_header_t) + sizeof(g_systemParams)];

    header.validFlag = SAVE_VALID_FLAG;//有效标志（版本变更时同步更新）
    memcpy(header.systemParams, g_systemParams, sizeof(g_systemParams));//参数数组快照
    header.crc32 = calculate_crc32((uint8_t *)header.systemParams, sizeof(header.systemParams));//CRC 只覆盖参数区

    memcpy(buffer, &header, sizeof(header));//头+参数 60B 打包

    // 擦除系统区（start=0x3F800，end 为开区间上界→只擦扇区A，不波及校准区）
    uint32_t end_addr = SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE;
    if (flash_sector_erase_range(SAVE_FLASH_START_ADDR, end_addr) != 0) {
        return -1;
    }

    // 配置为写模式（擦写前必须切模式，HC32L196 读/写/擦三种模式互斥）
    if (Flash_OpModeConfig(FlashWriteMode) != Ok) {
        return -1;
    }

    // 写入数据（带重试）
    int8_t ret = flash_write_with_retry(SAVE_FLASH_START_ADDR, buffer, sizeof(buffer));

    // 切换回读模式（务必恢复，否则后续内存映射读会异常）
    Flash_OpModeConfig(FlashReadMode);

    return ret;
}

/** 系统区整块载入：Flash 读头部 → 双层校验（validFlag + CRC32）→ 拷入 RAM 缓存。
 *  校验任一失败返回 -1 且不置 g_isSystemParamsLoaded（此时 RAM 缓存内容不可信），
 *  由调用方决定是否走默认值路径。CRC 校验同时防位翻转和参数数组错位 */
int8_t Load_System_Params_From_Flash(void)
{
    system_param_header_t header;
    flash_read_with_retry(SAVE_FLASH_START_ADDR, (uint8_t *)&header, sizeof(header));
    
    if (header.validFlag != SAVE_VALID_FLAG) {
        return -1;
    }
    
    uint32_t calc_crc = calculate_crc32((uint8_t *)header.systemParams, sizeof(header.systemParams));
    if (calc_crc != header.crc32) {
        DEBUG_LOGE(TAG, "System param CRC error! calc=0x%08X, save=0x%08X", calc_crc, header.crc32);
        return -1;
    }
    
    memcpy(g_systemParams, header.systemParams, sizeof(g_systemParams));
    g_isSystemParamsLoaded = 1;
    return 0;
}

/** 写单个系统参数：先更新 RAM 缓存，再整块回写 Flash（含擦除，约 10ms 级耗时）。
 *  ★ 因为是整块回写，两次连续 Save 会擦写两次 Flash——批量改参数时注意
 *    Flash 寿命（1 万次/扇区），或考虑将来改成"脏标记 + 延迟合并落盘" */
void Save_System(enum Save_System index, int32_t dat)
{
    if (index >= SAVE_SYSTEM_PARAM_COUNT) { //如果超出了范围
        DEBUG_LOGE(TAG, "Invalid system param index: %d", index);//debug报错
        return;
    }

    DEBUG_LOGD(TAG, "Save System index:%d data:%d", index, (int)dat);
    
    // 更新RAM缓存
    g_systemParams[index] = dat;
    
    // 保存整个参数块到Flash
    Save_System_Params_To_Flash();
}

/** 读单个系统参数：纯 RAM 读取，不碰 Flash、无耗时。
 *  正常流程下 Save_System_Init 已在上电时调过一次（缓存必已载入）；
 *  懒加载分支（g_isSystemParamsLoaded==0）只是防御性兜底。
 *  ★ 上电初始化请勿依赖此懒加载装载数据——分发逻辑统一放 Save_System_Init */
int32_t Read_System(enum Save_System index)
{
    if (index >= SAVE_SYSTEM_PARAM_COUNT) {
        DEBUG_LOGE(TAG, "Invalid system param index: %d", index);
        return 0;
    }

    if (!g_isSystemParamsLoaded) {
        Load_System_Params_From_Flash();
    }

    DEBUG_LOGD(TAG, "Read System index:%d data:%d", index, (int)g_systemParams[index]);
    return g_systemParams[index];
}

/* 控制模式读写（存于 SAVE_CONTROL_MODE 槽位，整型直存）。
 * 读取时做范围校验，Flash 脏数据/升级残留回落 CONTROL_MODE_CURRENT_PERCENT，
 * 避免闭环跑进不存在的模式分支 */
int get_inpu_mode(void)
{
    int mode = Read_System(SAVE_CONTROL_MODE);
    if (mode < 0 || mode >= CONTROL_MODE_END) {
        mode = CONTROL_MODE_CURRENT_PERCENT;
    }
    return mode;
}

/* 保存控制模式 */
void save_inpu_mode(int mode)
{
    if (mode < 0 || mode >= CONTROL_MODE_END) {
        mode = CONTROL_MODE_CURRENT_PERCENT;
    }
    Save_System(SAVE_CONTROL_MODE, mode);
}

/** 写校准区块：更新 RAM 缓存对应槽位 → 擦整个校准区扇区B → 整区回写。
 *  @param addr 校准区"内部偏移"（SAVE_ADJUST_ANGLE 等宏，0~511），函数内部
 *              再加区域基地址 0x3FA00 —— 调用方传绝对地址会写错区域
 *  @param dat  待写入数据指针；若指向 g_adjustData 自身则跳过自拷贝
 *  校准区槽位各自带 validFlag 自校验，整区擦写不影响系统区（扇区独占） */
int8_t Save_System_block(uint32_t addr, void *dat, uint32_t len)
{
    if (addr + len > SAVE_ADJUST_DATA_SIZE) {
        DEBUG_LOGE(TAG, "Save block out of range! addr=%lu, len=%lu",
                   (unsigned long)addr, (unsigned long)len);
        return -1;
    }

    // 更新RAM缓存（初始化时dat可能就是g_adjustData自身，跳过自拷贝）
    if (dat != (void *)g_adjustData) {
        memcpy(g_adjustData + addr, dat, len);
    }

    // 擦除整个校准数据区域（独占一个扇区，不影响其它区域）
    uint32_t start_addr = SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE;//校准区基地址
    uint32_t end_addr = start_addr + SAVE_ADJUST_DATA_SIZE;//校准区结束地址
    if (flash_sector_erase_range(start_addr, end_addr) != 0) {//擦除扇区
        return -1;
    }
    
    // 配置为写模式
    if (Flash_OpModeConfig(FlashWriteMode) != Ok) {
        return -1;
    }
    
    // 写入整个校准数据块
    int8_t ret = flash_write_with_retry(start_addr, g_adjustData, SAVE_ADJUST_DATA_SIZE);
    
    // 切换回读模式
    Flash_OpModeConfig(FlashReadMode);
    
    return ret;
}

/** 读校准区块：从 RAM 缓存 g_adjustData 拷贝，不碰 Flash。
 *  @param addr 校准区内部偏移（同 Save_System_block 约定）
 *  缓存由 Save_Flash_Init 上电时整区直读 Flash 填充；
 *  ★ 上电装载数据时应直读 Flash（flash_read_with_retry），不要用本函数
 *    （此时缓存还没填，会读到全 0——详见 Save_Flash_Init 注释） */
int8_t Read_System_block(uint32_t addr, void *dat, uint32_t len)
{
    if (addr + len > SAVE_ADJUST_DATA_SIZE) {
        DEBUG_LOGE(TAG, "Read block out of range! addr=%lu, len=%lu",
                   (unsigned long)addr, (unsigned long)len);
        return -1;
    }

    memcpy(dat, g_adjustData + addr, len);
    return 0;
}

/** 恢复出厂设置：把关键系统参数逐个刷回默认值（每个参数触发一次整块 Flash 回写）。
 *  当前不重置 SAVE_CONTROL_MODE / SAVE_CONTROL_KP 等控制参数，也不清校准区
 *  ——若需要"彻底恢复出厂"，应同步补齐并调用页面刷新 */
void data_Restore(void)
{
    Save_System(SAVE_BACKLIGHT, 100);                             // 亮度
    Save_System(SAVE_CONTRAST_RATIO, 10);                         // 对比对
    Save_System(SAVE_LANGUAGE, 0);                                // 语言中文
    Save_System(SAVE_DISPLAY_MIRROR, 0);                          // 屏幕自动
    Save_System(SAVE_ERROR_RANGE, 3);                             // 允许误差 0.3%（×10，遗留槽位）
    Save_System(SAVE_ANGLE_MIN, ANGLE_MIN_DEFAULT);               // 阀位零位默认 0.00°
    Save_System(SAVE_ANGLE_MAX, ANGLE_MAX_DEFAULT);               // 阀位满位默认 90.00°（×100）
//    Save_System(SAVE_CONTROL_MODE, CONTROL_MODE_CURRENT_PERCENT); // 控制模式
}



/**
 * @brief 上电参数统一分发入口 —— 所有 Flash 参数从这里流向各模块。
 *
 * 分发顺序（模块间有依赖，顺序不可调换）：
 *   第零步  ValveParam_SetDefault  灌硬编码默认值兜底
 *           （先默认后覆盖：Flash 有效值只覆盖部分字段，其余字段如
 *            k3/k4 补偿、dac_mid 等无 Flash 槽位，全靠这步给值）
 *   第一步  Save_Flash_Init        Flash→RAM 双区缓存（唯一碰硬件处）
 *   第二步  校准区 → 输入电流校准（槽位自校验，失败用默认斜率）
 *   第三步  系统区 → 屏幕模块（对比度/背光/语言/镜像）
 *   第四步  系统区 → 控制模块 ValveParam（含范围合法性检查，
 *           Flash 值不合理时保留第零步默认值，防止脏数据进闭环）
 *   第五步  系统区 → 控制模式（非法值回落电流模式）
 *
 * ★ 各模块的 Init 函数（如 ValveControl_Init）不再自己 Read_System，
 *   统一从这里拿最终值——避免"谁先初始化谁覆盖"的时序坑。
 * ★ 新增参数的分发代码也加在这里（配对 Save_Flash_Init 默认值）。
 */
void Save_System_Init(void)
{
    /* ========== 第零步：先刷模块默认值（作为 Flash 无效时的兜底） ========== */
    ValveParam_SetDefault();//

    /* ========== 第一步：Flash → RAM 缓存 ========== */
    Save_Flash_Init();

    /* ========== 第二步：校准区 → 各模块（输入电流校准） ========== */
    {
        input_calibration_t input_current;
        if (0 == Read_System_block(SAVE_INPUT_CURRENT, &input_current, sizeof(input_calibration_t)))
        {
            if (input_current.validFlag == INPUT_CALIB_VALID_FLAG &&
                input_current.calibrated == 1 &&
                input_current.raw_20ma > input_current.raw_4ma)
            {
                set_input_calibration(&input_current);
                DEBUG_LOGI(TAG, "input current calibrated: 4mA=%ld, 20mA=%ld",
                           (long)input_current.raw_4ma, (long)input_current.raw_20ma);
            }
            else
            {
                input_calibration_t def;
                def.validFlag  = INPUT_CALIB_VALID_FLAG;
                def.raw_4ma    = INPUT_CALIB_DEFAULT_4MA;
                def.raw_20ma   = INPUT_CALIB_DEFAULT_20MA;
                def.calibrated = 0;
                set_input_calibration(&def);
                DEBUG_LOGI(TAG, "input current not calibrated, use default");
            }
        }
    }

    /* ========== 第三步：系统区 → 屏幕模块 ========== */
    lcd_set_contrast_ratio(Read_System(SAVE_CONTRAST_RATIO));
    /* 背光度：绑定 Timer/PWM 输出（需对应硬件 API，暂留接口） */
    {
        int bl = Read_System(SAVE_BACKLIGHT);
        int bl_mode = Read_System(SAVE_BACKLIGHT_MODE);
        DEBUG_LOGI(TAG, "backlight=%d mode=%d (no HW API yet)", bl, bl_mode);
        /* TODO: 有 PWM 接口时加
         * UserTime2SetPwm(bl);
         */
    }
    /* 屏幕镜像 / 语言：GUIMain 在 DispKeyTask 里另读，此处只打日志确认 */
    DEBUG_LOGI(TAG, "language=%d display_mirror=%d",
               Read_System(SAVE_LANGUAGE), Read_System(SAVE_DISPLAY_MIRROR));

    /* ========== 第四步：系统区 → 控制模块（ValveParam） ==========
     * 合法性检查在这里做，ValveControl_Init 不再 Read_System */
    {
        valve_param_t *p = ValveParam_Get();

        /* 死区：由 SAVE_REDUNDANT_ERROR 提供（整型 3~100 = 0.3%~10.0%，
         * 数值与 deadband 单位 0.1% 一致，直接用）。SAVE_ERROR_RANGE 为遗留槽位不再分发 */
        {
            int re = Read_System(SAVE_REDUNDANT_ERROR);
            if (re >= REDUNDANT_ERROR_MIN && re <= REDUNDANT_ERROR_MAX) {
                p->deadband = re;
            }
        }

        /* 阀位零/满位：存储为角度×100 整数，除回为度；未标定时 amax <= amin + 5° 视为无效 */
        {
            float amin = (float)Read_System(SAVE_ANGLE_MIN) / ANGLE_SCALE;
            float amax = (float)Read_System(SAVE_ANGLE_MAX) / ANGLE_SCALE;
            if (amax > amin + 5.0f) {
                p->angle_min = amin;
                p->angle_max = amax;
            }
        }

        /* PID 参数：存储为缩放整型（KP×10 / KI,KD×100），分发时除回去还原实际增益。
         * 合法性在缩放后的整数域检查：Kp 1~500(0.1~50.0)、Ki 0~500(0~5.00)、Kd 0~500(0~5.00) */
        {
            int kp10  = Read_System(SAVE_CONTROL_KP);
            int ki100 = Read_System(SAVE_CONTROL_KI);
            int kd100 = Read_System(SAVE_CONTROL_KD);
            if (kp10  > 0 && kp10  <= 500) p->kp = (float)kp10  / PID_KP_SCALE;
            if (ki100 >= 0 && ki100 <= 500) p->ki = (float)ki100 / PID_KI_SCALE;
            if (kd100 >= 0 && kd100 <= 500) p->kd = (float)kd100 / PID_KD_SCALE;
        }
        {
            int kp10  = Read_System(SAVE_CONTROL_KP_DOWN);
            int ki100 = Read_System(SAVE_CONTROL_KI_DOWN);
            int kd100 = Read_System(SAVE_CONTROL_KD_DOWN);
            if (kp10  > 0 && kp10  <= 500) p->kp_down = (float)kp10  / PID_KP_SCALE;
            if (ki100 >= 0 && ki100 <= 500) p->ki_down = (float)ki100 / PID_KI_SCALE;
            if (kd100 >= 0 && kd100 <= 500) p->kd_down = (float)kd100 / PID_KD_SCALE;
        }

        /* v3 档位参数：合法档位才覆盖，否则保留默认值（当前仅存档备用，
         * 控制行为挂接点：actuator_type→行程映射、balance→PID增益组、friction→k3/k4） */
        {
            int act = Read_System(SAVE_ACTUATOR_TYPE);
            int bal = Read_System(SAVE_BALANCE_LEVEL);
            int fri = Read_System(SAVE_FRICTION_LEVEL);
            if (act == ACTUATOR_TYPE_LT || act == ACTUATOR_TYPE_ROTS) p->actuator_type = (uint8_t)act;
            if (bal >= BALANCE_MAX_SPEED && bal <= BALANCE_MAX_STABLE) p->balance_level = (uint8_t)bal;
            if (fri == FRICTION_LOW || fri == FRICTION_HIGH)           p->friction_level = (uint8_t)fri;
            /* 机械行程方向 → reverse_fb：angle_to_valve 里反馈阀位 0~1000 翻转，立即生效 */
            int dir = Read_System(SAVE_MECHANICAL_DIRECTION);
            if (dir == MECH_DIR_POSITIVE || dir == MECH_DIR_REVERSED) p->reverse_fb = (uint8_t)dir;
            /* 机械行程限位：必须在 0~1000 且 min < max，否则保留默认全行程 */
            int tmin = Read_System(SAVE_MECHANICAL_MIN);
            int tmax = Read_System(SAVE_MECHANICAL_MAX);
            if (tmin >= MECH_TRAVEL_MIN && tmax <= MECH_TRAVEL_MAX && tmin < tmax) {
                p->travel_min = tmin;
                p->travel_max = tmax;
            }
            /* 阀位反馈方向 → feedback_dir（对外反馈用，不影响闭环，闭环方向由 reverse_fb 管） */
            int fbdir = Read_System(SAVE_FEEDBACK_DIRECTION);
            if (fbdir == FB_DIR_POSITIVE || fbdir == FB_DIR_REVERSED) p->feedback_dir = (uint8_t)fbdir;
        }

        DEBUG_LOGI(TAG, "ValveParam loaded: Kp=%.2f Ki=%.3f Kd=%.3f deadband=%d angle=[%.1f, %.1f] act=%d bal=%d fri=%d",
                   p->kp, p->ki, p->kd, p->deadband, p->angle_min, p->angle_max,
                   p->actuator_type, p->balance_level, p->friction_level);
    }

    /* ========== 第四步续：系统区 → 定时任务周期（调度未实现，仅合法性检查 + 日志） ========== */
    {
        int32_t pst = Read_System(SAVE_PST_PERIOD);
        if (pst < 0 || pst > PST_MONTH_MAX * PST_MINUTES_PER_MONTH + PST_HOUR_MAX * 60 + PST_MINUTE_MAX) {
            pst = PST_PERIOD_DEFAULT;
        }
        DEBUG_LOGI(TAG, "pst period: %d month %d hour %d min",
                   (int)(pst / PST_MINUTES_PER_MONTH),
                   (int)((pst % PST_MINUTES_PER_MONTH) / 60),
                   (int)(pst % 60));
    }

    /* ========== 第五步：系统区 → 控制模式（合法性校验 + 日志） ========== */
    {
        int mode = get_inpu_mode();
        DEBUG_LOGI(TAG, "control mode: %d", mode);
    }

    /* 故障模式（SAVE_SAFE_POSITION）：分发在 OutputDriver_Init 内完成，
     * 因 PF05 GPIO 在 OutputDriver_Init 才初始化，此处仅日志确认 */
    {
        int32_t sp = Read_System(SAVE_SAFE_POSITION);
        DEBUG_LOGI(TAG, "safe position: %d (0=fail-safe 1=fail-lock)", (int)sp);
    }

    /* 行程时间极限（SAVE_EXECUTION_TIMEOUT）：消费者为自整定单步超时
     * （ValveAutoAdjust step_begin 每步装载），此处仅合法性检查 + 日志 */
    {
        int32_t to = Read_System(SAVE_EXECUTION_TIMEOUT);
        if (to < EXEC_TIMEOUT_MIN || to > EXEC_TIMEOUT_MAX) {
            to = EXEC_TIMEOUT_DEFAULT;
        }
        DEBUG_LOGI(TAG, "execution timeout: %d s", (int)to);
    }
}

/* 【遗留空函数】原 EEPROM 时代的分段读取接口，迁移到内部 Flash 后已无调用方，
 * 后续清理时可直接删除 */
void Read_SegA(void)
{
	
	
	
	
}

/* ===================== 输入电流校准 =====================
 * ADX111 采集 4-20mA 输入电流信号，raw 值与 mA 的映射由两点标定确定：
 *   标定前：用默认斜率（INPUT_CALIB_DEFAULT_4MA/20MA，实测反推）
 *   标定后：用 Flash 校准区槽位4 存的两组 raw 值做两点线性插值
 * 上电载入链：Save_System_Init 第二步 → Read_System_block → set_input_calibration
 */

static input_calibration_t g_inputCalib;  /* RAM 缓存：换算函数直接读它，无 Flash 依赖 */

/** 只写 RAM 缓存（上电载入用，不落盘） */
void set_input_calibration(const input_calibration_t *cal)
{
    if (cal != NULL)
    {
        g_inputCalib = *cal;
    }
}

/** 拷出当前 RAM 缓存（查询用） */
void get_input_calibration(input_calibration_t *cal)
{
    if (cal != NULL)
    {
        *cal = g_inputCalib;
    }
}

/** 标定完成落盘：组好结构体 → 更新 RAM 缓存 → 写入校准区槽位4（整区回写 Flash）。
 *  由"电流标定"页面在保存确认后调用，raw 值来自页面去头去尾均值采集 */
void save_input_calibration(int32_t raw_4ma, int32_t raw_20ma)
{
    input_calibration_t cal;

    cal.validFlag   = INPUT_CALIB_VALID_FLAG;
    cal.raw_4ma     = raw_4ma;
    cal.raw_20ma    = raw_20ma;
    cal.calibrated  = 1;
    cal.reserved[0] = 0;
    cal.reserved[1] = 0;
    cal.reserved[2] = 0;

    set_input_calibration(&cal);

    if (Save_System_block(SAVE_INPUT_CURRENT, &cal, sizeof(cal)) != 0)
    {
        DEBUG_LOGE(TAG, "save input calibration failed");
        return;
    }
    DEBUG_LOGI(TAG, "input calib saved: 4mA=%ld, 20mA=%ld",
               (long)raw_4ma, (long)raw_20ma);
}

/* raw AD 值 → 电流(mA)：两点线性插值，整数定点版（乘 100 保留 2 位小数再截断）。
 * 未校准/分母为 0 时回落默认斜率。M0+ 无 FPU，热路径优先用这版省浮点开销 */
int32_t input_current_raw_to_ma(int32_t raw)
{
    int32_t r4 = g_inputCalib.raw_4ma;
    int32_t r20 = g_inputCalib.raw_20ma;
    int32_t denom = r20 - r4;

    if (g_inputCalib.calibrated == 0 || denom == 0)
    {
        /* 未校准：用默认值 */
        r4 = INPUT_CALIB_DEFAULT_4MA;
        r20 = INPUT_CALIB_DEFAULT_20MA;
        denom = r20 - r4;
        if (denom == 0) return 0;
    }

    /* 线性插值: mA = 4 + (raw - r4) * (20 - 4) / (r20 - r4)
     * 用定点数避免浮点，结果保留两位小数(×100) */
    int64_t ma_x100 = 400 + (int64_t)(raw - r4) * 1600 / denom;
    return (int32_t)(ma_x100 / 100);  /* 返回 mA 整数部分 */
}

/* 浮点版本：控制环/需要小数精度的显示用（同样带未校准回落） */
float input_current_raw_to_ma_float(int32_t raw)
{
    int32_t r4 = g_inputCalib.raw_4ma;
    int32_t r20 = g_inputCalib.raw_20ma;
    int32_t denom = r20 - r4;

    if (g_inputCalib.calibrated == 0 || denom == 0)
    {
        r4 = INPUT_CALIB_DEFAULT_4MA;
        r20 = INPUT_CALIB_DEFAULT_20MA;
        denom = r20 - r4;
        if (denom == 0) return 0.0f;
    }

    return 4.0f + (float)(raw - r4) * 16.0f / (float)denom;
}

