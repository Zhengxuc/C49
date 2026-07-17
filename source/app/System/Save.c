#include "Save.h"
#include "Disp.h"
#include <string.h>
#include "stdlib.h"

static const char *TAG = "Save";

// 系统参数缓存（RAM中保存）
static float g_systemParams[SAVE_SYSTEM_PARAM_COUNT];
static uint8_t g_adjustData[SAVE_ADJUST_DATA_SIZE];
static uint8_t g_logData[SAVE_LOG_DATA_SIZE];
static uint8_t g_isSystemParamsLoaded = 0;

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

static uint32_t calculate_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/** 与 mcu/driver/src/flash.c 中 Flash_Init 的 u8FreqCfg 档位一致（按 HCLK 选档） */
static uint8_t save_flash_hclk_to_freq_cfg(void)
{
    SystemCoreClockUpdate();
    uint32_t hz = SystemCoreClock;
    if (hz <= 4000000u) {
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
 * 与原厂例程一致：擦写前须对参数区所在扇区组解锁（SLOCK 中 bit=1 为解锁）。
 * 按 SAVE 起止地址计算 Lock0~3 及 bit，读回当前寄存器后 OR，避免把其它扇区误锁。
 * 扇区划分与手册一致时：256KB Flash、512B/扇区下 0x3F800 落在 Sector508~511 → 仅动 SLOCK3.bit31。
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
    }
    if (mask2 != 0u && Flash_LockSet(FlashLock2, M0P_FLASH->SLOCK2 | mask2) != Ok) {
        DEBUG_LOGE(TAG, "FlashLock2 unlock failed");
    }
    if (mask3 != 0u && Flash_LockSet(FlashLock3, M0P_FLASH->SLOCK3 | mask3) != Ok) {
        DEBUG_LOGE(TAG, "FlashLock3 unlock failed");
    }
}

// Flash写入（带重试）
static int8_t flash_write_with_retry(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t error = 1;
    do {
        if (Flash_Write8(addr, data, len) != Ok) {
            error++;
            if (error > 10) {
                RaisingExceptions(FLASH_WRITE_ERROR);
                return -1;
            }
            delay1ms(1);
        } else {
            error = 0;
        }
    } while (error);
    return 0;
}

// Flash读取（带重试）
static int8_t flash_read_with_retry(uint32_t addr, uint8_t *data, uint32_t len)
{
    // Flash读取直接内存访问，无需特殊处理
    memcpy(data, (uint8_t *)addr, len);
    return 0;
}

// 擦除扇区（如果地址范围跨扇区需要擦除多个）
static int8_t flash_sector_erase_range(uint32_t start_addr, uint32_t end_addr)
{
    uint32_t sector_mask = ~(SAVE_FLASH_SECTOR_SIZE - 1);
    uint32_t sector_start = start_addr & sector_mask;
    uint32_t sector_end = end_addr & sector_mask;
    
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

// Flash初始化
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
        
        // 设置默认系统参数
        for (int i = 0; i < SAVE_SYSTEM_PARAM_COUNT; i++) {
            g_systemParams[i] = 0;
        }
        g_systemParams[SAVE_BACKLIGHT] = 100;
        g_systemParams[SAVE_CONTRAST_RATIO] = 10;
        g_systemParams[SAVE_LANGUAGE] = 0;
        g_systemParams[SAVE_DISPLAY_MIRROR] = 0;
        g_systemParams[SAVE_ERROR_RANGE] = 0.3f;
//        g_systemParams[SAVE_CONTROL_MODE] = CONTROL_MODE_CURRENT_PERCENT;
        
        // 保存到Flash
        Save_System_Params_To_Flash();
        
        // 初始化校准数据区域为0xFF
        memset(g_adjustData, 0xFF, sizeof(g_adjustData));
        Save_System_block(SAVE_ADJUST_ANGLE, g_adjustData, sizeof(g_adjustData));
    } else {
        // 读取已有参数
        Load_System_Params_From_Flash();
        // 读取校准数据
        Read_System_block(SAVE_ADJUST_ANGLE, g_adjustData, sizeof(g_adjustData));
    }
    
    return 0;
}

// 保存所有系统参数到Flash
int8_t Save_System_Params_To_Flash(void)
{
    system_param_header_t header;
    uint8_t buffer[sizeof(system_param_header_t) + sizeof(g_systemParams)];
    
    header.validFlag = SAVE_VALID_FLAG;
    memcpy(header.systemParams, g_systemParams, sizeof(g_systemParams));
    header.crc32 = calculate_crc32((uint8_t *)header.systemParams, sizeof(header.systemParams));
    
    memcpy(buffer, &header, sizeof(header));
    
    // 擦除Flash区域
    uint32_t end_addr = SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE;
    if (flash_sector_erase_range(SAVE_FLASH_START_ADDR, end_addr) != 0) {
        return -1;
    }
    
    // 配置为写模式
    if (Flash_OpModeConfig(FlashWriteMode) != Ok) {
        return -1;
    }
    
    // 写入数据
    int8_t ret = flash_write_with_retry(SAVE_FLASH_START_ADDR, buffer, sizeof(buffer));
    
    // 切换回读模式
    Flash_OpModeConfig(FlashReadMode);
    
    return ret;
}

// 从Flash读取所有系统参数
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

