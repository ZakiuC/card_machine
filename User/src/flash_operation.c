#include "flash_operation.h"

/**
 * @brief Flash初始化
*/
void FlashInit(void)
{
    // 如果需要，初始化Flash相关的资源或变量
}


/**
 * @brief 写入Flash
 * @param address: 起始地址
 * @param data: 数据起始地址
 * @param length: 数据长度
 * @return status: 写入结果
*/
HAL_StatusTypeDef FlashWrite(uint32_t address, uint32_t *data, uint32_t length)
{
    HAL_StatusTypeDef status = HAL_OK;
    // 解锁flash
    HAL_FLASH_Unlock();
    
    // 擦除Flash
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;             // 假设只操作第一个Bank
    EraseInitStruct.Page = address / FLASH_PAGE_SIZE; // 计算起始页
    EraseInitStruct.NbPages = 1;                      // 假设每次只擦除1页
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);

    if (status == HAL_OK)
    {
        for (uint32_t i = 0; i < length; i++)
        {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + (i * 8), data[i]) != HAL_OK)
            {
                // mark: 由于FLASH_TYPEPROGRAM_DOUBLEWORD是64bit需要调整data[i]来适应他的数据类型
                status = HAL_ERROR;
                break;
            }
        }
    }       

    HAL_FLASH_Lock();
    return status;
}




/**
 * @brief 读取Flash
 * @param address: 起始地址
 * @param data: 数据缓冲区
 * @param length: 数据长度
*/
void FlashRead(uint32_t address, uint32_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        data[i] = *(__IO uint32_t *)(address + (i * 4));
    }
}
