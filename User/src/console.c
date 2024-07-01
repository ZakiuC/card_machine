#include "console.h"
#include "bsp_key.h"
#include "TM1639.h"
#include <string.h>
#include "log.h"
#include "motor.h"
#include "adc.h"
#include "flash_operation.h"
#include "FreeRTOS.h"
#include "task.h"

/* 全局变量 ------------------------------------------------------------------*/
// 显示信息
DisplayInfo_t displayInfo = {
    .needUpdate = false,
    .content_type = DIGITAL_CONTENT,
    .blink_en = UNBLINK,
    .blink_state = BLINK_OFF,
    .blink_period = 0,
    .digital_content = {0},
    .string_content = {0},
    .dot_content = {0},
    .start_pos = 0,
    .start_pos2 = 0,
    .length = 0,
    .marQuee_index = 0,
    .marQuee_period = MARQUEE_PERIOD,
};

/* 私有变量 ------------------------------------------------------------------*/
// 历史显示数据 - 用于确认是否需要刷新显示内容，后续可用以优化显示刷新
DisplayInfo_t last_displayInfo = {
    .needUpdate = false,
    .content_type = DIGITAL_CONTENT,
    .blink_en = UNBLINK,
    .blink_state = BLINK_OFF,
    .blink_period = 0,
    .digital_content = {0},
    .string_content = {0},
    .dot_content = {0},
    .start_pos = 0,
    .start_pos2 = 0,
    .length = 0,
    .marQuee_index = 0,
    .marQuee_period = MARQUEE_PERIOD,
};

// 控制结构体
Console_t console = {
    .main_menu = {
        .setting = NO_SETTING,
        .dealMode = SWAY_DEAL,
        .dealOrder = BOTTOM_FIRST_DEAL,
        .dirRotate = CLOCKWISE,
        .deckCount = 3,
        .playerCount = 3,
        .cardCount = 17,
        .burstCount = 17,
        .launch_card_num = 0,
        .launch_card_pos = 0,
        .buzzer_time = 0}};

// 电机
Motor_t motor[2] = {
    {.id = OUTMOTOR,
     .current_pos = 0,
     .cards = 0,
     .totalCards = 0,
     .direction = MOTOR_STOP},
    {.id = ROTATEMOTOR,
     .current_pos = 0,
     .cards = 0,
     .totalCards = 0,
     .direction = MOTOR_STOP}};

// 按键数据
const key_t *key_info = NULL;
TM1639key_t *tm1639_key_info = NULL;

uint8_t view[2] = {0};

/* 函数声明 ------------------------------------------------------------------*/
static void PrepareMenu_handle(void);
static void IdleMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e setting_key);
static void SetPlayerLaunchMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e add_key, TM1639KeyState_e sub_key);
static void SettingMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e setting_key,
                               TM1639KeyState_e add_key, TM1639KeyState_e sub_key);
static void LaunchMenu_handle(void);
static void PauseMenu_handle(TM1639KeyState_e launch_key, KeyState_e power_key);
static void SafetyMenu_handle(void);
static void CloseMenu_handle(void);

static void SettingPlayerSwitch(int8_t delta);
static void SettingSwitch(SettingItem_e item, int8_t delta);
static void ModeSwitch(Console_t *console, CtrlMode_e target_mode);
static void updateMenuDisplayNum(uint8_t menu_display_num[5], const MenuItem_t *menuItem);
static void limitValue(uint8_t *value, uint8_t min, uint8_t max);
static void checkDisplayUpdate(void);
static void buzzerWork(void);
static void setBuzzer(void);
static void recoverLowPowerMode(void);

static HAL_StatusTypeDef SaveConsoleSettings(Console_t *consoleSettings);
static void LoadConsoleSettings(Console_t *consoleSettings);
static void volValueUpdate(void);

/* 函数体 --------------------------------------------------------------------*/
/**
 * @brief ms更新
 */
void ConsoleMsHandle(void)
{
    KeyMsHandle();
    TM1639MsHandle();
    console.prepare_wait_time += console.prepare_wait_increment;
    if (displayInfo.marQuee_period > 0)
    {
        displayInfo.marQuee_period--;
    }
    if (console.main_menu.buzzer_time > 0)
    {
        console.main_menu.buzzer_time--;
    }
    if (displayInfo.blink_period > 0)
    {
        displayInfo.blink_period--;
    }
}

/**
 * @brief 控制台初始化
 */
