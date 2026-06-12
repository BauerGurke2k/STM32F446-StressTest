/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "i2c.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "oled.h"
#include "usb.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c3; // External declaration for I2C3 handle
OLED_HandleTypeDef holed;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_I2C3_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // Initialize OLED
  if (OLED_Init(&holed, &hi2c3, OLED_I2C_ADDR_0X3C) != HAL_OK)
  {
    Error_Handler();
  }
  OLED_Clear(&holed);
  OLED_SetCursor(&holed, 0, 0);
  OLED_WriteString(&holed, "USB Debug Init...");
  OLED_UpdateScreen(&holed);
  HAL_Delay(500);

  USBD_StatusTypeDef usb_status;

  usb_status = USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
  OLED_SetCursor(&holed, 0, 8);
  if (usb_status == USBD_OK) {
    OLED_WriteString(&holed, "USBD_Init: OK");
  } else {
    OLED_WriteString(&holed, "USBD_Init: FAIL");
  }
  OLED_UpdateScreen(&holed);
  HAL_Delay(500);

  usb_status = USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
  OLED_SetCursor(&holed, 0, 16);
  if (usb_status == USBD_OK) {
    OLED_WriteString(&holed, "USBD_RegisterClass: OK");
  } else {
    OLED_WriteString(&holed, "USBD_RegisterClass: FAIL");
  }
  OLED_UpdateScreen(&holed);
  HAL_Delay(500);

  usb_status = USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
  OLED_SetCursor(&holed, 0, 24);
  if (usb_status == USBD_OK) {
    OLED_WriteString(&holed, "USBD_CDC_Register: OK");
  } else {
    OLED_WriteString(&holed, "USBD_CDC_Register: FAIL");
  }
  OLED_UpdateScreen(&holed);
  HAL_Delay(500);

  usb_status = USBD_Start(&hUsbDeviceFS);
  OLED_SetCursor(&holed, 0, 32);
  if (usb_status == USBD_OK) {
    OLED_WriteString(&holed, "USBD_Start: OK");
  } else {
    OLED_WriteString(&holed, "USBD_Start: FAIL");
  }
  OLED_UpdateScreen(&holed);
  HAL_Delay(2000);

  OLED_Clear(&holed);
  OLED_SetCursor(&holed, 0, 0);
  OLED_WriteString(&holed, "USB Init Complete");
  OLED_UpdateScreen(&holed);
  HAL_Delay(1000);

  // Start CAN Peripheral
  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }

  CAN_TxHeaderTypeDef TxHeader;
  uint8_t TxData[8];
  uint32_t TxMailbox;

  TxHeader.StdId = 0x123;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.DLC = 8;
  TxHeader.TransmitGlobalTime = DISABLE;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Alle LEDs aus
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);

    // USB Debug Informationen auf OLED anzeigen
    OLED_Clear(&holed);
    OLED_SetCursor(&holed, 0, 0);
    OLED_WriteString(&holed, "USB Status:");

    // VBUS Status (PA9)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET)
    {
      OLED_SetCursor(&holed, 0, 8);
      OLED_WriteString(&holed, "VBUS: HIGH");
    }
    else
    {
      OLED_SetCursor(&holed, 0, 8);
      OLED_WriteString(&holed, "VBUS: LOW");
    }

    // D+ Status (PA12)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_SET)
    {
      OLED_SetCursor(&holed, 0, 16);
      OLED_WriteString(&holed, "DP: HIGH");
    }
    else
    {
      OLED_SetCursor(&holed, 0, 16);
      OLED_WriteString(&holed, "DP: LOW");
    }

    // USB Device State
    // Hier müsstest du den aktuellen USB-Gerätezustand abfragen, 
    // z.B. über USBD_HandleTypeDef.pClassData->pClass->GetState() oder ähnliches
    // Da dies komplexer ist und nicht direkt über HAL_GPIO_ReadPin geht,
    // belasse ich es vorerst bei den Pin-States.

    OLED_UpdateScreen(&holed);
    HAL_Delay(1000); // Update every 1 second

    CDC_SendMessage_FS("USB CDC Message: %lu\r\n", HAL_GetTick());
    USART2_Printf("Hello from STM32! Tick: %lu\r\n", HAL_GetTick());

    // Update CAN Tx data
    TxData[0] = 0xDE;
    TxData[1] = 0xAD;
    TxData[2] = 0xBE;
    TxData[3] = 0xEF;
    TxData[4] = (uint8_t)((HAL_GetTick() >> 24) & 0xFF);
    TxData[5] = (uint8_t)((HAL_GetTick() >> 16) & 0xFF);
    TxData[6] = (uint8_t)((HAL_GetTick() >> 8) & 0xFF);
    TxData[7] = (uint8_t)(HAL_GetTick() & 0xFF);

    // Send CAN Message
    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
