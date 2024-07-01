/* 包含头文件 ----------------------------------------------------------------*/
#include "user_task.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "log.h"
#include "TM1639.h"
#include "bsp_key.h"
#include "console.h"
#include "gpio.h"
#include "test_key.h"

/* 私有宏 ------------------------------------------------------------------*/
#define TM1639_TASK_STACK 128
#define WORK_TASK_STACK 128
#define CONSOLE_TASK_STACK 128
#define TEST_TASK_STACK 128

#define START_TASK_PERIOD 100
#define TM1639_TASK_PERIOD 10
#define WORK_TASK_PERIOD 1
#define CONSOLE_TASK_PERIOD 10
#define TEST_TASK_PERIOD 1000

/* 私有变量 ------------------------------------------------------------------*/
TaskHandle_t TM1639_TaskHandle;
TaskHandle_t Work_TaskHandle;
TaskHandle_t Console_TaskHandle;
TaskHandle_t Test_TaskHandle;


/* 函数声明 ------------------------------------------------------------------*/
static void TM1639_task(void *pvParameters);
static void Console_task(void *pvParameters);
static void Work_task(void *pvParameters);
static void Test_task(void *pvParameters);


static void CreateTask(TaskFunction_t task, const char *name, uint16_t stackSize, TaskHandle_t *taskHandle);
static void vTimerCallback(TimerHandle_t xTimer);
/* 函数体 --------------------------------------------------------------------*/
/**
 * @brief  StartDefaultTask 复写启动任务
 * @param  argument: 未使用
 * @retval None
 */
void StartDefaultTask(void *argument)
{
    (void)argument;
    SEGGER_RTT_Init();
    ConsoleInit();
    TimerHandle_t xTimer;

    xTimer = xTimerCreate("Timer",
                          pdMS_TO_TICKS(1),
                          pdTRUE,
                          (void *)0,
                          vTimerCallback);

    vTaskDelay(pdMS_TO_TICKS(100));
    CreateTask(TM1639_task, "TM1639Task", TM1639_TASK_STACK, &TM1639_TaskHandle);
    // CreateTask(Work_task, "WorkTask", CONSOLE_TASK_STACK, &Console_TaskHandle);
    CreateTask(Console_task, "ConsoleTask", CONSOLE_TASK_STACK, &Console_TaskHandle);
    CreateTask(Test_task, "TestTask", TEST_TASK_STACK, &Test_TaskHandle);
    

    if (xTimer != NULL)
    {
        if (xTimerStart(xTimer, 0) != pdPASS)
        {
            LOG_ERROR("Timer 0 start failed\n");
        }
    }

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(START_TASK_PERIOD));
    }
}

/**
 * @brief  TM1639_task 显示控制任务
 * @param  argument: 未使用
 * @retval None
 */
static void TM1639_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        if (displayInfo.needUpdate) 
        {
            switch (displayInfo.content_type)
            {
            case DIGITAL_CONTENT:    
                TM1639NumShow(displayInfo.digital_content, displayInfo.dot_content, displayInfo.start_pos, displayInfo.length);
                break;
            case STRING_CONTENT:
                TM1639LetterShow(displayInfo.string_content, displayInfo.length, displayInfo.dot_content);
                break;
            case STRING_DIGITAL_CONTENT:
                TM1639RemixShow(displayInfo.string_content, displayInfo.start_pos2, displayInfo.digital_content, (displayInfo.length - displayInfo.start_pos2), displayInfo.dot_content);
                break;
            case MARQUEE_CONTENT:
                MarqueeDisplay(displayInfo.marQuee_index);
                break;
            default:
                break;
            }
            displayInfo.needUpdate = false; // 重置更新标志
        }
        vTaskDelay(pdMS_TO_TICKS(TM1639_TASK_PERIOD));
    }
}

/**
 * @brief  Console_task 控制台任务
 * @param  argument: 未使用
 * @retval None
 */
static void Console_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        KeyScan(); // 按键扫描
        TM1639KeyScan();   // TM1639按键扫描
        ConsoleModeSwitch();   // 更新运行模式

        vTaskDelay(pdMS_TO_TICKS(CONSOLE_TASK_PERIOD));
    }
}

/**
 * @brief  Work_task 工作任务
 * @param  argument: 未使用
 * @retval None
 */
static void Work_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        WorkModeSwitch();
        vTaskDelay(pdMS_TO_TICKS(WORK_TASK_PERIOD));
    }
}

void printSystemClockFrequency(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    uint32_t PFLatency = 0;
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &PFLatency);

    // 获取系统时钟源和频率
    uint32_t sysclk_freq = HAL_RCC_GetSysClockFreq();
    
    LOG_DEBUG("System Clock Frequency: %lu Hz\n", sysclk_freq);
}

/**
 * @brief  Test_task 测试任务
 * @param  argument: 未使用
 * @retval None
 */
static void Test_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(TEST_TASK_PERIOD));
    }
}


/**
 * @brief  CreateTask 任务创建函数
 * @param  task: 任务函数
 * @param  name: 任务名称
 * @param  stackSize: 任务堆栈大小
 * @param  taskHandle: 任务句柄

 * @retval None
 */
static void CreateTask(TaskFunction_t task, const char *name, uint16_t stackSize, TaskHandle_t *taskHandle)
{
    size_t  HeapSize_before = 0;
    size_t  HeapSize_after = 0;

    HeapSize_before = xPortGetFreeHeapSize();
    BaseType_t xReturned = xTaskCreate(task, name, stackSize, NULL, osPriorityNormal, taskHandle);
    HeapSize_after = xPortGetFreeHeapSize();
    if (xReturned == pdPASS)
    {
        LOG_INFO("%s created. handle: 0x%x.\tHeapSize: %u -> %u\n", name, (unsigned int)(*taskHandle), HeapSize_before, HeapSize_after);
    }
    else
    {
        LOG_ERROR("%s task creation failed.\tHeapSize: %u -> %u\n", name, (unsigned int)(*taskHandle), HeapSize_before, HeapSize_after);
    }
}

/**
 * @brief  vTimerCallback 软件定时器回调
 * @param  xTimer: 未使用
 * @retval None
 */
static void vTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer; // 如果不使用 xTimer 参数，可以避免编译器警告
    ConsoleMsHandle();
}
