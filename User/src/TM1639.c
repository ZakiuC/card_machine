#include "TM1639.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "log.h"

#define NUM_TM1639KEYS 5                   
#define KEY_LONG_PRESS_THRESHOLD 2000 // 长按时长
#define KEY_CLICK_THRESHOLD 500       // 单击时长

// 操作单个位命令
#define bitSet(value, bit) ((value) |= (1UL << (bit)))											// 设置位
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))										// 清除位
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))										// 切换位状态
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)											// 读取位状态
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit)) // 根据bitvalue写入位状态

// IO操作
static inline void set_data_pin(GPIO_PinState state)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, state);
}
static inline void set_clk_pin(GPIO_PinState state)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, state);
}
static inline void set_strobe_pin(GPIO_PinState state)
{	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, state);
}
static inline GPIO_PinState get_data_pin()
{
	return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
}

// 按键掩码，用于标识TM1639模块上不同按键的位置
static uint16_t key_mask[5] = {12, 11, 16, 15, 4};
uint8_t data[2] = {0};
static uint16_t TM1639Key_Value = 0;
static uint8_t start_hooking = 0;

static TM1639key_t tm1639_keys[NUM_TM1639KEYS] = {
    {TM1639KEY_RANDOM, 1, 1, 0, TM1639KEY_IDLE, TM1639KEY_IDLE},
    {TM1639KEY_ADD, 1, 1, 0, TM1639KEY_IDLE, TM1639KEY_IDLE},
	{TM1639KEY_SUB, 1, 1, 0, TM1639KEY_IDLE, TM1639KEY_IDLE},
	{TM1639KEY_SETTING, 1, 1, 0, TM1639KEY_IDLE, TM1639KEY_IDLE},
	{TM1639KEY_LAUNCH, 1, 1, 0, TM1639KEY_IDLE, TM1639KEY_IDLE},
};



static uint16_t TM1639ReadKey(void);



/**
 * @brief 设置数据引脚模式
 *
 * @param mode GPIO_PIN_SET: 输出模式; GPIO_PIN_RESET: 输入模式
 */
static void set_data_pin_mode(GPIO_PinState mode)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = mode == GPIO_PIN_SET ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
 
/**	
 * @brief TM1639延时函数
 * 使用hal库的延时函数代替nop延时
 * @param count 延时时间(count * 78.125 纳秒)
 */
void TM1639_Delay(uint8_t count)
{
	for(uint8_t i=0;i < count; i++)
	{
		__NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
	}
}

/**
 * @brief 写一字节数据
 *
 * @param data 数据
 */
static void TM1639WriteByte(uint8_t data)
{
	start_hooking = 0;
	// 逐位发送数据，从最低位开始
	for (uint8_t i = 0; i < 8; i++)
	{
		set_clk_pin(0);			// 时钟线拉低，准备数据发送
		set_data_pin(data & 1); // 设置数据引脚
		data >>= 1;				// 移位准备发送下一位
		set_clk_pin(1);			// 时钟线拉高，完成这一位数据的发送
		TM1639_Delay(1);
	}
	start_hooking = 1;
}


/**
 * @brief 发送数据
 *
 * @param address 地址
 * @param data 数据
 */
static void TM1639WriteData(uint8_t address, uint8_t data)
{
	start_hooking = 0;
	set_strobe_pin(0);
	TM1639WriteByte(0x00 | address);
	TM1639WriteByte(data);
	set_strobe_pin(1);
	start_hooking = 1;
}

/**
 * @brief 发送命令
 * B7 B6 指令
 *	0  1 数据命令设置
 *	1  0 显示控制命令设置
 *	1  1 地址命令设置
 * 数据命令设置 (B7=0, B6=1):
 * B5,B4   	- 无关项，填0
 * B3   	- 测试模式设置 (0: 普通模式, 1: 测试模式，内部使用)
 * B2   	- 地址增加模式设置 (0: 自动地址增加, 1: 固定地址)
 * B1   	- 数据读写模式设置 (0: 写数据到显示寄存器, 1: 读键扫数据)
 * B0 		- 无关项，填0
 * 显示控制命令设置 (B7=1, B6=0):
 * B5,B4   	- 无关项，填0
 * B3		- 显示开关设置 (0: 关闭显示, 1: 打开显示)
 * B2,B1,B0	- 亮度设置 (000: 1/16, 001: 2/16, 010: 4/16, 011: 10/16, 100: 11/16, 101: 12/16, 110: 13/16, 111: 14/16)
 * 地址命令设置 (B7=1, B6=1):
 * B5,B4   	- 无关项，填0
 * B3,B2,B1,B0	- 地址设置 (0000: C0H, 0001: C1H, 0010: C2H, 0011: C3H, 0100: C4H, 0101: C5H, 0110: C6H, 0111: C7H,
 * 						   1000: C8H, 1001: C9H, 1010: CAH, 1011: CBH, 1100: CCH, 1101: CDH, 1110: CEH, 1111: CFH)
 * @param cmd 命令
 */
