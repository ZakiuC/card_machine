#include "console.h"
#include "bsp_key.h"
#include "TM1639.h"
#include <string.h>
#include "log.h"

/* 全局变量 ------------------------------------------------------------------*/
// 显示信息
DisplayInfo_t displayInfo = {
    .needUpdate = false,
    .content_type = DIGITAL_CONTENT,
    .digital_content = {0},
    .string_content = {0},
    .dot_content = {0},
    .start_pos = 0,
    .start_pos2 = 0,
    .length = 0,
};

/* 私有变量 ------------------------------------------------------------------*/
// 历史显示数据 - 用于确认是否需要刷新显示内容，后续可用以优化显示刷新
DisplayInfo_t last_displayInfo = {
    .needUpdate = false,
    .content_type = DIGITAL_CONTENT,
    .digital_content = {0},
    .string_content = {0},
    .dot_content = {0},
    .start_pos = 0,
    .start_pos2 = 0,
    .length = 0,
};

// 控制结构体
Console_t  console = {
    .main_menu = {
        .setting = NO_SETTING,
        .dealMode = SWAY_DEAL,
        .dealOrder = BOTTOM_FIRST_DEAL,
        .deckCount = 3,
        .playerCount = 3,
        .cardCount = 17,
        .burstCount = 17,
    }
};

// 按键数据
const Key_inf_t *key_info = NULL;
const TM1639Key_inf_t *tm1639_key_info = NULL;

/* 函数声明 ------------------------------------------------------------------*/
static void PrepareMenu_handle(void);
static void IdleMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e setting_key);
static void SettingMenu_handle(TM1639KeyState_e launch_key, TM1639KeyState_e random_key, TM1639KeyState_e setting_key,
                             TM1639KeyState_e add_key, TM1639KeyState_e sub_key);
static void PauseMenu_handle(TM1639KeyState_e launch_key, Key_State_e power_key);

static void SettingSwitch(SettingItem_e item, int8_t delta);
static void ModeSwitch(Console_t *console, CtrlMode_e target_mode);
static void updateMenuDisplayNum(uint8_t menu_display_num[5], const MenuItem_t* menuItem);
static void limitValue(uint8_t *value, uint8_t min, uint8_t max);
static void check_displayUpdate(void);


/* 函数体 --------------------------------------------------------------------*/
/**
 * @brief ms更新
*/
void Console_msHandle(void)
{
    Key_msHandle();
    TM1639_msHandle();
    console.prepare_wait_time += console.prepare_wait_increment;
}

/**
 * @brief 控制台初始化
*/
void Console_Init(void)
{
    
    console.ctrl_mode = PREPARE_MODE;
    console.last_mode = console.ctrl_mode;
    console.setting_mode = NO_SETTING;
    console.prepare_wait_time = 0;
    console.prepare_wait_increment = 1;

    LOG("\n\n///////////////////////\nstart running.\n");
    Key_Init();
    key_info = GetKeyInfo();                                // mark: 等待初始化完再绑定，下同
    LOG_INFO("Key initialization succeeded.\n");
    TM1639_Init();
    tm1639_key_info = GetTM1639KeyInfo();
    LOG_INFO("TM1639 initialization succeeded.\n");
    TM1639_PowerCtrl(TM1639_ON);
    LOG_INFO("TM1639 power on.\n");
    console.ctrl_mode = IDLE_MODE;
    
    console.prepare_wait_increment = 0;
    console.prepare_wait_time = 0;
    LOG_INFO("Machine is in idle mode.\n");
}

