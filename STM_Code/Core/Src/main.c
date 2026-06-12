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
#include "app_stats.h"
#include "app_stress_test.h"
#include "app_protocol.h"
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
// DWT cycle counter for microsecond timing
#define DWT_CONTROL *(volatile uint32_t*)0xE0001000
#define DWT_CYCCNT  *(volatile uint32_t*)0xE0001004
#define CPU_FREQUENCY_MHZ (SystemCoreClock / 1000000)
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
  // DWT cycle counter enable
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT_CONTROL |= 1;

  // Init custom modules
  stats_init();
  protocol_init();
  stress_test_init();


  // Clear OLED and show startup message
  oled_Clear();
  oled_SetCursor(0, 0);
  oled_WriteString("Stress Test FW", Font_11x18, White);
  oled_SetCursor(0, 20);
  oled_WriteString("Starting...", Font_7x10, White);
  oled_UpdateScreen();
  HAL_Delay(1500);


  // Start CAN Peripheral
  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }

  // Activate CAN RX interrupt
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    Error_Handler();
  }

  // Activate CAN Error interrupts
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR_WARNING | CAN_IT_ERROR_PASSIVE | CAN_IT_BUS_OFF | CAN_IT_LAST_ERROR_CODE | CAN_IT_ERROR) != HAL_OK)
  {
      Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_tick = HAL_GetTick();
  uint32_t last_second_tick = last_tick;
  uint32_t loop_start_cycles = 0;


  while (1)
  {
    // --- Timing and Uptime ---
    loop_start_cycles = DWT_CYCCNT;
    uint32_t current_tick = HAL_GetTick();
    if(current_tick != last_tick)
    {
      stats_get()->uptime_ms += (current_tick - last_tick);
      last_tick = current_tick;
    }

    if (current_tick - last_second_tick >= 1000)
    {
        stats_get()->uptime_s++;
        last_second_tick = current_tick;
    }

    stats_get()->main_loop_counter++;

    // --- Application Logic ---
    stress_test_run_slice(); // Runs CPU stress, FRAM test, OLED update
    protocol_run();          // Runs USB and CAN communication

    // --- LED Management ---
    app_stats_t* stats = stats_get();
    // LED1: Heartbeat (toggles every 500ms)
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, (stats->uptime_ms % 1000 < 500));

    // LED2: USB Activity (blink on tx)
    static uint32_t last_usb_tx = 0;
    if (stats->usb_tx_packets > last_usb_tx) {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        last_usb_tx = stats->usb_tx_packets;
    } else {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    }

    // LED3: CAN Activity (blink on tx/rx)
     static uint32_t last_can_tx = 0;
     static uint32_t last_can_rx = 0;
     if (stats->can_tx_packets > last_can_tx || stats->can_rx_packets > last_can_rx) {
         HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
         last_can_tx = stats->can_tx_packets;
         last_can_rx = stats->can_rx_packets;
     } else {
         HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
     }

    // LED4: Error latched
    if (stats->can_busoff_count > 0 || stats->fram_crc_errors > 0 || stats->i2c_errors > 0)
    {
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET); // Error: LED ON
    } else {
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
    }


    // --- Loop time measurement ---
    uint32_t loop_end_cycles = DWT_CYCCNT;
    uint32_t loop_time_us = (loop_end_cycles - loop_start_cycles) / CPU_FREQUENCY_MHZ;
    stats_update_loop_time(loop_time_us);

    // Small delay to yield CPU time if loop is too fast
    HAL_Delay(1);
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
/**
  * @brief  Rx Fifo 0 message pending callback.
  * @param  hcan: pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rxHeader;
  uint8_t             rxData[8];

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
  {
    protocol_handle_can_rx(&rxHeader, rxData);
  }
}

/**
  * @brief  CAN error callback.
  * @param  hcan: pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    app_stats_t* stats = stats_get();
    uint32_t error = HAL_CAN_GetError(hcan);

    if ((error & HAL_CAN_ERROR_EWG) != 0)
    {
        stats->can_error_warning_count++;
    }
    if ((error & HAL_CAN_ERROR_EPV) != 0)
    {
        stats->can_error_passive_count++;
    }
    if ((error & HAL_CAN_ERROR_BOF) != 0)
    {
        stats->can_busoff_count++;
        // Optional: Add recovery logic here, e.g., re-init CAN
        // Be careful not to get stuck in a re-init loop
        // HAL_CAN_ResetError(hcan);
        // HAL_CAN_Start(hcan);
    }
}

/**
  * @brief  This function handles Hard Fault exception.
  * @retval None
  */
void HardFault_Handler(void)
{
  stats_get()->hardfault_count++;
  // Turn on error LED and loop forever
  HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET);
  while(1)
  {
      for(int i=0; i < 1000000; i++);
      HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin);
  }
}

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
