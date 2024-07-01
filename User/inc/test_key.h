#ifndef _TEST_KEY_H_
#define _TEST_KEY_H_

#include "main.h"

// Define key states
typedef enum
{
    KEY_IDLE,
    KEY_PRESSED,
    KEY_RELEASED,
    KEY_CLICKED,
    KEY_LONG_PRESSED,
} key_state_t;

// Define key structure
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t current; // Current state (pressed/released)
    uint8_t last;
    uint32_t press_time;    // Time of press
    key_state_t state;      // Key state
    key_state_t last_state; // Key state
} key_t;


void ScanKey(void);
void KeyMsHandle(void);
const key_t *GetKeyInfo(void);
#endif // _TEST_KEY_H_
