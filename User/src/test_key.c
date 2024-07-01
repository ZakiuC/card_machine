#include "test_key.h"
#include "log.h"

#define NUM_KEYS 2                   
#define KEY_LONG_PRESS_THRESHOLD 2000 // 长按时长
#define KEY_CLICK_THRESHOLD 500       // 单击时长

key_t keys[NUM_KEYS] = {
    {powerKey_GPIO_Port, powerKey_Pin, 1, 1, 0, KEY_IDLE, KEY_IDLE},
    {touchKey_GPIO_Port, touchKey_Pin, 1, 1, 0, KEY_IDLE, KEY_IDLE},
};


/**
 * @brief 按键扫描
*/
void ScanKey(void)
{
    for (int i = 0; i < NUM_KEYS; i++)
    {
        keys[i].current = HAL_GPIO_ReadPin(keys[i].port, keys[i].pin);

        if (keys[i].state != KEY_LONG_PRESSED)
        {
            if (keys[i].current == 0)
            {
                if (keys[i].last == 1)
                {
                    keys[i].state = KEY_PRESSED;
                    LOG_DEBUG("Key %d pressed.\n", i);
                    keys[i].press_time = 0;
                }
                else
                {
                    if (keys[i].press_time > KEY_LONG_PRESS_THRESHOLD)
                    {
                        keys[i].state = KEY_LONG_PRESSED;
                        LOG_DEBUG("Key %d long pressed.\n", i);
                    }
                }
            }
            else
            {
                if (keys[i].last == 0)
                {
                    keys[i].state = KEY_RELEASED;
                    if (keys[i].press_time < KEY_CLICK_THRESHOLD)
                    {
                        keys[i].state = KEY_CLICKED;
                        LOG_DEBUG("Key %d clicked.\n", i);
                    }
                    else
                    {
                        LOG_DEBUG("Key %d released.\n", i);
                    }
                }
                else
                {
                    keys[i].press_time = 0;
                    keys[i].state = KEY_IDLE;
                }
            }
        }
        else
        {
            keys[i].press_time = 0;
            if (keys[i].current == 1)
            {
                keys[i].state = KEY_RELEASED;
                LOG_DEBUG("Key %d released.\n", i);
            }
        }
        keys[i].last_state = keys[i].state;
        keys[i].last = keys[i].current;
    }
}

/**
 * @brief 按键毫秒处理
*/
void KeyMsHandle(void)
{
    for (int i = 0; i < NUM_KEYS; i++)
    {
        if (keys[i].state == KEY_PRESSED)
        {
            keys[i].press_time++;
        }
    }
}


/**
 * @brief 获取按键信息
 *
 * @return const Key_inf_t* keys   按键信息
 */
const key_t *GetKeyInfo(void)
{
    return keys;
}