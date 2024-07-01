#ifndef _TEST_KEY_H_
#define _TEST_KEY_H_

#include "main.h"


#define PRESEED GPIO_PIN_RESET
#define RELEASED GPIO_PIN_SET

typedef enum
{
    KEY_POWER,
    KEY_TOUCH,
} KeyId_e;

typedef enum
{
    KEY_IDLE,
    KEY_PRESSED,
    KEY_RELEASED,
    KEY_CLICKED,
    KEY_LONG_PRESSED,
    KEY_LONG_PRESSED_BACK
} KeyState_e;

typedef struct
{
    KeyId_e    id;
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState current; // Current state (pressed/released)
    GPIO_PinState last;
    uint32_t press_time;    // Time of press
    KeyState_e state;      // Key state
    KeyState_e last_state; // Key state
} key_t;


void KeyScan(void);
void KeyMsHandle(void);
const key_t *GetKeyInfo(void);
#endif // _TEST_KEY_H_
