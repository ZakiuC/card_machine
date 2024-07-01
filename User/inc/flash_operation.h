#ifndef __FLASH_OPERATIONS_H
#define __FLASH_OPERATIONS_H

#include "main.h"

#ifdef __cplusplus
extern "C"
{
#endif

// 初始化Flash模块
void FlashInit(void);

// 写入数据到Flash
HAL_StatusTypeDef FlashWrite(uint32_t address, uint32_t *data, uint32_t length);

// 从Flash读取数据
void FlashRead(uint32_t address, uint32_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_OPERATIONS_H */