void ConsoleInit(void)
{
    console.ctrl_mode = PREPARE_MODE;
    console.last_mode = console.ctrl_mode;
    console.setting_mode = NO_SETTING;
    console.prepare_wait_time = 0;
    console.prepare_wait_increment = 1;

    // 读取flash保存的设置内容
    // vTaskSuspendAll();
    // LoadConsoleSettings(&console);      //
    // xTaskResumeAll();

    LOG("\n\n///////////////////////\nstart running.\n");
    key_info = GetKeyInfo(); // mark: 等待初始化完再绑定，下同
    LOG_INFO("Key initialization succeeded.\n");
    TM1639Init();
    tm1639_key_info = GetTM1639KeyInfo();
    LOG_INFO("TM1639 initialization succeeded.\n");
    TM1639PowerCtrl(TM1639_ON);
    LOG_INFO("TM1639 power on.\n");
    console.ctrl_mode = IDLE_MODE;
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        LOG_ERROR("ADC Calibration error.\n");
        Error_Handler();
    }
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)console.adBuff_value, 30);
    LOG_INFO("start power volt update.\n");

    console.prepare_wait_increment = 0;
    console.prepare_wait_time = 0;
    LOG_INFO("Machine is in idle mode.\n");
}


/**
 * @brief 无尽狂潮
 */
void sytryy()
{

}

/**
 * @brief 控制台模式切换
 */
void ConsoleModeSwitch(void)
{
    // 全局[响应：1.SW6(长按关机) 2.touch 单击暂停]
    TM1639KeyState_e random, add, sub, setting, launch;
    KeyState_e power, touch;
    random = tm1639_key_info[TM1639KEY_RANDOM].state;   // SW1
    add = tm1639_key_info[TM1639KEY_ADD].state;         // SW2
    sub = tm1639_key_info[TM1639KEY_SUB].state;         // SW3
    setting = tm1639_key_info[TM1639KEY_SETTING].state; // SW4
    launch = tm1639_key_info[TM1639KEY_LAUNCH].state;   // SW5

    power = key_info[KEY_POWER].state; // SW6
    touch = key_info[KEY_TOUCH].state; // touch

    if (touch == KEY_PRESSED && console.ctrl_mode != PAUSE_MODE && console.ctrl_mode != SAFETY_MODE && console.ctrl_mode != SETTING_MODE)
    {
        // 碰到touch: 进入暂停模式，暂停所有的工作
        ModeSwitch(&console, PAUSE_MODE);
    }
    if (power == KEY_LONG_PRESSED)
    {
        // 长按SW6: 关机
        ModeSwitch(&console, CLOSE_MODE);
    }
    else if (random == TM1639KEY_LONG_PRESSED && console.ctrl_mode != PAUSE_MODE && console.ctrl_mode != SAFETY_MODE)
    {
        // 长按SW1: 进入选择‘位’，再发牌
        ModeSwitch(&console, SETPLAYER_LAUNCH_MODE);
    }
    else if (setting == TM1639KEY_LONG_PRESSED && console.ctrl_mode != PAUSE_MODE && console.ctrl_mode != SAFETY_MODE)
    {
        // 长按SW4: 进旋转方向设置
        LOG_DEBUG("Enter direction of rotation setting mode.\n");
        // 设置项移动到旋转方向
        console.main_menu.setting = DIRECTION_ROTATE_SETTING;
        ModeSwitch(&console, SETTING_MODE);
    }

    switch (console.ctrl_mode)
    {
    case PREPARE_MODE:
        PrepareMenu_handle();
        break;
    case IDLE_MODE:
        IdleMenu_handle(launch, random, setting);
        break;
    case SETPLAYER_LAUNCH_MODE:
        SetPlayerLaunchMenu_handle(launch, random, add, sub);
        break;
    case SETTING_MODE:
        SettingMenu_handle(launch, random, setting, add, sub);
        break;
    case LAUNCH_MODE:
        LaunchMenu_handle();
        break;
    case PAUSE_MODE:
        PauseMenu_handle(launch, power);
        break;
    case SAFETY_MODE:
        SafetyMenu_handle();
        break;
    case CLOSE_MODE:
        CloseMenu_handle();
        break;
    default:
        break;
    }

    checkDisplayUpdate();
    buzzerWork();

    volValueUpdate();
}

/**
 * @brief 检查是否需要刷新
 */
