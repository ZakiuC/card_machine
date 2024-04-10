#include "bsp_key.h"

static Key_inf_t key_info[KEY_AMOUNT] = {
    {KEY_POWER, KEY_NULL, 1, 0, 0},
    {KEY_TOUCH1, KEY_NULL, 1, 0, 0},
    {KEY_OPTO_LAUNCH, KEY_NULL, 1, 0, 0},
    {KEY_OPTO_ROTATE, KEY_NULL, 1, 0, 0}};

static uint8_t Key_GetPortState(Key_e key_id);

/**
 * @brief 按键初始化
 */
void Key_Init(void)
{
    uint8_t key_i = 0;

    for (key_i = 0; key_i < KEY_AMOUNT; key_i++)
    {
        key_info[key_i].key_state = KEY_NULL;
        key_info[key_i].key_en = 1;
        key_info[key_i].key_delay = 0;
        key_info[key_i].key_step = 0;
    }
}

/**
 * @brief 按键扫描
 */
void Key_Scan(void)
{
    uint8_t key_i = 0;

    for (key_i = 0; key_i < KEY_AMOUNT; key_i++)
    {
        switch (key_info[key_i].key_step)
        {
        case 0:
            if (Key_GetPortState(key_i) == 0)
            {
                key_info[key_i].key_en = ENABLE;
                key_info[key_i].key_delay = KEY_DEBOUNCE_TIME_MS;
                key_info[key_i].key_step = 1;
            }
            else
            {
                key_info[key_i].key_state = KEY_NULL;
                key_info[key_i].key_en = DISABLE;
            }
            break;
        case 1:
            if (key_info[key_i].key_delay == 0)
            {
                if (Key_GetPortState(key_i) == 0)
                {
                    key_info[key_i].key_state = KEY_CLICK;
                    key_info[key_i].key_step = 2;
                }
                else
                {
                    key_info[key_i].key_state = KEY_NULL;
                    key_info[key_i].key_step = 0;
                    key_info[key_i].key_en = DISABLE;
                }
            }
            break;
        case 2:
            if (Key_GetPortState(key_i) == 1)
            {
                key_info[key_i].key_state = KEY_UP;
                key_info[key_i].key_step = 0;
                key_info[key_i].key_en = DISABLE;
            }
            else
            {
                key_info[key_i].key_state = KEY_DOWN;
                key_info[key_i].key_step = 4;
                key_info[key_i].key_delay = KEY_LONG_PRESS_TIME_2S;
                key_info[key_i].key_en = ENABLE;
            }
            break;
        case 4:
            if ((key_info[key_i].key_delay == 0) && (Key_GetPortState(key_i) == 0))
            {
                key_info[key_i].key_state = KEY_LONG_2S;
                key_info[key_i].key_step = 5;
            }
            if (Key_GetPortState(key_i) == 1)
            {
                key_info[key_i].key_state = KEY_UP;
                key_info[key_i].key_step = 0;
                key_info[key_i].key_en = DISABLE;
            }
            break;
        case 5:
            if ((key_info[key_i].key_delay == 0) && (Key_GetPortState(key_i) == 0))
            {
                key_info[key_i].key_state = KEY_LONG_4S;
                key_info[key_i].key_step = 6;
                key_info[key_i].key_en = DISABLE;
            }
            if (Key_GetPortState(key_i) == 1)
            {
                key_info[key_i].key_state = KEY_NULL;
                key_info[key_i].key_step = 0;
                key_info[key_i].key_en = DISABLE;
            }
            break;
        case 6:
            if (Key_GetPortState(key_i) == 1)
            {
                key_info[key_i].key_state = KEY_NULL;
                key_info[key_i].key_step = 0;
                key_info[key_i].key_en = DISABLE;
            }
            break;
        }
    }
}

/**
 * @brief 按键毫秒处理
 */
void Key_msHandle(void)
{
    for (uint8_t i = 0; i < KEY_AMOUNT; i++)
    {
        if (key_info[i].key_delay > 0)
        {
            key_info[i].key_delay--;
        }
    }
}

/**
 * @brief 获取按键信息
 * @param key_id 按键ID
 * @return uint8_t  0:按键按下 1:按键弹起
 */
static uint8_t Key_GetPortState(Key_e key_id)
{
    uint8_t port_state = 1;
    if (key_id < KEY_AMOUNT)
    {
        switch (key_id)
        {
        case KEY_POWER:
            port_state = HAL_GPIO_ReadPin(powerKey_GPIO_Port, powerKey_Pin);
            break;
        case KEY_TOUCH1:
            port_state = HAL_GPIO_ReadPin(touchKey_GPIO_Port, touchKey_Pin);
            break;
        case KEY_OPTO_LAUNCH:
            port_state = HAL_GPIO_ReadPin(outputOptoKey_GPIO_Port, outputOptoKey_Pin);
            if (port_state == 1)
                port_state = 0;
            else
                port_state = 1;
            break;
        case KEY_OPTO_ROTATE:
            port_state = HAL_GPIO_ReadPin(rotateOptoKey_GPIO_Port, rotateOptoKey_Pin);
            if (port_state == 1)
                port_state = 0;
            else
                port_state = 1;
            break;
        default:
            break;
        }

        return port_state;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief 获取按键信息
 *
 * @return const Key_inf_t* key_info   按键信息
 */
const Key_inf_t *GetKeyInfo(void)
{
    return key_info;
}