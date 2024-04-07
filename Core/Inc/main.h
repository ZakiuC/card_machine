/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define rotateMotorFi_Pin GPIO_PIN_9
#define rotateMotorFi_GPIO_Port GPIOB
#define powerKey_Pin GPIO_PIN_14
#define powerKey_GPIO_Port GPIOC
#define powerKey_EXTI_IRQn EXTI4_15_IRQn
#define touchKey_Pin GPIO_PIN_15
#define touchKey_GPIO_Port GPIOC
#define rotateOptoKey_Pin GPIO_PIN_0
#define rotateOptoKey_GPIO_Port GPIOA
#define batVol_Pin GPIO_PIN_1
#define batVol_GPIO_Port GPIOA
#define outSdb628Enable_Pin GPIO_PIN_2
#define outSdb628Enable_GPIO_Port GPIOA
#define tm1639Clk_Pin GPIO_PIN_3
#define tm1639Clk_GPIO_Port GPIOA
#define tm1639Din_Pin GPIO_PIN_4
#define tm1639Din_GPIO_Port GPIOA
#define tm1639Stb_Pin GPIO_PIN_5
#define tm1639Stb_GPIO_Port GPIOA
#define outMotorFi_Pin GPIO_PIN_0
#define outMotorFi_GPIO_Port GPIOB
#define outMotorBi_Pin GPIO_PIN_1
#define outMotorBi_GPIO_Port GPIOB
#define outputOptoKey_Pin GPIO_PIN_2
#define outputOptoKey_GPIO_Port GPIOB
#define displayPower_Pin GPIO_PIN_8
#define displayPower_GPIO_Port GPIOA
#define cs5080Stat_Pin GPIO_PIN_9
#define cs5080Stat_GPIO_Port GPIOA
#define buzzer_Pin GPIO_PIN_11
#define buzzer_GPIO_Port GPIOA
#define rotateSdb628Enable_Pin GPIO_PIN_6
#define rotateSdb628Enable_GPIO_Port GPIOB
#define rotateMotorBi_Pin GPIO_PIN_8
#define rotateMotorBi_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