// 保存单个系统参数（同时更新RAM和Flash）
void Save_System(enum Save_System index, float32_t dat)
{
    if (index >= SAVE_SYSTEM_PARAM_COUNT) {
        DEBUG_LOGE(TAG, "Invalid system param index: %d", index);
        return;
    }
    
    DEBUG_LOGD(TAG, "Save System index:%d data:%.2f", index, dat);
    
    // 更新RAM缓存
    g_systemParams[index] = dat;
    
    // 保存整个参数块到Flash
    Save_System_Params_To_Flash();
}

// 读取单个系统参数
float32_t Read_System(enum Save_System index)
{
    if (index >= SAVE_SYSTEM_PARAM_COUNT) {
        DEBUG_LOGE(TAG, "Invalid system param index: %d", index);
        return 0;
    }
    
    if (!g_isSystemParamsLoaded) {
        Load_System_Params_From_Flash();
    }
    
    DEBUG_LOGD(TAG, "Read System index:%d data:%.2f", index, g_systemParams[index]);
    return g_systemParams[index];
}

// 保存块数据（校准参数等）
int8_t Save_System_block(uint32_t addr, void *dat, uint32_t len)
{
    uint32_t flash_addr = SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE + addr;
    
    if (flash_addr + len > SAVE_FLASH_START_ADDR + SAVE_TOTAL_SIZE) {
        DEBUG_LOGE(TAG, "Save block out of range! addr=0x%08X, len=%d", addr, len);
        return -1;
    }
    
    // 更新RAM缓存
    if (addr < SAVE_ADJUST_DATA_SIZE) {
        memcpy(g_adjustData + addr, dat, len);
    }
    
    // 擦除整个校准数据区域（简化处理，实际可以只擦除影响的扇区）
    uint32_t start_addr = SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE;
    uint32_t end_addr = start_addr + SAVE_ADJUST_DATA_SIZE;
    if (flash_sector_erase_range(start_addr, end_addr) != 0) {
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

// 读取块数据
int8_t Read_System_block(uint32_t addr, void *dat, uint32_t len)
{
    uint32_t flash_addr = SAVE_FLASH_START_ADDR + SAVE_SYSTEM_DATA_SIZE + addr;
    
    if (flash_addr + len > SAVE_FLASH_START_ADDR + SAVE_TOTAL_SIZE) {
        DEBUG_LOGE(TAG, "Read block out of range! addr=0x%08X, len=%d", addr, len);
        return -1;
    }
    
    // 从RAM缓存读取
    if (addr < SAVE_ADJUST_DATA_SIZE) {
        memcpy(dat, g_adjustData + addr, len);
    } else {
        // 直接从Flash读取
        flash_read_with_retry(flash_addr, (uint8_t *)dat, len);
    }
    
    return 0;
}

void data_Restore()
{
    Save_System(SAVE_BACKLIGHT, 100);                             // 亮度
    Save_System(SAVE_CONTRAST_RATIO, 10);                         // 对比对
    Save_System(SAVE_LANGUAGE, 0);                                // 语言中文
    Save_System(SAVE_DISPLAY_MIRROR, 0);                          // 屏幕自动
    Save_System(SAVE_ERROR_RANGE, 0.3f);                          // 允许误差0.3
//    Save_System(SAVE_CONTROL_MODE, CONTROL_MODE_CURRENT_PERCENT); // 控制模式
}

void Save_System_Init(void)
{
    // 初始化Flash存储
    Save_Flash_Init();
    
    /*设置对比对*/
    lcd_set_contrast_ratio(Read_System(SAVE_CONTRAST_RATIO));

    /*读取控制模式*/
//    control_mode_t input_mode;
//    input_mode = (int32_t)Read_System(SAVE_CONTROL_MODE);
//    DEBUG_LOGI(TAG, "输入控制模式:%d", input_mode);
//    if (input_mode < 0 || input_mode >= CONTROL_MODE_END)
//        input_mode = CONTROL_MODE_CURRENT_PERCENT;
//    set_input_confit_mode(input_mode);
//    DEBUG_LOGI(TAG, "输入控制模式:%d", input_mode);

//    /*读取误差范围*/
//    getControlProgram()->system.errorRange = Read_System(SAVE_ERROR_RANGE);
//    if (getControlProgram()->system.errorRange < 0.3)
//        getControlProgram()->system.errorRange = 0.3;
//    else if (getControlProgram()->system.errorRange > 10)
//        getControlProgram()->system.errorRange = 10;
//    DEBUG_LOGI(TAG, "误差范围:%0.1f%%", getControlProgram()->system.errorRange);




//    /*读取输入电流校准数据*/
//    input_calibration_t input_current;
//    Read_System_block(SAVE_INPUT_CURRENT, &input_current, sizeof(input_calibration_t));
//    if (input_current.calibration_max <= input_current.calibration_min)
//        input_current.state = false;
//    if (input_current.state == false)
//    {
//        input_current.calibration_max = 15877;
//        input_current.calibration_min = 3110;
//    }
//    set_input_calibration(&input_current);
//    DEBUG_LOGI(TAG, "输入:%s", input_current.state == true ? "已校准" : "未校准");
//    DEBUG_LOGI(TAG, "输入电流最小值:%d", input_current.calibration_min);
//    DEBUG_LOGI(TAG, "输入电流最大值:%d", input_current.calibration_max);
//    if (input_current.state == false)
//    {
//        input_current.calibration_max = 15877;
//        input_current.calibration_min = 3110;
//    }
}