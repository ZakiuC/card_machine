#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#include "main.h"
#include "bsp_key.h"
#include "TM1639.h"

#define PREPARE_WAITTIME_MAX (3000)
#define BLINK_PERIOD    (300)
// 定义设置菜单的设置项
typedef enum {
    NO_SETTING = 0,
    BASECARD_COUNT_SETING,      // 底牌数量
    PLAYER_COUNT_SETTING,       // 玩家数量
    LAUNCH_COUNT_SETTING,       // 发牌数量
    BURST_COUNT_SETTING,        // 连发数量
    DEALING_MODE_SETTING,       // 发牌模式
    DEALING_ORDER_SETTING       // 出牌顺序
} SettingItem_e;

// 定义发牌模式
typedef enum {
    SWAY_DEAL,              // 左右摇摆发牌
    ROTATE_DEAL             // 单旋转发牌
} DealingMode_e;

// 定义出牌顺序
typedef enum {
    BOTTOM_FIRST_DEAL,      // 底牌先出
    BOTTOM_LAST_DEAL        // 底牌后出
} DealingOrder_e;


// 菜单项结构体
typedef struct {
    SettingItem_e setting;    // 当前设置项
    uint8_t deckCount;          // 底牌数量
    uint8_t playerCount;        // 玩家数量
    uint8_t cardCount;          // 发牌数量
    uint8_t burstCount;         // 连发数量
    DealingMode_e dealMode;   // 发牌模式
    DealingOrder_e dealOrder; // 出牌顺序
} MenuItem_t;

typedef enum
{
    PREPARE_MODE,               
    IDLE_MODE,
    PAUSE_MODE,
    POWERMANGER_MODE,
    SETTING_MODE,
    SAFETY_MODE,
} CtrlMode_e;       // 运行模式


typedef struct
{
    MenuItem_t main_menu;
    MenuItem_t setting_menu;
    CtrlMode_e  ctrl_mode;
    CtrlMode_e  last_mode;
    SettingItem_e setting_mode;
    uint8_t prepare_wait_increment;
    uint32_t prepare_wait_time;
}Console_t;

// 显示信息内容类型枚举
typedef enum{
    NONE_CONTENT,      // 无内容
    DIGITAL_CONTENT,   // 数字
    STRING_CONTENT,    // 字符串
    STRING_DIGITAL_CONTENT, // 字符串+数字
}DisplayContentType_e;

typedef struct 
{
    bool needUpdate;
    DisplayContentType_e content_type;
    uint8_t digital_content[5];
    char string_content[5];
    uint8_t dot_content[5];
    uint8_t start_pos;
    uint8_t start_pos2;
    uint8_t length;
} DisplayInfo_t;

extern DisplayInfo_t displayInfo;
void Console_Init(void);
void Console_ModeSwitch(void);
void Console_msHandle(void);
#endif // _CONSOLE_H_
