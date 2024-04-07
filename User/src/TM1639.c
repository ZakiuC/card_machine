#include "TM1639.h"
#include "gpio.h"

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
 * @param count 延时时间(ms)
 */
void TM1639_Delay(uint8_t count)
{
	HAL_Delay(count);
}

/**
 * @brief 写一字节数据
 *
 * @param data 数据
 */
static void TM1639_WriteByte(uint8_t data)
{
	// 逐位发送数据，从最低位开始
	for (uint8_t i = 0; i < 8; i++)
	{
		set_clk_pin(0);			// 时钟线拉低，准备数据发送
		set_data_pin(data & 1); // 设置数据引脚
		data >>= 1;				// 移位准备发送下一位
		set_clk_pin(1);			// 时钟线拉高，完成这一位数据的发送
		TM1639_Delay(1);
	}
}

// static void TM1639_ReadByte(void)
// {
// 	unsigned char i, data, temp = 0;

//     // 配置DIO为输入，准备读取数据
//     set_data_pin(1);
//     for (i = 0; i < 8; i++)
//     {
//         temp >>= 1;                                                      // 为下一位数据腾出位置
//         set_clk_pin(0);                                              // 下降沿准备读取数据
//         data = HAL_GPIO_ReadPin(tm1639Din_GPIO_Port, tm1639Din_Pin); // 从DIN读取一位数据
//         if (data)
// 		{
//             temp |= 0x80;  // 如果读取到1，则设置相应位
// 		}
//         set_clk_pin(1); // 上升沿完成读取
//     }
//     return temp; // 返回读取到的数据
// }

/**
 * @brief 发送数据
 *
 * @param address 地址
 * @param data 数据
 */
static void TM1639_WriteData(uint8_t address, uint8_t data)
{
	set_strobe_pin(0);
	TM1639_WriteByte(0x00 | address);
	TM1639_WriteByte(data);
	set_strobe_pin(1);
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
static void TM1639_WriteCmd(uint8_t cmd)
{
	set_strobe_pin(0);
	TM1639_WriteByte(cmd);
	set_strobe_pin(1);
}

/**
 * @brief 清除显示
 *
 * @param void
 */
void TM1639_Clear(void)
{
	set_strobe_pin(0);
	TM1639_WriteCmd(0xC0); // 设置数据地址
	for (int i = 0; i < 16; i++)
	{
		TM1639_WriteByte(0x00); // 清除所有LED
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
	};
	return digitCode[num];
}

/**
 * @brief 更新TM1639显示数字内容
 *
 * @param nums 数字数组
 * @param dots 小数点数组
 * @param length 数组长度
 */
void TM1639_NumShow(uint8_t nums[], uint8_t dots[], uint8_t length)
{
	const uint8_t pos_base = 0xC0; // 命令字节的基地址
	uint8_t index = 0;

	// 填充数字编码并发送
	for (uint8_t i = 0; i < length; i++)
	{
		// MARK: 3和4的位置调整以符合硬件位置
		index = (i == 3) ? 4 : (i == 4) ? 3
										: i;

		// 获取数字编码并设置小数点
		uint8_t data = getDigitCode(nums[index]) | (dots[index] ? 0x80 : 0x00);

		// 发送数字编码到TM1639
		TM1639_WriteData(pos_base + i * 2, data & 0x0F);   // 发送低4位
		TM1639_WriteData(pos_base + i * 2 + 1, data >> 4); // 发送高4位
	}

	// 更新显示命令，以应用显示内容和亮度设置
	TM1639_WriteCmd(0x40); // 写数据到显示寄存器,自动地址增加
	TM1639_WriteCmd(0x8A); // 显示控制,设置脉冲宽度为4/16
}

/**
 * @brief 更改显示器的亮度
 *
 * @param brightness 亮度等级 1-8, 对应1/16 - 14/16
 */
void TM1639_SetBrightness(uint8_t brightness)
{
	brightness = (brightness == 0) ? 1 : (brightness > 8) ? 8
														  : brightness;
	TM1639_WriteCmd(0x88 + brightness - 1); // 显示亮度
}

/**
 * @brief 控制显示器的开关
 *
 * @param state TM1639_ON: 开启显示器 TM1639_OFF: 关闭显示器
 */
void TM1639_SetDisplayState(TM1639_Switch_e state)
{
	TM1639_WriteCmd(state ? 0x8B : 0x80); // 启用/关闭显示
}

/**
 * @brief 控制显示器电源开关
 *
 * @param PinState TM1639_ON: 开启显示器电源 TM1639_OFF: 关闭显示器
 */
void TM1639_PowerCtrl(TM1639_Switch_e PinState)
{
	HAL_GPIO_WritePin(displayPower_GPIO_Port, displayPower_Pin, !(GPIO_PinState)PinState);
}

/**
 * @brief 读取TM1639按键值
 *
 * @return uint16_t 按键值
 */
uint16_t TM1639_ReadKey(void)
{
	uint16_t key_value = 0; // 存储解析后的按键值
	uint16_t data_16b = 0;
	uint8_t data_8bl = 0; // 从TM1639读取的原始数据
	uint8_t data_8bh = 0;

	// 设置TM1639为读取按键模式
	set_strobe_pin(0);
	TM1639_WriteByte(0x42); // 发送读键扫描模式命令
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
		return KEY_UP;
	}
	else
	{
		return KEY_DOWN;
	}
}

/**
 * @brief 初始化TM1639
 *
 * @param void
 */
void TM1639_Init(void)
{
	set_data_pin_mode(GPIO_PIN_SET);
	set_strobe_pin(1);
	set_clk_pin(1);
	TM1639_Delay(2);

	TM1639_WriteCmd(0x40); // 0100 0000	写数据到显示寄存器,自动地址增加
	TM1639_WriteCmd(0x87); // 1000 0111	显示控制,设置脉冲宽度为14/16

	TM1639_Clear();
}

/**
 * @brief TM1639测试函数，测试使用
 */
void TM1639_Test(void)
{
}
