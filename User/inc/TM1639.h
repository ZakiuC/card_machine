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
    KEY_UP = 0U,
    KEY_DOWN
} TM1639_KeyState_e; // TM1639按键状态枚举

void TM1639_Clear(void);
void TM1639_NumShow(uint8_t nums[], uint8_t dots[], uint8_t length);
// void TM1639_Show(uint8_t a1, uint8_t b1, uint8_t a2, uint8_t b2, uint8_t a3, uint8_t b3, uint8_t a4, uint8_t b4, uint8_t a5, uint8_t b5, uint8_t a6, uint8_t b6, uint8_t a7, uint8_t b7, uint8_t a8, uint8_t b8);	//����� ������ �� �������
void TM1639_SetBrightness(uint8_t brightness);
void TM1639_SetDisplayState(TM1639_Switch_e state);
void TM1639_PowerCtrl(TM1639_Switch_e PinState);
uint16_t TM1639_ReadKey(void);
TM1639_KeyState_e parse_key_status(uint16_t key_value, uint8_t key_number);
void TM1639_Init(void);
void TM1639_Test(void);

#endif // _TM1639_H_
