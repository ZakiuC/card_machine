#ifndef _TM1639_H_
#define _TM1639_H_
#include "main.h"

typedef enum
{
    TM1639_OFF = 0U,
    TM1639_ON
} TM1639_Switch_e; // TM1639开关变量

typedef enum
{
    TM1639_KEY_UP = 0U,
    TM1639_KEY_DOWN
} TM1639_KeyState_e; // TM1639按键状态枚举


// 按键数量
#define TM1639KEY_AMOUNT 5

// 长按时间定义
#define TM1639KEY_LONG_PRESS_TIME_2S 2000
#define TM1639KEY_LONG_PRESS_TIME_4S 4000

// 按键枚举
typedef enum
{
    TM1639KEY_RANDOM = 0, // SW1  随机发牌键
    TM1639KEY_ADD,        // SW2  加键
    TM1639KEY_SUB,        // SW3  减键
    TM1639KEY_SETTING,    // SW4  设置键
    TM1639KEY_LAUNCH,     // SW5  确认键
} TM1639Key_e;

// 按键状态
typedef enum
{
    TM1639KEY_NULL,
    TM1639KEY_DOWN,
    TM1639KEY_UP,
    TM1639KEY_CLICK,
    TM1639KEY_LONG_2S,
    TM1639KEY_LONG_4S,
} TM1639KeyState_e;

// 按键信息结构体
typedef struct
{
    TM1639Key_e key_id;
    TM1639KeyState_e key_state;
    uint8_t key_en;
    uint16_t key_delay;
    uint8_t key_step;
} TM1639Key_inf_t;

void TM1639_Clear(void);
void TM1639_NumShow(uint8_t nums[], uint8_t dots[], uint8_t start_pos, uint8_t length);
void TM1639_LetterShow(char texts[], uint8_t textLength, uint8_t dots[]);
void TM1639_RemixShow(char texts[], uint8_t textLength, uint8_t nums[], uint8_t numsLength, uint8_t dots[]);
void TM1639_SetBrightness(uint8_t brightness);
void TM1639_SetDisplayState(TM1639_Switch_e state);
void TM1639_PowerCtrl(TM1639_Switch_e PinState);
uint16_t TM1639_ReadKey(void);
TM1639_KeyState_e parse_key_status(uint16_t key_value, uint8_t key_number);
void TM1639_Init(void);
void TM1639_KeyScan(void);
void TM1639_msHandle(void);
const TM1639Key_inf_t *GetTM1639KeyInfo(void);
void TM1639_Test(void);

#endif // _TM1639_H_