static void checkDisplayUpdate(void)
{
    // 默认认为需要更新
    displayInfo.needUpdate = true;

    // 控制选中项闪烁
    if (displayInfo.blink_en == ENBLINK)
    {
        if (displayInfo.blink_period == 0)
        {
            displayInfo.blink_state = !displayInfo.blink_state;
            displayInfo.blink_period = BLINK_PERIOD;
        }
    }

    // 如果内容类型相同，则继续检查具体内容
    if (displayInfo.content_type == last_displayInfo.content_type)
    {
        // 检查点内容是否相同
        if (memcmp(&displayInfo.dot_content, &last_displayInfo.dot_content, sizeof(displayInfo.dot_content)) == 0)
        {
            bool isSame = false; // 用于记录除类型和点内容外，其他内容是否相同

            switch (displayInfo.content_type)
            {
            case DIGITAL_CONTENT:
                isSame = memcmp(&displayInfo.digital_content, &last_displayInfo.digital_content, sizeof(displayInfo.digital_content)) == 0 &&
                         displayInfo.start_pos == last_displayInfo.start_pos &&
                         displayInfo.length == last_displayInfo.length;
                break;
            case STRING_CONTENT:
                isSame = memcmp(&displayInfo.string_content, &last_displayInfo.string_content, sizeof(displayInfo.string_content)) == 0 &&
                         displayInfo.start_pos == last_displayInfo.start_pos &&
                         displayInfo.length == last_displayInfo.length;
                break;
            case STRING_DIGITAL_CONTENT:
                isSame = memcmp(&displayInfo.string_content, &last_displayInfo.string_content, sizeof(displayInfo.string_content)) == 0 &&
                         memcmp(&displayInfo.digital_content, &last_displayInfo.digital_content, sizeof(displayInfo.digital_content)) == 0 &&
                         displayInfo.start_pos == last_displayInfo.start_pos &&
                         displayInfo.start_pos2 == last_displayInfo.start_pos2 &&
                         displayInfo.length == last_displayInfo.length;
                break;
            case MARQUEE_CONTENT:
                isSame = displayInfo.marQuee_index == last_displayInfo.marQuee_index;
                break;
            default:
                // 默认更新，防止出现意外类型卡死
                isSame = true;
                break;
            }

            // 如果所有检查项都相同，则不需要更新
            if (isSame)
            {
                displayInfo.needUpdate = false;
            }
        }
    }

    // 同步历史记录，
    memcpy(&last_displayInfo.digital_content, &displayInfo.digital_content, sizeof(displayInfo.digital_content));
    memcpy(&last_displayInfo.string_content, &displayInfo.string_content, sizeof(displayInfo.string_content));
    memcpy(&last_displayInfo.dot_content, &displayInfo.dot_content, sizeof(displayInfo.dot_content));
    last_displayInfo.content_type = displayInfo.content_type;
    last_displayInfo.start_pos = displayInfo.start_pos;
    last_displayInfo.start_pos2 = displayInfo.start_pos2;
    last_displayInfo.length = displayInfo.length;
    last_displayInfo.marQuee_index = displayInfo.marQuee_index;
    last_displayInfo.marQuee_period = displayInfo.marQuee_period;
}

/**
 * @brief 准备模式处理
 */
static void PrepareMenu_handle(void)
{
    // 初始化超时检测，进入安全模式
    if (console.prepare_wait_time > PREPARE_WAITTIME_MAX)
    {
        ModeSwitch(&console, SAFETY_MODE);
    }
}

/**
 * @brief 空闲模式处理
 * @param launch_key 发牌键
 * @param random_key 随机键
 * @param setting_key 设置键
 */
static void IdleMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e setting_key)
{
    uint8_t menu_display_num[5] = {0};
    uint8_t menu_display_dot[5] = {0, 1, 1, 0, 0};

    // 主菜单[显示底 位 张]
    if (launch_key == TM1639KEY_CLICKED)
    {
        // 单击SW5: 当前方向发‘张数’牌
        console.main_menu.launchMode = NORMAL_LAUNCH;
        ModeSwitch(&console, LAUNCH_MODE);
    }
    else if (random_key == TM1639KEY_CLICKED)
    {
        // 单击SW1: 随机选择位发牌
        console.main_menu.launchMode = RANDOM_LAUNCH;
        ModeSwitch(&console, LAUNCH_MODE);
    }
    else if (setting_key == TM1639KEY_CLICKED)
    {
        LOG_DEBUG("Enter setting mode.\n");
        // 设置项移动到底牌数量
        console.main_menu.setting = BASECARD_COUNT_SETING;
        // 单击SW4: 进数值和模式的设置
        ModeSwitch(&console, SETTING_MODE);
    }
    else
    {
        // 无操作
    }

    updateMenuDisplayNum(displayInfo.digital_content, &console.main_menu);
    memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
    displayInfo.start_pos = 0;
    displayInfo.length = 5;
    displayInfo.content_type = DIGITAL_CONTENT;
}

/**
 * @brief 设置玩家人数处理
 * @param launch_key    发牌键
 * @param random_key    随机发牌键
 * @param add_key
 */
static void SetPlayerLaunchMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e add_key, TM1639KeyState_e sub_key)
{
    int8_t delta = 0;

    // 设置菜单[位]
    if (launch_key == TM1639KEY_CLICKED || random_key == TM1639KEY_CLICKED)
    {
        // 单击SW5/SW1: 保存当前设置后发牌
        memcpy(&console.main_menu, &console.setting_menu, sizeof(console.setting_menu));
        ModeSwitch(&console, IDLE_MODE);
    }
    else if (add_key == TM1639KEY_CLICKED)
    {
        // 单击SW2: 增加当前选中项的值
        delta = 1;
        setBuzzer();
        LOG_INFO("set player num +1\n", console.setting_menu.setting);
    }
    else if (sub_key == TM1639KEY_CLICKED)
    {
        // 单击SW3: 减少当前选中项的值
        delta = -1;
        setBuzzer();
        LOG_INFO("set player num -1\n", console.setting_menu.setting);
    }

    SettingPlayerSwitch(delta);
    // 设置项切换至单独状态
}

