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
#define TM1639KEY_DEBOUNCE_TIME 5
// 按键枚举
typedef enum
{
    TM1639KEY_RANDOM,   // SW1  随机发牌键
    TM1639KEY_ADD,      // SW2  加键
    TM1639KEY_SUB,      // SW3  减键
    TM1639KEY_SETTING,  // SW4  设置键
    TM1639KEY_LAUNCH    // SW5  确认键  
} TM1639KeyId_e;
// 按键状态
typedef enum
{
    TM1639KEY_IDLE,
    TM1639KEY_PRESSED,
    TM1639KEY_RELEASED,
    TM1639KEY_CLICKED,
    TM1639KEY_LONG_PRESSED,
    TM1639KEY_LONG_PRESSED_BACK,
} TM1639KeyState_e;

// 按键信息结构体
typedef struct
{
    TM1639KeyId_e    id;
    GPIO_PinState current; // Current state (pressed/released)
    GPIO_PinState last;
    uint32_t press_time;    // Time of press
    TM1639KeyState_e state;      // Key state
    TM1639KeyState_e last_state; // Key state
} TM1639key_t;



void TM1639Clear(void);
void TM1639NumShow(uint8_t nums[], uint8_t dots[], uint8_t start_pos, uint8_t length);
void TM1639LetterShow(char texts[], uint8_t textLength, uint8_t dots[]);
void TM1639RemixShow(char texts[], uint8_t textLength, uint8_t nums[], uint8_t numsLength, uint8_t dots[]);
void TM1639SetBrightness(uint8_t brightness);
void TM1639SetDisplayState(TM1639_Switch_e state); 
void TM1639PowerCtrl(TM1639_Switch_e PinState);
uint16_t TM1639_ReadKey(void);
TM1639_KeyState_e parse_key_status(uint16_t key_value, uint8_t key_number);
void TM1639Init(void);
void TM1639KeyScan(void);
void TM1639MsHandle(void);
TM1639key_t *GetTM1639KeyInfo(void);
void TM1639_Test(void);
void MarqueeDisplay(uint8_t index_num);

#endif // _TM1639_H_
