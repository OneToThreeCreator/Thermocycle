/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim9;
extern UART_HandleTypeDef huart2;
extern RTC_HandleTypeDef hrtc;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void debug(const char *fmt, ...);
size_t uint64_to_string(uint64_t value, char *buffer, size_t buffer_size);
size_t int64_to_string(int64_t value, char *buffer, size_t buffer_size);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define USART2_REDE_Pin GPIO_PIN_3
#define USART2_REDE_GPIO_Port GPIOA
#define SPI1_SS_Pin GPIO_PIN_4
#define SPI1_SS_GPIO_Port GPIOA
#define ENDBUTTON_Pin GPIO_PIN_10
#define ENDBUTTON_GPIO_Port GPIOB
#define AIR_1_Pin GPIO_PIN_12
#define AIR_1_GPIO_Port GPIOB
#define AIR_2_Pin GPIO_PIN_13
#define AIR_2_GPIO_Port GPIOB
#define EMERGENCY_BUTTON_Pin GPIO_PIN_4
#define EMERGENCY_BUTTON_GPIO_Port GPIOB
#define MOTOR_ENA_Pin GPIO_PIN_6
#define MOTOR_ENA_GPIO_Port GPIOB
#define DIR_Pin GPIO_PIN_7
#define DIR_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