/**
 * @brief 设置模式处理
 * @param launch_key 发牌键
 * @param random_key 随机键
 * @param setting_key 设置键
 * @param add_key 加键
 * @param sub_key 减键
 */
static void SettingMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e setting_key,
                               TM1639KeyState_e add_key, TM1639KeyState_e sub_key)
{
    int8_t delta = 0;

    // 设置菜单[显示底 位 张]
    if (launch_key == TM1639KEY_CLICKED || random_key == TM1639KEY_CLICKED)
    {
        // 单击SW5/SW1: 保存当前设置，返回主菜单
        memcpy(&console.main_menu, &console.setting_menu, sizeof(console.setting_menu));
        ModeSwitch(&console, IDLE_MODE);
    }
    else if (setting_key == TM1639KEY_CLICKED)
    {
        // 单击SW4
        // 单击SW4: 切换下一项设置
        console.main_menu.setting++;
        // 当玩家人数在2/3时才会进入 左右发牌/单旋转发牌模式 的选择
        if (console.main_menu.setting == DEALING_MODE_SETTING)
        {
            if (console.setting_menu.playerCount != 2 && console.setting_menu.playerCount != 3)
            {
                console.main_menu.setting++;
            }
        }

        if (console.main_menu.setting > DEALING_ORDER_SETTING || console.main_menu.setting > DIRECTION_ROTATE_SETTING)
        {
            // 保存设置返回主菜单,设置项返回初始位置
            memcpy(&console.main_menu, &console.setting_menu, sizeof(console.setting_menu));
            console.main_menu.setting = NO_SETTING;
            ModeSwitch(&console, IDLE_MODE);
        }
        else
        {
            LOG_INFO("setting item move to %d\n", console.main_menu.setting);
        }
    }
    else if (add_key == TM1639KEY_CLICKED)
    {
        // 单击SW2: 增加当前选中项的值
        delta = 1;
        setBuzzer();
    }
    else if (sub_key == TM1639KEY_CLICKED)
    {
        // 单击SW3: 减少当前选中项的值
        delta = -1;
        setBuzzer();
    }
    SettingSwitch(console.main_menu.setting, delta);
}

/**
 * @brief 发牌工作模式处理
 */
static void LaunchMenu_handle(void)
{
    // 发牌数量更新
    console.main_menu.launch_card_num = console.main_menu.cardCount;
    console.main_menu.launch_deck_num = console.main_menu.deckCount;

    // 角度设置
    console.main_menu.launch_card_pos = console.main_menu.playerCount;
    console.main_menu.deck_Launch_flag = 0;

    switch (console.main_menu.launchMode)
    {
    case NO_LAUNCH:
        ModeSwitch(&console, IDLE_MODE);
        break;

    case NORMAL_LAUNCH:
        break;

    case RANDOM_LAUNCH:
        break;

    default:
        break;
    }
}

/**
 * @brief 暂停模式处理
 * @param launch_key 发牌键
 * @param random_key 随机键
 */
static void PauseMenu_handle(TM1639KeyState_e launch_key, KeyState_e power_key)
{
    uint8_t menu_display_dot[5] = {0};
    char menu_display_string[5] = {'\0'};

    if (launch_key == TM1639KEY_CLICKED)
    {
        ModeSwitch(&console, console.last_mode);
    }
    else if (power_key == KEY_CLICKED)
    {
        ModeSwitch(&console, IDLE_MODE);
    }
    console.main_menu.launchMode = NO_LAUNCH;
    outMotorStop(&motor[OUTMOTOR]);
    rotateMotorStop(&motor[ROTATEMOTOR]);
    displayInfo.content_type = STRING_CONTENT;
    menu_display_string[0] = 'P';
    menu_display_string[1] = 'A';
    menu_display_string[2] = 'U';
    menu_display_string[3] = 'S';
    menu_display_string[4] = 'E';
    menu_display_dot[0] = 0;
    menu_display_dot[1] = 0;
    menu_display_dot[2] = 0;
    menu_display_dot[3] = 0;
    menu_display_dot[4] = 0;
    memcpy(&displayInfo.string_content, &menu_display_string, sizeof(menu_display_string));
    memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
    displayInfo.start_pos = 0;
    displayInfo.length = 5;
}

/**
 * @brief 安全模式处理
 */
static void SafetyMenu_handle(void)
{
    // 目前只有初始化超时时会进入
    LOG_INFO("Machine is in safe mode.\n");
    rotateMotorStop(&motor[ROTATEMOTOR]);
    outMotorStop(&motor[OUTMOTOR]);

    // 挂起其它任务
    vTaskSuspendAll();
}

/**
 * @brief 关机模式处理
 */