/**
 * @brief 控制台模式切换
*/
void Console_ModeSwitch(void)
{
    // 主菜单[响应：1.SW6(长按关机)]
    TM1639KeyState_e random, add, sub, setting, launch; 
    Key_State_e power, touch, opto_launch, opto_rotate;
    random = tm1639_key_info[TM1639KEY_RANDOM].key_state;   // SW1
    add = tm1639_key_info[TM1639KEY_ADD].key_state;         // SW2
    sub = tm1639_key_info[TM1639KEY_SUB].key_state;         // SW3
    setting = tm1639_key_info[TM1639KEY_SETTING].key_state; // SW4
    launch = tm1639_key_info[TM1639KEY_LAUNCH].key_state;   // SW5

    power = key_info[KEY_POWER].key_state;                  // SW6
    touch = key_info[KEY_TOUCH1].key_state;                  // touch
    opto_launch = key_info[KEY_OPTO_LAUNCH].key_state;      // 发牌光电
    opto_rotate = key_info[KEY_OPTO_ROTATE].key_state;      // 旋转光电


    if (touch == TM1639KEY_CLICK && console.ctrl_mode != PAUSE_MODE && console.ctrl_mode != SAFETY_MODE)
    {
        // 单击touch: 进入暂停模式，暂停所有的工作
        ModeSwitch(&console, PAUSE_MODE);
    }
    
    switch (console.ctrl_mode)
    {
    case PREPARE_MODE:
        PrepareMenu_handle();
        break;
    case IDLE_MODE:
        IdleMenu_handle(launch, random, setting);
        break;
    case SETTING_MODE:
        SettingMenu_handle(launch, random, setting, add, sub);
        break;
    case PAUSE_MODE:
        PauseMenu_handle(launch, power);  
        break;
    case SAFETY_MODE:
        LOG_INFO("Machine is in safe mode.\n");
        break;
    default:
        break;
    }
    
    check_displayUpdate();
}

/** 
 * @brief 检查是否需要刷新
*/
static void check_displayUpdate(void)
{
    // 默认认为需要更新
    displayInfo.needUpdate = true;

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
                default:
                    // 对于未知类型，默认不更新（或视为总是更新）
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

    // 更新历史记录，无论是否需要更新
    memcpy(&last_displayInfo.digital_content, &displayInfo.digital_content, sizeof(displayInfo.digital_content));
    memcpy(&last_displayInfo.string_content, &displayInfo.string_content, sizeof(displayInfo.string_content));
    memcpy(&last_displayInfo.dot_content, &displayInfo.dot_content, sizeof(displayInfo.dot_content));
    last_displayInfo.content_type = displayInfo.content_type;
    last_displayInfo.start_pos = displayInfo.start_pos;
    last_displayInfo.start_pos2 = displayInfo.start_pos2;
    last_displayInfo.length = displayInfo.length;
}

