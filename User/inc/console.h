#ifndef _CONSOLE_H_
#define _CONSOLE_H_
#include "main.h"
#include "bsp_key.h"
#include "TM1639.h"

#define PREPARE_WAITTIME_MAX (3000)
#define MARQUEE_PERIOD    (80)
#define BUZZER_TIME (100)
#define BLINK_PERIOD (350)
// #define BUZZER_ENABLE   1

#define FLASH_USER_START_ADDR   ((uint32_t)0x08007C17) /* 用户Flash区域起始地址 使用flash尾部1000字节的空间*/ 
#define FLASH_USER_END_ADDR     ((uint32_t)0x08007FFF) /* 用户Flash区域结束地址 */


// 定义设置菜单的设置项
typedef enum {
    NO_SETTING = 0,
    BASECARD_COUNT_SETING,      // 底牌数量
    PLAYER_COUNT_SETTING,       // 玩家数量
    LAUNCH_COUNT_SETTING,       // 发牌数量
    BURST_COUNT_SETTING,        // 连发数量
    DEALING_MODE_SETTING,       // 发牌模式
    DEALING_ORDER_SETTING,      // 出牌顺序
    DIRECTION_ROTATE_SETTING    // 旋转方向
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

// 定义旋转方向
typedef enum {
    CLOCKWISE,              // 顺时针
    COUNTER_CLOCKWISE       // 逆时针
} DirRotate_e;

// 定义设置发牌的设置项
typedef enum {
    NO_LAUNCH = 0,
    NORMAL_LAUNCH,      // 发牌
    RANDOM_LAUNCH,       // 随机发牌
    TEST_LAUNCH        // 测试
} LaunchMode_e;

// 菜单项结构体
typedef struct {
    SettingItem_e setting;    // 当前设置项
    uint8_t deckCount;          // 底牌数量
    uint8_t playerCount;        // 玩家数量
    uint8_t cardCount;          // 发牌数量
    uint8_t burstCount;         // 连发数量
    DealingMode_e dealMode;   // 设置的发牌模式
    DealingOrder_e dealOrder; // 出牌顺序
    DirRotate_e dirRotate;      // 旋转方向
    LaunchMode_e launchMode;   // 当前发牌模式
    uint16_t launch_card_num;  // 目标发牌数
    uint16_t launch_deck_num;  // 目标发牌数
    uint8_t launch_card_pos;     // 发牌位置
    uint16_t buzzer_time;      // 蜂鸣器叫的时间
    uint8_t deck_Launch_flag;   // 底牌发牌标志
} MenuItem_t;


typedef enum
{
    PREPARE_MODE,              // 准备模式
    IDLE_MODE,                 // 空闲模式
    PAUSE_MODE,                // 暂停模式
    POWERMANGER_MODE,          // 
    LAUNCH_MODE,               // 发牌模式
    SETPLAYER_LAUNCH_MODE,     // 玩家数量设置模式
    SETTING_MODE,              // 设置模式  
    SAFETY_MODE,               // 安全模式
    CLOSE_MODE,                // 关机模式
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
    uint16_t bat_value[10];
    uint16_t outMotor_value[10];
    uint16_t rotateMotor_value[10];
    uint16_t adBuff_value[30];
}Console_t;

typedef enum{
    UNBLINK,
    ENBLINK
}Blink_e;

typedef enum{
    BLINK_OFF,
    BLINK_ON
}BlinkState_e;

// 显示信息内容类型枚举
typedef enum{
    NONE_CONTENT,      // 无内容
    DIGITAL_CONTENT,   // 数字
    STRING_CONTENT,    // 字符串
    STRING_DIGITAL_CONTENT, // 字符串+数字
    MARQUEE_CONTENT,    // 跑马灯
}DisplayContentType_e;

typedef struct 
{
    bool needUpdate;
    DisplayContentType_e content_type;
    Blink_e blink_en;
    BlinkState_e blink_state;
    uint8_t marQuee_index;
    uint16_t marQuee_period;
    uint8_t digital_content[5];
    char string_content[5];
    uint8_t dot_content[5];
    uint8_t start_pos;
    uint8_t start_pos2;
    uint8_t length;
    uint16_t blink_period;
} DisplayInfo_t;


extern DisplayInfo_t displayInfo;
void ConsoleInit(void);
void ConsoleModeSwitch(void);
void ConsoleMsHandle(void);
void WorkModeSwitch(void);

#endif // _CONSOLE_H_