static void CloseMenu_handle(void)
{
    if (console.main_menu.buzzer_time == 0) // 等待蜂鸣器停止工作
    {
        TM1639PowerCtrl(TM1639_OFF);
        LOG_INFO("Machine is power off.\n");

        // 关机前处理
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // 关闭蜂鸣器
        // Low_Power_MX_GPIO_Init();                              // 低功耗IO初始化

        HAL_ADC_DeInit(&hadc1); // 关闭ADC

        // 挂起其他任务

        // 保存当前数据
        // SaveConsoleSettings(&console);        // bug: 进入低功耗前写入flash会无法唤醒

        // 进入低功耗
        HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

        recoverLowPowerMode();
        ConsoleInit();
    }
}

/**
 * @brief 设置项[位]切换处理
 * @param delta 增量
 */
static void SettingPlayerSwitch(int8_t delta)
{
    uint8_t menu_display_num[5] = {0};
    uint8_t menu_display_dot[5] = {0, 1, 1, 0, 0};
    char menu_display_string[5] = {'\0'};

    console.setting_menu.playerCount += delta;
    limitValue(&console.setting_menu.playerCount, 0, 8);
    displayInfo.content_type = DIGITAL_CONTENT;
    if (last_displayInfo.digital_content[0] == 10 && last_displayInfo.digital_content[1] == 10)
    {
        menu_display_num[0] = console.setting_menu.deckCount / 10;
        menu_display_num[1] = console.setting_menu.deckCount % 10;
        menu_display_num[2] = console.setting_menu.playerCount % 10;
        menu_display_dot[0] = 0;
        menu_display_dot[1] = 1;
        menu_display_dot[2] = 1;
        menu_display_dot[3] = 0;
        menu_display_dot[4] = 0;
        memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
        memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
        displayInfo.start_pos = 0;
        displayInfo.length = 3;
    }
    else
    {
        if (displayInfo.blink_state == BLINK_OFF)
        {
            menu_display_num[0] = 10;
        }
        else
        {
            menu_display_num[0] = console.setting_menu.playerCount % 10;
        }
        menu_display_dot[0] = 1;
        menu_display_dot[1] = 0;
        menu_display_dot[2] = 0;
        menu_display_dot[3] = 0;
        menu_display_dot[4] = 0;
        memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
        memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
        displayInfo.start_pos = 2;
        displayInfo.length = 1;
    }
}

/**
 * @brief 设置项切换处理
 * @param item 当前设置项
 * @param delta 增量
 */
