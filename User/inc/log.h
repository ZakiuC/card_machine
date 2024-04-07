#ifndef __LOG_H_
#define __LOG_H_
#include "SEGGER_RTT.h"

// 定义日志级别
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

// 设置当前日志级别
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// 直接调用SEGGER_RTT_printf并根据日志级别判断是否输出
#define LOG_PRINT(level, type, color, format, ...) \
    do { \
        if (level <= CURRENT_LOG_LEVEL) { \
            SEGGER_RTT_printf(0, "%s%s" format "%s", color, type, ##__VA_ARGS__, RTT_CTRL_RESET); \
        } \
    } while (0)

#define LOG_CLEAR() if (LOG_LEVEL_DEBUG <= CURRENT_LOG_LEVEL) { SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR); }

#define LOG(format, ...) LOG_PRINT(LOG_LEVEL_INFO, "", RTT_CTRL_TEXT_BRIGHT_WHITE, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG_PRINT(LOG_LEVEL_INFO, "LOG: ", RTT_CTRL_TEXT_BRIGHT_WHITE, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG_PRINT(LOG_LEVEL_WARN, "WAR: ", RTT_CTRL_TEXT_BRIGHT_YELLOW, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_PRINT(LOG_LEVEL_ERROR, "ERR: ", RTT_CTRL_TEXT_BRIGHT_RED, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG_PRINT(LOG_LEVEL_DEBUG, "DBG: ", RTT_CTRL_TEXT_BRIGHT_GREEN, format, ##__VA_ARGS__)

#endif  // __LOG_H_