static void TM1639WriteCmd(uint8_t cmd)
{
	set_strobe_pin(0);
	TM1639WriteByte(cmd);
	set_strobe_pin(1);
}

/**
 * @brief 清除显示
 *
 * @param void
 */
void TM1639Clear(void)
{
	set_strobe_pin(0);
	TM1639WriteCmd(0xC0); // 设置数据地址
	for (int i = 0; i < 16; i++)
	{
		TM1639WriteByte(0x00); // 清除所有LED
	}
	set_strobe_pin(1);
}

/**
 * @brief 获取数字编码
 *
 * @param num 数字
 * @return uint8_t 数字编码
 */
static uint8_t getDigitCode(uint8_t num)
{
	const uint8_t digitCode[] = {
		0x3F, // 0
		0x06, // 1
		0x5B, // 2
		0x4F, // 3
		0x66, // 4
		0x6D, // 5
		0x7D, // 6
		0x07, // 7
		0x7F, // 8
		0x6F, // 9
		0x00, // 10（不显示）
	};
	return digitCode[num];
}

/**
 * @brief 获取字母编码
 *
 * @param letter 字母
 * @return uint8_t 数字编码
 */
static uint8_t getLetterCode(char letter)
{
	uint8_t index = 0;
    // 大写字母编码
    const uint8_t letterToSegmentUpperCase[26] = {
        0x77, // A
        0x7F, // B
        0x27, // C
        0x3F, // D
        0x79, // E
        0x71, // F
        0x5E, // G
        0x76, // H
        0x06, // I
        0x1E, // J
        0x80, // K无法准确显示
		0x38, // L
		0x80, // M无法准确显示
		0x80, // N无法准确显示
		0x3F, // O
		0x73, // P
		0x67, // Q
		0x33, // R, 仿照但不完美
		0x6D, // S
		0x31, // T, 仿照但不完美
		0x3E, // U
		0x80, // V无法准确显示
		0x80, // W无法准确显示
		0x80, // X无法准确显示
		0x6E, // Y
		0x00  // Z 不显示
	};

	// 小写字母编码（示例，可以根据实际情况修改）
    const uint8_t letterToSegmentLowerCase[26] = {
        0x77, // a
        0x7F, // b
        0x27, // c
        0x5E, // d
        0x79, // e
        0x47, // f
        0x5E, // g
        0x76, // h
        0x06, // i
        0x1E, // j
        0x80, // k无法准确显示
		0x38, // l
		0x80, // m无法准确显示
		0x80, // n无法准确显示
		0x3F, // o
		0x73, // p
		0x67, // q
		0x33, // r, 仿照但不完美
		0x6D, // s
		0x31, // t, 仿照但不完美
		0x3E, // u
		0x80, // v无法准确显示
		0x80, // w无法准确显示
		0x80, // x无法准确显示
		0x6E, // y
		0x00  // z不显示
    };

	if (letter >= 'A' && letter <= 'Z') {
        index = letter - 'A';
        return letterToSegmentUpperCase[index];
    }
    else if (letter >= 'a' && letter <= 'z') {
        index = letter - 'a';
        return letterToSegmentLowerCase[index];
    }
    else if (letter == '-') {
        return 0x40; // “-”的编码
    }
    else {
        // 不是字母
        return 0;
    }
}