static void SettingSwitch(SettingItem_e item, int8_t delta)
{
    uint8_t menu_display_num[5] = {0};
    uint8_t menu_display_dot[5] = {0, 1, 1, 0, 0};
    char menu_display_string[5] = {'\0'};

    // 更新选中项数值
    switch (item)
    {
    case BASECARD_COUNT_SETING: // 底牌数量变更
        console.setting_menu.deckCount += delta;
        limitValue(&console.setting_menu.deckCount, 0, 99);
        displayInfo.content_type = DIGITAL_CONTENT;
        if (displayInfo.blink_state == BLINK_OFF)
        {
            menu_display_num[0] = 10;
            menu_display_num[1] = 10;
        }
        else
        {
            menu_display_num[0] = console.setting_menu.deckCount / 10;
            menu_display_num[1] = console.setting_menu.deckCount % 10;
        }
        menu_display_dot[0] = 0;
        menu_display_dot[1] = 1;
        menu_display_dot[2] = 1;
        menu_display_dot[3] = 0;
        menu_display_dot[4] = 0;
        memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));

        memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
        displayInfo.start_pos = 0;
        displayInfo.length = 2;
        break;
    case PLAYER_COUNT_SETTING: // 玩家数量变更
        console.setting_menu.playerCount += delta;
        limitValue(&console.setting_menu.playerCount, 0, 8);
        displayInfo.content_type = DIGITAL_CONTENT;
        if (last_displayInfo.digital_content[0] == 10 && last_displayInfo.digital_content[1] == 10)
        {
            menu_display_num[0] = console.setting_menu.deckCount / 10;
            menu_display_num[1] = console.setting_menu.deckCount % 10;
            menu_display_num[2] = console.setting_menu.playerCount % 10;
            menu_display_dot[0] = 0;
            menu_display_dot[1] = 1;
            menu_display_dot[2] = 1;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 0;
            displayInfo.length = 3;
        }
        else
        {
            if (displayInfo.blink_state == BLINK_OFF)
            {
                menu_display_num[0] = 10;
            }
            else
            {
                menu_display_num[0] = console.setting_menu.playerCount % 10;
            }
            menu_display_dot[0] = 1;
            menu_display_dot[1] = 0;
            menu_display_dot[2] = 0;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 2;
            displayInfo.length = 1;
        }
        break;
    case LAUNCH_COUNT_SETTING: // 发牌数量变更
        console.setting_menu.cardCount += delta;
        limitValue(&console.setting_menu.cardCount, 0, 99);
        displayInfo.content_type = DIGITAL_CONTENT;
        if (last_displayInfo.digital_content[0] == 10 && last_displayInfo.digital_content[1] == 0)
        {
            menu_display_num[0] = console.setting_menu.playerCount % 10;
            menu_display_num[1] = console.setting_menu.cardCount / 10;
            menu_display_num[2] = console.setting_menu.cardCount % 10;
            menu_display_dot[0] = 1;
            menu_display_dot[1] = 0;
            menu_display_dot[2] = 0;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 2;
            displayInfo.length = 3;
        }
        else
        {
            if (displayInfo.blink_state == BLINK_OFF)
            {
                menu_display_num[0] = 10;
                menu_display_num[1] = 10;
            }
            else
            {
                menu_display_num[0] = console.setting_menu.cardCount / 10;
                menu_display_num[1] = console.setting_menu.cardCount % 10;
            }
            menu_display_dot[0] = 0;
            menu_display_dot[1] = 0;
            menu_display_dot[2] = 0;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 3;
            displayInfo.length = 2;
        }

        break;
    case BURST_COUNT_SETTING: // 连发数量变更
        console.setting_menu.burstCount += delta;
        limitValue(&console.setting_menu.burstCount, 0, 99);
        displayInfo.content_type = STRING_DIGITAL_CONTENT;
        if (displayInfo.blink_state == BLINK_OFF)
        {
            menu_display_num[0] = 10;
            menu_display_num[1] = 10;
        }
        else
        {
            menu_display_num[0] = console.setting_menu.burstCount / 10;
            menu_display_num[1] = console.setting_menu.burstCount % 10;
        }
        // 显示连发
        menu_display_string[0] = 'L';
        menu_display_string[1] = 'F';
        menu_display_string[2] = '-';
        menu_display_dot[0] = 0;
        menu_display_dot[1] = 0;
        menu_display_dot[2] = 0;
        menu_display_dot[3] = 0;
        menu_display_dot[4] = 0;

        // 写入显示数据
        memcpy(&displayInfo.string_content, &menu_display_string, sizeof(menu_display_string));
        memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
        memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
        displayInfo.start_pos = 0;
        displayInfo.start_pos2 = 3;
        displayInfo.length = 5;
        break;
    case DEALING_MODE_SETTING: // 发牌模式变更
        if (delta != 0)
        {
            if (console.setting_menu.dealMode == ROTATE_DEAL)
            {
                console.setting_menu.dealMode = SWAY_DEAL;
            }
            else
            {
                console.setting_menu.dealMode = ROTATE_DEAL;
            }
        }
        displayInfo.content_type = STRING_CONTENT;
        if (displayInfo.blink_state == BLINK_OFF)
        {
            menu_display_string[0] = 'z';
            menu_display_string[1] = 'z';
            menu_display_string[2] = 'z';
            menu_display_string[3] = 'z';
            menu_display_string[4] = 'z';
        }
        else
        {
            if (console.setting_menu.dealMode == ROTATE_DEAL)
            {
                menu_display_string[0] = 'F';
                menu_display_string[1] = '-';
                menu_display_string[2] = 'F';
                menu_display_string[3] = '-';
                menu_display_string[4] = 'F';
            }
            else
            {
                menu_display_string[0] = '\0';
                menu_display_string[1] = 'F';
                menu_display_string[2] = 'F';
                menu_display_string[3] = 'F';
                menu_display_string[4] = '\0';
            }
        }
        menu_display_dot[0] = 0;
        menu_display_dot[1] = 0;
        menu_display_dot[2] = 0;
        menu_display_dot[3] = 0;
        menu_display_dot[4] = 0;
        memcpy(&displayInfo.string_content, &menu_display_string, sizeof(menu_display_string));
        memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
        displayInfo.start_pos = 0;
        displayInfo.length = 5;
        break;
    case DEALING_ORDER_SETTING: // 发牌顺序变更
        if (delta != 0)
        {
            if (console.setting_menu.dealOrder == BOTTOM_LAST_DEAL)
            {
                console.setting_menu.dealOrder = BOTTOM_FIRST_DEAL;
            }
            else
            {
                console.setting_menu.dealOrder = BOTTOM_LAST_DEAL;
            }
        }
        displayInfo.content_type = STRING_CONTENT;
        if (displayInfo.blink_state == BLINK_OFF)
        {
            menu_display_string[0] = 'z';
            menu_display_string[1] = 'z';
            menu_display_string[2] = 'z';
            menu_display_string[3] = 'z';
            menu_display_string[4] = 'z';
        }
        else
        {
            if (console.setting_menu.dealOrder == BOTTOM_LAST_DEAL)
            {
                menu_display_string[0] = 'F';
                menu_display_string[1] = 'F';
                menu_display_string[2] = 'F';
                menu_display_string[3] = 'F';
                menu_display_string[4] = 'd';
            }
            else
            {
                menu_display_string[0] = 'd';
                menu_display_string[1] = 'F';
                menu_display_string[2] = 'F';
                menu_display_string[3] = 'F';
                menu_display_string[4] = 'F';
            }
        }
        menu_display_dot[0] = 0;
        menu_display_dot[1] = 0;
        menu_display_dot[2] = 0;
        menu_display_dot[3] = 0;
        menu_display_dot[4] = 0;
        memcpy(&displayInfo.string_content, &menu_display_string, sizeof(menu_display_string));
        memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
        displayInfo.start_pos = 0;
        displayInfo.length = 5;
        break;

    case DIRECTION_ROTATE_SETTING:
        if (delta != 0)
        {
            if (console.setting_menu.dirRotate == CLOCKWISE)
            {
                console.setting_menu.dirRotate = COUNTER_CLOCKWISE;
            }
            else
            {
                console.setting_menu.dirRotate = CLOCKWISE;
            }
        }
        if (displayInfo.marQuee_period == 0)
        {
            if (console.setting_menu.dirRotate == CLOCKWISE)
            {
                displayInfo.marQuee_index++;
            }
            else
            {
                displayInfo.marQuee_index--;
            }
            displayInfo.marQuee_period = MARQUEE_PERIOD;
        }

        limitValue(&displayInfo.marQuee_index, 0, 13);
        displayInfo.content_type = MARQUEE_CONTENT;
        break;

    default:
        break;
    }
}