/**
 * @brief 准备模式处理
*/
static void PrepareMenu_handle(void)
{
    // 初始化超时 
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
    if (launch_key == TM1639KEY_CLICK)
    {
        // 单击SW5: 当前方向发‘张数’牌
    }else if(launch_key == TM1639KEY_LONG_4S)
    {
        // 长按SW5: 无
    }
    else if (random_key == TM1639KEY_CLICK)
    {
        // 单击SW1: 随机选择位发牌
    }else if(random_key == TM1639KEY_LONG_4S)
    {
        // 长按SW1: 进入选择‘位’，再发牌
    }
    else if (setting_key == TM1639KEY_CLICK)
    {
        LOG_DEBUG("Enter setting mode.\n");
        // 设置项移动到底牌数量
        console.main_menu.setting = BASECARD_COUNT_SETING;
        // 单击SW4: 进数值和模式的设置
        ModeSwitch(&console, SETTING_MODE);
    }else if(setting_key == TM1639KEY_LONG_4S)
    {
        // 长按SW4: 进旋转方向设置
    }else
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
    if (launch_key == TM1639KEY_CLICK || random_key == TM1639KEY_CLICK)
    {
        // 单击SW5/SW1: 保存当前设置，返回主菜单
        memcpy(&console.main_menu, &console.setting_menu, sizeof(console.setting_menu));
        LOG_DEBUG("Back idle mode 1.\n");
        ModeSwitch(&console, IDLE_MODE);
    }
    else if (setting_key == TM1639KEY_CLICK)
    {
        LOG_DEBUG("SW4 click.\n");
        // 单击SW4: 切换下一项设置
        console.main_menu.setting++;
        // 当玩家人数在2/3时才会进入 左右发牌/单旋转发牌模式 的选择
        if (console.main_menu.setting == DEALING_MODE_SETTING)
        {
            if(console.setting_menu.playerCount != 2 && console.setting_menu.playerCount != 3)
            {
                console.main_menu.setting++;
            }
        }
        if(console.main_menu.setting > DEALING_ORDER_SETTING)
        {
            // 保存设置返回主菜单,设置项返回初始位置
            memcpy(&console.main_menu, &console.setting_menu, sizeof(console.setting_menu));
            console.main_menu.setting = NO_SETTING;
            LOG_DEBUG("Back idle mode 2.\n");
            ModeSwitch(&console, IDLE_MODE);
        }else{
            LOG_INFO("setting item next: %d\n", console.main_menu.setting);
        }
    }
    else if(add_key == TM1639KEY_CLICK)
    {
        // 单击SW2: 增加当前选中项的值
        delta = 1;
        LOG_INFO("count %d, checkbox +1\n", console.setting_menu.setting);
    }
    else if(sub_key == TM1639KEY_CLICK)
    {
        // 单击SW3: 减少当前选中项的值
        delta = -1;
        LOG_INFO("count %d, checkbox -1\n", console.setting_menu.setting);
    }
    
    SettingSwitch(console.main_menu.setting, delta);
}


