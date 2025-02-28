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
#include "stm32h7xx_hal.h"

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
void my_printf(const char *fmt, ...);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SE_RST_Pin GPIO_PIN_0
#define SE_RST_GPIO_Port GPIOC
#define BTN_1_Pin GPIO_PIN_0
#define BTN_1_GPIO_Port GPIOA
#define BTN_2_Pin GPIO_PIN_1
#define BTN_2_GPIO_Port GPIOA
#define BTN_3_Pin GPIO_PIN_2
#define BTN_3_GPIO_Port GPIOA
#define BTN_4_Pin GPIO_PIN_3
#define BTN_4_GPIO_Port GPIOA
#define LCD_CS_Pin GPIO_PIN_4
#define LCD_CS_GPIO_Port GPIOC
#define LCD_RESET_Pin GPIO_PIN_5
#define LCD_RESET_GPIO_Port GPIOC
#define LCD_DC_Pin GPIO_PIN_0
#define LCD_DC_GPIO_Port GPIOB
#define USER_BTN_Pin GPIO_PIN_4
#define USER_BTN_GPIO_Port GPIOD
#define CAMERA_PWDN_Pin GPIO_PIN_5
#define CAMERA_PWDN_GPIO_Port GPIOD
#define CAMERA_RESET_Pin GPIO_PIN_6
#define CAMERA_RESET_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
#define JPEG_OUTPUT_DATA_BUFFER  0xD0200000
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