/**
 * @brief 监控
 */
void monitorHook(uint8_t monitor)
{
    
}


/**
 * @brief 模式切换
 * @param console 控制结构体
 * @param target_mode 目标模式
 */
static void ModeSwitch(Console_t *console, CtrlMode_e target_mode)
{
    console->last_mode = console->ctrl_mode;
    console->ctrl_mode = target_mode;

    if ((target_mode == SETTING_MODE || target_mode == SETPLAYER_LAUNCH_MODE) && console->last_mode != PAUSE_MODE)
    {
        // 进入设置前保存当前当前状态
        memcpy(&console->setting_menu, &console->main_menu, sizeof(MenuItem_t));
    }
    if (target_mode == SETTING_MODE || target_mode == SETPLAYER_LAUNCH_MODE)
    {
        // 打开闪烁
        displayInfo.blink_en = ENBLINK;
    }
    else
    {
        // 关闭闪烁
        displayInfo.blink_en = UNBLINK;
    }
    setBuzzer(); // 蜂鸣器工作(模式切换时bee)
    LOG_INFO("Machine switch mode: %d\t->\t%d.\n", console->last_mode, target_mode);
}

/**
 * @brief 更新显示数字内容
 * @param menu_display_num 待写入数组
 * @param menuItem 菜单项(包含数据)
 *
 */
static void updateMenuDisplayNum(uint8_t menu_display_num[5], const MenuItem_t *menuItem)
{
    // 更新前两位为底牌数量
    menu_display_num[0] = menuItem->deckCount / 10;
    menu_display_num[1] = menuItem->deckCount % 10;

    // 更新中间一位为玩家数量
    menu_display_num[2] = menuItem->playerCount;

    // 更新后两位为发牌数量
    menu_display_num[3] = menuItem->cardCount / 10;
    menu_display_num[4] = menuItem->cardCount % 10;
}

/**
 * @brief 蜂鸣器工作保存设置
 */
