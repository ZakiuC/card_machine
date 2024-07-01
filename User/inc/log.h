#ifndef __LOG_H_
#define __LOG_H_
#include "SEGGER_RTT.h"
#include "stdarg.h"


// 定义日志级别
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} LogLevel;

// 设置当前日志级别
static const LogLevel CURRENT_LOG_LEVEL = LOG_LEVEL_DEBUG;

/**
 * @brief 日志打印函数
 * @param level 日志级别
 * @param type 日志类型
 * @param color 日志颜色
 * @param format 日志格式
*/
static inline void LOG_PRINT(LogLevel level, const char* type, const char* color, const char* format, ...) {
    if (level <= CURRENT_LOG_LEVEL) {
        va_list args;
        va_start(args, format);
        SEGGER_RTT_printf(0, "%s%s", color, type);
        SEGGER_RTT_vprintf(0, format, &args); // 修改此处，传递args的地址
        SEGGER_RTT_printf(0, "%s", RTT_CTRL_RESET);
        va_end(args);
    }
}

/**
 * @brief 清空日志
*/
static inline void LOG_CLEAR() {
    if (LOG_LEVEL_DEBUG <= CURRENT_LOG_LEVEL) {
        SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR);
    }
}

#define LOG(format, ...) LOG_PRINT(LOG_LEVEL_INFO, "", RTT_CTRL_TEXT_BRIGHT_WHITE, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG_PRINT(LOG_LEVEL_INFO, "LOG: ", RTT_CTRL_TEXT_BRIGHT_WHITE, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG_PRINT(LOG_LEVEL_WARN, "WAR: ", RTT_CTRL_TEXT_BRIGHT_YELLOW, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_PRINT(LOG_LEVEL_ERROR, "ERR: ", RTT_CTRL_TEXT_BRIGHT_RED, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG_PRINT(LOG_LEVEL_DEBUG, "DBG: ", RTT_CTRL_TEXT_BRIGHT_GREEN, format, ##__VA_ARGS__)

#endif  // __LOG_H_
