/*
 * @Descripttion:
 * @version:
 * @Author: Youliangxin
 * @Date: 2022-08-11 10:29:37
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-22 13:48:44
 */
#ifndef LOG_DEBUG_H__
#define LOG_DEBUG_H__

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "SEGGER_RTT.h"
#include "UserUart1.h"

#define SEGGER_RTT
#define LOG_LOCAL_LEVEL LOG_NONE // 日志调试等级
#define CONFIG_LOG_COLORS 1       // 日志是否启用颜色支持

typedef enum {
    LOG_NONE,     // 不输出
    LOG_ERROR,    // 严重错误，模块自身无法恢复
    LOG_WARN,     // 已采取恢复措施的错误
    LOG_INFO,     // 正常流程信息（默认输出最低级别）
    LOG_DEBUG,    // 调试细节（变量值、指针、尺寸等）
    LOG_VERBOSE   // 大段/高频调试信息
} log_level_t;

#if CONFIG_LOG_COLORS
#define LOG_COLOR_BLACK "30"
#define LOG_COLOR_RED "31"
#define LOG_COLOR_GREEN "32"
#define LOG_COLOR_BROWN "33"
#define LOG_COLOR_BLUE "34"
#define LOG_COLOR_PURPLE "35"
#define LOG_COLOR_CYAN "36"
#define LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR) "\033[1;" COLOR "m"
#define LOG_RESET_COLOR "\033[0m"
#define LOG_COLOR_E LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_COLOR_V
#else // CONFIG_LOG_COLORS
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_COLOR_V
#define LOG_RESET_COLOR
#endif // CONFIG_LOG_COLORS

static void log_write(log_level_t level, const char *format, ...)
{
    va_list list;
    va_start(list, format);
#ifdef SEGGER_RTT
    SEGGER_RTT_vprintf(0, format, &list);
#else
    static char buffer[128];
    // vprintf(format, list);
    vsnprintf(buffer, sizeof(buffer), format, list);
    va_end(list);
    buffer[sizeof(buffer) - 1] = '\0';
    UserUart1_send_data(buffer, strlen(buffer));
#endif
}

#define LOG_FORMAT(letter, format) LOG_COLOR_##letter #letter " %s : " format LOG_RESET_COLOR "\r\n"

#define LOG_LEVEL(level, tag, format, ...)                                     \
    do                                                                         \
    {                                                                          \
        if (level == LOG_ERROR)                                                \
        {                                                                      \
            log_write(LOG_ERROR, LOG_FORMAT(E, format), tag, ##__VA_ARGS__);   \
        }                                                                      \
        else if (level == LOG_WARN)                                            \
        {                                                                      \
            log_write(LOG_WARN, LOG_FORMAT(W, format), tag, ##__VA_ARGS__);    \
        }                                                                      \
        else if (level == LOG_DEBUG)                                           \
        {                                                                      \
            log_write(LOG_DEBUG, LOG_FORMAT(D, format), tag, ##__VA_ARGS__);   \
        }                                                                      \
        else if (level == LOG_VERBOSE)                                         \
        {                                                                      \
            log_write(LOG_VERBOSE, LOG_FORMAT(V, format), tag, ##__VA_ARGS__); \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            log_write(LOG_INFO, LOG_FORMAT(I, format), tag, ##__VA_ARGS__);    \
        }                                                                      \
    } while (0)

#define LOG_LEVEL_LOCAL(level, tag, format, ...)          \
    do                                                    \
    {                                                     \
        if (LOG_LOCAL_LEVEL >= level)                     \
            LOG_LEVEL(level, tag, format, ##__VA_ARGS__); \
    } while (0)

#define DEBUG_LOGE(tag, format, ...) LOG_LEVEL_LOCAL(LOG_ERROR, tag, format, ##__VA_ARGS__)
#define DEBUG_LOGW(tag, format, ...) LOG_LEVEL_LOCAL(LOG_WARN, tag, format, ##__VA_ARGS__)
#define DEBUG_LOGI(tag, format, ...) LOG_LEVEL_LOCAL(LOG_INFO, tag, format, ##__VA_ARGS__)
#define DEBUG_LOGD(tag, format, ...) LOG_LEVEL_LOCAL(LOG_DEBUG, tag, format, ##__VA_ARGS__)
#define DEBUG_LOGV(tag, format, ...) LOG_LEVEL_LOCAL(LOG_VERBOSE, tag, format, ##__VA_ARGS__)
#define DEBUG_PRINT(format, ...) log_write(LOG_NONE, format, ##__VA_ARGS__)

#define debug(info, ...) DEBUG_LOGD("", info, ##__VA_ARGS__)

#endif