static void buzzerWork(void)
{
    if (console.main_menu.buzzer_time > 0)
    {
        HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief bee一下
 */
static void setBuzzer(void)
{
#ifdef BUZZER_ENABLE
    console.main_menu.buzzer_time = BUZZER_TIME;
#endif
}

/**
 * @brief 低功耗模式恢复
 */
void recoverLowPowerMode(void)
{
    NVIC_SystemReset();
}

/**
 * @brief 把console_t 保存到flash中的指定区域
 * @param consoleSettings 待保存的console_t结构体变量
 */
static HAL_StatusTypeDef SaveConsoleSettings(Console_t *consoleSettings)
{
    return FlashWrite(FLASH_USER_START_ADDR, (uint32_t *)consoleSettings, sizeof(Console_t) / sizeof(uint32_t));
}

/**
 * @brief 从flash中读取console_t结构体
 * @param consoleSettings 读取到的console_t结构体变量
 */
static void LoadConsoleSettings(Console_t *consoleSettings)
{
    FlashRead(FLASH_USER_START_ADDR, (uint32_t *)consoleSettings, sizeof(Console_t) / sizeof(uint32_t));
}

/**
 * @brief 发牌
 * @param cards 发牌数量
 */
void launchCard(uint16_t cards)
{
    GPIO_PinState last_opto_launch_key, opto_launch_key = RELEASED;

    while (cards > 0 && console.main_menu.launchMode != NO_LAUNCH)
    {
        console.main_menu.cardCount++;
        outMotorForward(&motor[OUTMOTOR]);
        opto_launch_key = HAL_GPIO_ReadPin(outputOptoKey_GPIO_Port, outputOptoKey_Pin);
        if (last_opto_launch_key != RELEASED && opto_launch_key == RELEASED)
        {
            cards--;
            motor[OUTMOTOR].cards++;
            LOG_DEBUG("send one card, output cards: %d\n", motor[OUTMOTOR].cards);
        }
        last_opto_launch_key = opto_launch_key;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief 旋转
 * 旋转到指定位置
 */
void RotatePos(uint16_t pos)
{
    GPIO_PinState last_opto_rotate_key, opto_rotate_key = RELEASED;

    while (pos > 0 && console.main_menu.launchMode != NO_LAUNCH)
    {
        switch (console.main_menu.dirRotate)
        {
        case CLOCKWISE:
            rotateMotorForward(&motor[ROTATEMOTOR]);
            break;

        case COUNTER_CLOCKWISE:
            rotateMotorBackward(&motor[ROTATEMOTOR]);
            break;

        default:
            LOG_WARN("Unknown value in the direction of rotation.");
            break;
        }
        opto_rotate_key = HAL_GPIO_ReadPin(rotateOptoKey_GPIO_Port, rotateOptoKey_Pin);
        if (last_opto_rotate_key != RELEASED && opto_rotate_key == RELEASED)
        {
            pos--;
            LOG_DEBUG("rotate pos: %d\n", pos);
        }
        LOG_DEBUG("last pos: %d\t, pos: %d\n", last_opto_rotate_key, opto_rotate_key);
        last_opto_rotate_key = opto_rotate_key;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief 发牌控制模式切换
 * 执行发牌操作
 */
void WorkModeSwitch(void)
{
    KeyState_e last_opto_launch_key = KEY_IDLE;
    KeyState_e last_opto_rotate_key = KEY_IDLE;
    uint16_t cards = 0;

    switch (console.main_menu.launchMode)
    {
    case NO_LAUNCH:
        outMotorStop(&motor[OUTMOTOR]);
        rotateMotorStop(&motor[ROTATEMOTOR]);
        break;

    case NORMAL_LAUNCH:
        console.main_menu.dirRotate = CLOCKWISE; // 顺时针旋转
        RotatePos(1);
        break;

    case RANDOM_LAUNCH:
        break;

    case TEST_LAUNCH:
        RotatePos(3);
        console.main_menu.launchMode = NO_LAUNCH;
        break;
    default:
        break;
    }
}

/**
 * @brief 数据大小限制
 * @param value 待处理数据 uint8_t
 * @param min 最小边界
 * @param max 最大边界
 */
static void limitValue(uint8_t *value, uint8_t min, uint8_t max)
{
    // 确保max大于min，防止逻辑错误
    if (max < min)
    {
        uint8_t temp = min;
        min = max;
        max = temp;
    }
    if (*value > 254)
    {
        *value = max;
    }
    else if (*value > max)
    {
        *value = min;
    }
    else
    {
        *value = *value;
    }
}

/**
 * @brief 更新电压
 *      用EWMA平滑ad值实现滤波
 */
static void volValueUpdate(void)
{
    const float alpha = 0.04; // EMMA 平滑系数

    static float bat_filtered = 0;
    static float out_motor_filtered = 0;
    static float rotate_motor_filtered = 0;
    static bool initialized = false;

    uint32_t bat_ad = 0;
    uint32_t out_motor_ad = 0;
    uint32_t rotate_motor_ad = 0;

    for (uint8_t i = 0; i < 10; i++)
    {
        bat_ad += console.adBuff_value[3 * i];
        out_motor_ad += console.adBuff_value[3 * i + 1];
        rotate_motor_ad += console.adBuff_value[3 * i + 2];

        console.bat_value[i] = console.adBuff_value[3 * i];
        console.outMotor_value[i] = console.adBuff_value[3 * i + 1];
        console.rotateMotor_value[i] = console.adBuff_value[3 * i + 2];
    }

    uint16_t bat_val = bat_ad / 10;
    uint16_t out_motor_val = out_motor_ad / 10;
    uint16_t rotate_motor_val = rotate_motor_ad / 10;

    // 初始化滤波器值为第一次采样值
    if (!initialized)
    {
        bat_filtered = bat_val;
        out_motor_filtered = out_motor_val;
        rotate_motor_filtered = rotate_motor_val;
        initialized = true;
    }
    else
    {
        // EWMA 套公式
        bat_filtered = alpha * bat_val + (1 - alpha) * bat_filtered;
        out_motor_filtered = alpha * out_motor_val + (1 - alpha) * out_motor_filtered;
        rotate_motor_filtered = alpha * rotate_motor_val + (1 - alpha) * rotate_motor_filtered;
    }

    LOG_DEBUG("bat value: %d\n", (uint16_t)bat_filtered);
    LOG_DEBUG("out motor value: %d\n", (uint16_t)out_motor_filtered);
    LOG_DEBUG("rotate motor value: %d\n", (uint16_t)rotate_motor_filtered);
}