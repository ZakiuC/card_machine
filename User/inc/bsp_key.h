#ifndef _BSP_KEY_H_
#define _BSP_KEY_H_
#include "main.h"


// 按键数量
#define KEY_AMOUNT 4

// 去抖时间和长按时间定义
#define KEY_DEBOUNCE_TIME_MS 3
#define KEY_LONG_PRESS_TIME_2S 2000
#define KEY_LONG_PRESS_TIME_4S 4000

// 按键枚举
typedef enum
{
    KEY_POWER = 0, // SW6
    KEY_TOUCH1,
    KEY_OPTO_LAUNCH,
    KEY_OPTO_ROTATE,
} Key_e;

// 按键状态
typedef enum
{
    KEY_NULL,
    KEY_DOWN,
    KEY_UP,
    KEY_CLICK,
    KEY_LONG_2S,
    KEY_LONG_4S,
} Key_State_e;

typedef struct
{
    Key_e key_id;
    Key_State_e key_state;
    uint8_t key_en;
    uint16_t key_delay;
    uint8_t key_step;
} Key_inf_t;


void Key_Init(void);
void Key_Scan(void);
void Key_msHandle(void);
const Key_inf_t* GetKeyInfo(void);
#endif // _BSP_KEY_H_