/**
 * @brief 跑马灯显示
 * @param index_num 索引 0-13
 * 
 * top: 0x01,bottm: 0x08,left-top: 0x20,left-bottom: 0x10,right-top: 0x02,right-bottom: 0x04
*/
void MarqueeDisplay(uint8_t index_num) 
{
	const uint8_t pos_base = 0xC0;	// 基址

	uint8_t origin[5] = {0, 1, 2, 4, 3};	
	uint8_t index[14][4] = {{0, 1, 2, 4}, {1, 2, 4, 3}, {2, 4, 3, 3}, {4, 3, 3, 3}, {3, 3, 3, 3}, {3, 3, 3, 4}, {3, 3, 4, 2}, 
							{3, 4, 2, 1}, {4, 2, 1, 0}, {2, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 2}};
	uint8_t pattern[14][4] = {{0x01, 0x01, 0x01, 0x01}, {0x01, 0x01, 0x01, 0x01}, {0x01, 0x01, 0x03, 0x03}, {0x01, 0x07, 0x07, 0x07},
							{0x0F, 0x0F, 0x0F, 0x0F}, {0x0E, 0x0E, 0x0E, 0x08}, {0x0C, 0x0C, 0x08, 0x08}, {0x08, 0x08, 0x08, 0x08},
							{0x08, 0x08, 0x08, 0x08}, {0x08, 0x08, 0x18, 0x18}, {0x08, 0x38, 0x38, 0x38}, {0x39, 0x39, 0x39, 0x39},	
							{0x31, 0x31, 0x31, 0x01}, {0x21, 0x21, 0x01, 0x01}};
	for (uint8_t i = 0; i < 5; i++)
	{	
		for(uint8_t j = 0; j < 4;j++)
		{
			if(origin[i] == index[index_num][j])
			{	
				break;
			}
			if(j == 3)
			{
				TM1639WriteData(pos_base + origin[i] * 2, 0x00 & 0x0F);
        		TM1639WriteData(pos_base + origin[i] * 2 + 1, 0x00 >> 4);
			}
		}
	}
	
    for (uint8_t i = 0; i < 4; i++) 	
	{
        TM1639WriteData(pos_base + index[index_num][i] * 2, pattern[index_num][i] & 0x0F);
        TM1639WriteData(pos_base + index[index_num][i] * 2 + 1, pattern[index_num][i] >> 4);
    }
    TM1639WriteCmd(0x40); // 写数据到显示寄存器,自动地址增加
}

/**
 * @brief 更新TM1639显示数字内容
 *
 * @param nums 数字数组
 * @param dots 小数点数组
 * @param start_pos 刷新起始位置
 * @param length 数组长度
 */
void TM1639NumShow(uint8_t nums[], uint8_t dots[], uint8_t start_pos, uint8_t length)
{
	const uint8_t pos_base = 0xC0; // 命令字节的基地址
	uint8_t index = start_pos;

	// 填充数字编码并发送	
	for (uint8_t i = start_pos; i < (start_pos + length); i++)
	{
		index = i - start_pos;
		// 获取数字编码并设置小数点
		uint8_t data = getDigitCode(nums[index]) | (dots[index] ? 0x80 : 0x00);
		// MARK: 3和4的位置调整以符合硬件位置
		index = (i == 3) ? 4 : (i == 4) ? 3 : i;
		// 发送数字编码到TM1639	
		TM1639WriteData(pos_base + index * 2, data & 0x0F);   // 发送低4位
		TM1639WriteData(pos_base + index * 2 + 1, data >> 4); // 发送高4位
	}

	// 更新显示命令，以应用显示内容和亮度设置
	TM1639WriteCmd(0x40); // 写数据到显示寄存器,自动地址增加
	TM1639WriteCmd(0x8A); // 显示控制,设置脉冲宽度为4/16
}

/**
 * @brief 更新TM1639显示字母内容
 *
 * @param texts 字母数组
 * @param textLength 字母数组长度
 * @param dots 小数点数组
 */
void TM1639LetterShow(char texts[], uint8_t length, uint8_t dots[])
{	
	const uint8_t pos_base = 0xC0; // 命令字节的基地址
	uint8_t index = 0;

	// 填充字母编码并发送
	for (uint8_t i = 0; i < length; i++)
	{
		// MARK: 3和4的位置调整以符合硬件位置
		index = (i == 3) ? 4 : (i == 4) ? 3 : i;

		// 获取数字编码并设置小数点
		uint8_t data = getLetterCode(texts[index]) | (dots[index] ? 0x80 : 0x00); 

		// 发送数字编码到TM1639
		TM1639WriteData(pos_base + i * 2, data & 0x0F);   // 发送低4位
		TM1639WriteData(pos_base + i * 2 + 1, data >> 4); // 发送高4位
	}

	// 更新显示命令，以应用显示内容和亮度设置
	TM1639WriteCmd(0x40); // 写数据到显示寄存器,自动地址增加
	TM1639WriteCmd(0x8A); // 显示控制,设置脉冲宽度为4/16
}



/**
 * @brief 更新TM1639显示字母加符号内容
 *
 * @param texts 字母数组
 * @param textLength 字母数组长度
 * @param nums 数字数组
 * @param numsLength 数字数组长度
 * @param dots 小数点数组
 */