/**
 * @brief 暂停模式处理
 * @param launch_key 发牌键
 * @param random_key 随机键
*/
static void PauseMenu_handle(TM1639KeyState_e launch_key, Key_State_e power_key)
{// click SW5继续工作，click SW6退回主菜单。
    uint8_t menu_display_dot[5] = {0};
    char menu_display_string[5] = {'\0'};
    
    if (launch_key == TM1639KEY_CLICK)
    {
        ModeSwitch(&console, console.last_mode);
    }else if(power_key == KEY_CLICK)
    {
        ModeSwitch(&console, IDLE_MODE);
    }
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
    switch(item)
    {
        case BASECARD_COUNT_SETING: // 底牌数量变更
            console.setting_menu.deckCount += delta;
            limitValue(&console.setting_menu.deckCount, 0, 99);
            displayInfo.content_type = DIGITAL_CONTENT;
            menu_display_num[0] = console.setting_menu.deckCount / 10;
            menu_display_num[1] = console.setting_menu.deckCount % 10;
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
        case PLAYER_COUNT_SETTING:  // 玩家数量变更
            console.setting_menu.playerCount += delta;
            limitValue(&console.setting_menu.playerCount, 0, 8);
            displayInfo.content_type = DIGITAL_CONTENT;
            menu_display_num[0] = console.setting_menu.playerCount % 10;
            menu_display_dot[0] = 1;
            menu_display_dot[1] = 0;
            menu_display_dot[2] = 0;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 2;
            displayInfo.length = 1;
            break;
        case LAUNCH_COUNT_SETTING:  // 发牌数量变更
            console.setting_menu.cardCount += delta;
            limitValue(&console.setting_menu.cardCount, 0, 99);
            displayInfo.content_type = DIGITAL_CONTENT;
            menu_display_num[0] = console.setting_menu.cardCount / 10;
            menu_display_num[1] = console.setting_menu.cardCount % 10;
            menu_display_dot[0] = 0;
            menu_display_dot[1] = 0;
            menu_display_dot[2] = 0;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 3;
            displayInfo.length = 2;
            break;
        case BURST_COUNT_SETTING:   // 连发数量变更
            console.setting_menu.burstCount += delta;
            limitValue(&console.setting_menu.burstCount, 0, 99);
            displayInfo.content_type = STRING_DIGITAL_CONTENT;
            menu_display_num[0] = console.setting_menu.burstCount / 10;
            menu_display_num[1] = console.setting_menu.burstCount % 10;
            menu_display_string[0] = 'L';
            menu_display_string[1] = 'F';
            menu_display_string[2] = '-';
            menu_display_dot[0] = 0;
            menu_display_dot[1] = 0;
            menu_display_dot[2] = 0;
            menu_display_dot[3] = 0;
            menu_display_dot[4] = 0;
            memcpy(&displayInfo.string_content, &menu_display_string, sizeof(menu_display_string));
            memcpy(&displayInfo.digital_content, &menu_display_num, sizeof(menu_display_num));
            memcpy(&displayInfo.dot_content, &menu_display_dot, sizeof(menu_display_dot));
            displayInfo.start_pos = 0;
            displayInfo.start_pos2 = 3;
            displayInfo.length = 5;
            break;
        case DEALING_MODE_SETTING:  // 发牌模式变更
            if(delta != 0)
            {
                if(console.setting_menu.dealMode == ROTATE_DEAL)
                {
                    console.setting_menu.dealMode = SWAY_DEAL;
                }else{
                    console.setting_menu.dealMode = ROTATE_DEAL;
                }
            }
            if(console.setting_menu.dealMode == ROTATE_DEAL)
            {
                menu_display_string[0] = 'F';
                menu_display_string[1] = '-';
                menu_display_string[2] = 'F';
                menu_display_string[3] = '-';
                menu_display_string[4] = 'F';
            }else{
                menu_display_string[0] = '\0';
                menu_display_string[1] = 'F';
                menu_display_string[2] = 'F';
                menu_display_string[3] = 'F';
                menu_display_string[4] = '\0';
            }
            displayInfo.content_type = STRING_CONTENT;
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
        case DEALING_ORDER_SETTING:  // 发牌顺序变更
            if(delta != 0)
            {
                if(console.setting_menu.dealOrder == BOTTOM_LAST_DEAL)
                {
                    console.setting_menu.dealOrder = BOTTOM_FIRST_DEAL;
                }else{
                    console.setting_menu.dealOrder = BOTTOM_LAST_DEAL;
                }
            }
            if(console.setting_menu.dealOrder == BOTTOM_LAST_DEAL)
            {
                menu_display_string[0] = 'F';
                menu_display_string[1] = 'F';
                menu_display_string[2] = 'F';
                menu_display_string[3] = 'F';
                menu_display_string[4] = 'd';
            }else{
                menu_display_string[0] = 'd';
                menu_display_string[1] = 'F';
                menu_display_string[2] = 'F';
                menu_display_string[3] = 'F';
                menu_display_string[4] = 'F';
            }
            displayInfo.content_type = STRING_CONTENT;
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
        default:
        break;
    }
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
    if(target_mode == SETTING_MODE && console->last_mode != PAUSE_MODE)
    {
        // 进入设置前保存当前当前状态
        memcpy(&console->setting_menu, &console->main_menu, sizeof(MenuItem_t));
    }
    LOG_INFO("Machine switch mode: %d\t->\t%d.\n", console->last_mode, target_mode);
}

/**
 * @brief 更新显示数字内容
 * @param menu_display_num 待写入数组
 * @param menuItem 菜单项(包含数据)
 * 
*/
static void updateMenuDisplayNum(uint8_t menu_display_num[5], const MenuItem_t* menuItem) 
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
 * @brief 数据大小限制
 * @param value 待处理数据 uint8_t 
 * @param min 最小边界
 * @param max 最大边界
*/
static void limitValue(uint8_t *value, uint8_t min, uint8_t max)
{
    // 确保max大于min，防止逻辑错误
    if (max < min) {
        uint8_t temp = min;
        min = max;
        max = temp;
    }
    if(*value > 254)
    {
        *value = max;
    }
    else if (*value > max)
    {
        *value = min;
    }else{
        *value = *value;
    }
}