void TM1639RemixShow(char texts[], uint8_t textLength, uint8_t nums[], uint8_t numsLength, uint8_t dots[])
{
	const uint8_t pos_base = 0xC0; // 命令字节的基地址
	uint8_t index = 0;

	// 填充字母编码并发送
	for (uint8_t i = 0; i < textLength; i++)
	{
		// MARK: 3和4的位置调整以符合硬件位置
		index = (i == 3) ? 4 : (i == 4) ? 3 : i;

		// 获取数字编码并设置小数点
		uint8_t data = getLetterCode(texts[index]) | (dots[index] ? 0x80 : 0x00);

		// 发送数字编码到TM1639
		TM1639WriteData(pos_base + index * 2, data & 0x0F);   // 发送低4位
		TM1639WriteData(pos_base + index * 2 + 1, data >> 4); // 发送高4位
	}

	// 填充数字编码并发送
	for (uint8_t i = textLength; i < textLength + numsLength; i++)
	{
		// MARK: 3和4的位置调整以符合硬件位置
		index = i - textLength;
		// 获取数字编码并设置小数点
		uint8_t data = getDigitCode(nums[index]) | (dots[index] ? 0x80 : 0x00);
		index += textLength;
		index = (index == 3) ? 4 : (index == 4) ? 3 : index;
		// 发送数字编码到TM1639
		TM1639WriteData(pos_base + index * 2, data & 0x0F);   // 发送低4位
		TM1639WriteData(pos_base + index * 2 + 1, data >> 4); // 发送高4位
	}

	// 更新显示命令，以应用显示内容和亮度设置
	TM1639WriteCmd(0x40); // 写数据到显示寄存器,自动地址增加
	TM1639WriteCmd(0x8A); // 显示控制,设置脉冲宽度为4/16
}


/**
 * @brief 更改显示器的亮度
 *
 * @param brightness 亮度等级 1-8, 对应1/16 - 14/16
 */
void TM1639SetBrightness(uint8_t brightness)
{
	brightness = (brightness == 0) ? 1 : (brightness > 8) ? 8
														  : brightness;
	TM1639WriteCmd(0x88 + brightness - 1); // 显示亮度
}

/**
 * @brief 控制显示器的开关
 *
 * @param state TM1639_ON: 开启显示器 TM1639_OFF: 关闭显示器
 */
void TM1639SetDisplayState(TM1639_Switch_e state)
{
	TM1639WriteCmd(state ? 0x8B : 0x80); // 启用/关闭显示
}

/**
 * @brief 控制显示器电源开关	
 *
 * @param PinState TM1639_ON: 开启显示器电源 TM1639_OFF: 关闭显示器
 */
void TM1639PowerCtrl(TM1639_Switch_e PinState)
{
	HAL_GPIO_WritePin(displayPower_GPIO_Port, displayPower_Pin, !(GPIO_PinState)PinState);
}

/**	
 * @brief 读取TM1639按键值
 *
 * @return uint16_t 按键值
 */
static uint16_t TM1639ReadKey(void)
{
	uint16_t key_value = 0; // 存储解析后的按键值
	uint16_t data_16b = 0;
	uint8_t data_8bl = 0; // 从TM1639读取的原始数据
	uint8_t data_8bh = 0;

	// 设置TM1639为读取按键模式
	set_strobe_pin(0);
	TM1639WriteByte(0x42); // 发送读键扫描模式命令
	set_data_pin_mode(GPIO_PIN_RESET);

	for (int i = 0; i < 16; i++)
	{ // 读二个字节
		set_clk_pin(0);
		TM1639_Delay(2);
		if (i < 8)
		{
			bitWrite(data_8bh, i, get_data_pin()); // 读第一个字节
		}
		else
		{
			bitWrite(data_8bl, (i - 8), get_data_pin()); // 读第二个字节
		}
		set_clk_pin(1);
		TM1639_Delay(2);
	}
	set_data_pin_mode(GPIO_PIN_SET);
	set_strobe_pin(1);
	data[0] = data_8bh;
	data[1] = data_8bl;
	data_16b = data_8bh;
	data_16b = data_16b << 8 | data_8bl; // 合并两个字节
	// 根据key_mask解析按键值
	for (uint8_t i = 0; i < (sizeof(key_mask) / sizeof(key_mask[0])); i++)
	{
		if ((data_16b >> (key_mask[i] - 1)) & 0x0001)
		{						   // 检查对应位是否为1
			key_value |= (1 << i); // 设置对应按键的位
		}
	}
	return key_value; // 返回解析后的按键值		
}


/**
 * @brief 解析指定按键的状态
 *
 * @param key_value 按键值
 * @param key_number 按键编号
 * @return TM1639_KeyState_e 返回按键状态，KEY_DOWN表示按下，KEY_UP表示未按下
 */
TM1639_KeyState_e parse_key_status(uint16_t key_value, uint8_t key_number)
{
	// 检查指定按键是否被按下
	if (((1 << (key_number - 1)) & key_value) == 0)
	{
		return TM1639_KEY_DOWN;
	}
	else
	{
		return TM1639_KEY_UP;
	}
}

/**
 * @brief 初始化TM1639
 */               
void TM1639Init(void)
{
	set_data_pin_mode(GPIO_PIN_SET);
	set_strobe_pin(1);
	set_clk_pin(1);
	TM1639_Delay(2);	

	TM1639WriteCmd(0x40); // 0100 0000	写数据到显示寄存器,自动地址增加
	TM1639WriteCmd(0x87); // 1000 0111	显示控制,设置脉冲宽度为14/16

	TM1639Clear();
	start_hooking = 1;
}

/**
 * @brief 获取指定按键的状态
 * @param key_id 按键编号
 * @return TM1639_KeyState_e 返回按键状态，KEY_DOWN表示按下，KEY_UP表示未按下
*/
static uint8_t Key_GetPortState(TM1639KeyId_e key_id)
{
	return parse_key_status(TM1639Key_Value, key_id + 1);
}


/**
 * @brief TM1639按键扫描
*/
void TM1639KeyScan(void)
{
    for (uint8_t i = 0; i < NUM_TM1639KEYS; i++)
    {
		tm1639_keys[i].last_state = tm1639_keys[i].state;
        tm1639_keys[i].last = tm1639_keys[i].current;

        tm1639_keys[i].current = Key_GetPortState(i);

        if (tm1639_keys[i].state != TM1639KEY_LONG_PRESSED && tm1639_keys[i].state != TM1639KEY_LONG_PRESSED_BACK)
        {
            if (tm1639_keys[i].current == 0)
            {
                if (tm1639_keys[i].last == 1)
                {
                    tm1639_keys[i].state = TM1639KEY_PRESSED;
                    LOG_DEBUG("TM1639 Key %d pressed.\n", i);
                    tm1639_keys[i].press_time = 0;
                }
                else
                {
                    if (tm1639_keys[i].press_time > KEY_LONG_PRESS_THRESHOLD)
                    {
                        tm1639_keys[i].state = TM1639KEY_LONG_PRESSED;
                        LOG_DEBUG("TM1639 Key %d long pressed.\n", i);
                    }
                }
            }
            else
            {
                if (tm1639_keys[i].last == 0)
                {
                    tm1639_keys[i].state = TM1639KEY_RELEASED;
                    if (tm1639_keys[i].press_time < KEY_CLICK_THRESHOLD)
                    {
                        tm1639_keys[i].state = TM1639KEY_CLICKED;
                        LOG_DEBUG("TM1639 Key %d clicked.\n", i);
                    }
                    else
                    {
                        LOG_DEBUG("TM1639 Key %d released.\n", i);
                    }
                }
                else
                {
                    tm1639_keys[i].press_time = 0;
                    tm1639_keys[i].state = TM1639KEY_IDLE;
                }
            }
        }
        else if(tm1639_keys[i].state == TM1639KEY_LONG_PRESSED)
        {
			if(tm1639_keys[i].last_state == TM1639KEY_LONG_PRESSED && tm1639_keys[i].state == TM1639KEY_LONG_PRESSED)
			{
				tm1639_keys[i].press_time = 0;
				tm1639_keys[i].state = TM1639KEY_LONG_PRESSED_BACK;
			}
        }
		else{
			if (tm1639_keys[i].current == 1)
            {
                tm1639_keys[i].state = TM1639KEY_RELEASED;
                LOG_DEBUG("TM1639 Key %d released.\n", i);
            }
		}
    }
}


/**
 * @brief TM1639按键处理
*/
void TM1639MsHandle(void)
{
	static uint8_t check_key_delay = 0;
	if (check_key_delay > 2)
	{
		TM1639Key_Value = TM1639ReadKey();
		check_key_delay = 0;
	}
	
    for (uint8_t i = 0; i < NUM_TM1639KEYS; i++)
    {
        if (tm1639_keys[i].state == TM1639KEY_PRESSED)
        {
            tm1639_keys[i].press_time++;
        }
    }
	check_key_delay += start_hooking;
}

/**
 * @brief 获取TM1639按键信息
 * @return TM1639key_t* 返回TM1639按键信息
*/
TM1639key_t *GetTM1639KeyInfo(void)
{
    return tm1639_keys;
}


/**
 * @brief TM1639测试函数，测试使用
 */
void TM1639_Test(void)
{
	
}
