/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>		// For sprintf message to be sent by USART1.
#include <string.h>		// For strlen, used in sending message by UART1.
#include <stdlib.h>
#include <time.h>	// For random number generator (RNG), to simulate Sensor data.
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
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

RNG_HandleTypeDef hrng;

UART_HandleTypeDef huart1;

/* Definitions for healthCheckTask */
osThreadId_t healthCheckTaskHandle;
const osThreadAttr_t healthCheckTask_attributes = {
  .name = "healthCheckTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for telemetryTask */
osThreadId_t telemetryTaskHandle;
const osThreadAttr_t telemetryTask_attributes = {
  .name = "telemetryTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LinkMonitorTask */
osThreadId_t LinkMonitorTaskHandle;
const osThreadAttr_t LinkMonitorTask_attributes = {
  .name = "LinkMonitorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for sensorTask */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for adcSem */
osSemaphoreId_t adcSemHandle;
const osSemaphoreAttr_t adcSem_attributes = {
  .name = "adcSem"
};
/* USER CODE BEGIN PV */


// Variables needed to receive messages from the USART1.
#define RX_BUFFER_SIZE 32
uint8_t rx_byte;
char command_buffer[RX_BUFFER_SIZE];
uint8_t cmd_index = 0;
// Variables related to the log.
#define LOG_BUFFER_SIZE 1024
char log_buffer[LOG_BUFFER_SIZE];
size_t log_index = 0;				//size_t is defined as uint32_t on STM32.
//Random Number Generator:
extern RNG_HandleTypeDef hrng;
// Link Strength Status
uint8_t rssi = 0;			//Strength of the satellite Uplink/Downlink in 0-100%. (Received Signal Strength Indicator)
/* Buffer to store ADC values */
uint16_t adc_value;
char uart_buffer[20];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RNG_Init(void);
void StartHealthCheckTask(void *argument);
void StartTelemetryTask(void *argument);
void StartLinkMonitorTask(void *argument);
void StartSensorTask(void *argument);

/* USER CODE BEGIN PFP */
//void ReadSensor();
void TelemetryToGCS();
void log_message(const char* msg);
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);	// Call the callback func "HAL_UART_RxCpltCallback" when a message is received over USART1. This must be reset after message reception, if another message will be received.

  //Log message to indicate that satellite subsystem is ready.
  log_message("Device Ready.");
  /* Start ADC with DMA */
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_value, 1);
  // Send a welcome message to the GCS console.
  char welcomeMessage1[] = "\r\n--- Welcome to the Ground Control Station ---\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)welcomeMessage1, strlen(welcomeMessage1), HAL_MAX_DELAY);
  char welcomeMessage2[] = "LED ON - LED OFF - DUMP LOG - STATUS\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)welcomeMessage2, strlen(welcomeMessage2), HAL_MAX_DELAY);
  char welcomeMessage3[] = "Type a command below.\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)welcomeMessage3, strlen(welcomeMessage3), HAL_MAX_DELAY);


  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, 0); 	//Make sure LED is off at the start.
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of adcSem */
  adcSemHandle = osSemaphoreNew(1, 1, &adcSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of healthCheckTask */
  healthCheckTaskHandle = osThreadNew(StartHealthCheckTask, NULL, &healthCheckTask_attributes);

  /* creation of telemetryTask */
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);

  /* creation of LinkMonitorTask */
  LinkMonitorTaskHandle = osThreadNew(StartLinkMonitorTask, NULL, &LinkMonitorTask_attributes);

  /* creation of sensorTask */
  sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BL_CTRL_Pin_GPIO_Port, LCD_BL_CTRL_Pin_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LCD_BL_CTRL_Pin_Pin */
  GPIO_InitStruct.Pin = LCD_BL_CTRL_Pin_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BL_CTRL_Pin_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD1_Pin */
  GPIO_InitStruct.Pin = LD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// Logger
void log_message(const char* msg) {

	// Remove any \n or \r from end of the input message. This help with proper formatting at the GCS.
    char clean_msg[100];
    strncpy(clean_msg, msg, sizeof(clean_msg) - 1);	// Create a local copy of the message
    clean_msg[sizeof(clean_msg) - 1] = '\0';

    // Strip trailing newline or carriage return
    size_t clean_len = strlen(clean_msg);
    while (clean_len > 0 && (clean_msg[clean_len - 1] == '\n' || clean_msg[clean_len - 1] == '\r')) {
        clean_msg[--clean_len] = '\0';
    }


	char log_entry[128];

	// Get time since last reboot.
	uint32_t ms = HAL_GetTick();
	uint32_t sec = ms / 1000;
	uint32_t h = sec / 3600;
	uint32_t m = (sec % 3600) / 60;
	uint32_t s = sec % 60;

	snprintf(log_entry, sizeof(log_entry), "[%02lu:%02lu:%02lu] %s\r\n", h, m, s, clean_msg);

	size_t entry_len = strlen(log_entry);
    if (log_index + entry_len < LOG_BUFFER_SIZE) {		// Prevent buffer overflow.
        strcpy(&log_buffer[log_index], log_entry);
        log_index += entry_len;
    }
}

// Send a message to the Ground Control Station (GCS), by USART1.
void TelemetryToGCS()
{
	char msg[128]={0};																								// Message string, to be sent over USART1/Serial/USB to Ground Control Station. Initialized to 0, to prevent garbage text.
	uint32_t uptime = HAL_GetTick();  // milliseconds since boot

	// Send message
	snprintf(msg, sizeof(msg), "STATUS: | Uptime: %7lu ms | Sensor: %3d | Link: %3d\r\n", uptime,adc_value,rssi);	// Populate msg string with text and values. Use snprintf to prevent buffer overflow.
	HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);											// Send the message string out through USART1 / USB.
	//log_message(msg);
}


// Invoke this function when data is received from USART1 and it's interrupt is triggered.
// This is the data received from the Ground Control Station
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)		// Copied from "stm32f7xx_hal_uart.c".
{
	if (huart->Instance == USART1) {
		HAL_UART_Transmit(&huart1, &rx_byte, 1, HAL_MAX_DELAY);	// Echo the received character
	    if (rx_byte == '\r' || rx_byte == '\n') {
	    	command_buffer[cmd_index] = '\0';  // Null-terminate
	    	// Respond and act on command.
	    	// Use IF here instead of SWITCH, since CASE can't be used with strings ("case "LED ON":" will give error).
	    	if (strcmp(command_buffer, "LED ON") == 0) {
	    		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);  // Turn LED on
	            char message[] = "LED turned ON";
	            char response[64];
	            snprintf(response, sizeof(response), "\r\n%s\r\n", message);
	    		HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response), HAL_MAX_DELAY);
	    		log_message(message);
	    	}
	    	else if (strcmp(command_buffer, "LED OFF") == 0) {
	            HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);  // Turn LED off
	            char message[] = "LED turned OFF";
	            char response[64];
	            snprintf(response, sizeof(response), "\r\n%s\r\n", message);
                HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response), HAL_MAX_DELAY);
                log_message(message);
	        }
	    	else if (strcmp(command_buffer, "STATUS") == 0) {
		    	TelemetryToGCS();			// Send Status to the Ground Control Station (GCS).
	        }
	    	else if (strcmp(command_buffer, "DUMP LOG") == 0) {
                char response[] = "\r\nLOG DUMP:\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response), HAL_MAX_DELAY);
	    		HAL_UART_Transmit(&huart1, (uint8_t*)log_buffer, log_index, HAL_MAX_DELAY);
	    		char response2[] = "\r\nLOG DUMP complete\r\n";
	    		HAL_UART_Transmit(&huart1, (uint8_t *)response2, strlen(response2), HAL_MAX_DELAY);
	        }
	    	else {
                char response[] = "\r\nUnknown command\r\n";
                HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response), HAL_MAX_DELAY);
	    	}

	    	cmd_index = 0;  // Reset for next command
	    }

	    else {
	    	if (cmd_index < RX_BUFFER_SIZE - 1) {
	    		command_buffer[cmd_index++] = rx_byte;
	        }
	    }
	    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);  // Re-enable interrupt
	}
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartHealthCheckTask */
/**
  * @brief  Function implementing the healthCheckTask thread.
  * Blink the on-board LED briefly, every 2 seconds, to show 'Heart beat'. This should be sent via telemetry.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartHealthCheckTask */
void StartHealthCheckTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, 1);
	  osDelay(250);			// Delay, in Ticks. 1 Tick ~ 1ms.
	  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, 0);
	  osDelay(2000);		// Delay, in Ticks.
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTelemetryTask */
/**
* @brief Function implementing the telemetryTask thread. Send data to the GCS peridically.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTelemetryTask */
void StartTelemetryTask(void *argument)
{
  /* USER CODE BEGIN StartTelemetryTask */
  /* Infinite loop */
  for(;;)
  {
	  TelemetryToGCS();
	  osDelay(3000);	// Send telemetry data to GCS every 3 seconds
  }
  /* USER CODE END StartTelemetryTask */
}

/* USER CODE BEGIN Header_StartLinkMonitorTask */
/**
* @brief Function implementing the LinkMonitorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLinkMonitorTask */
void StartLinkMonitorTask(void *argument)
{
  /* USER CODE BEGIN StartLinkMonitorTask */
  /* Infinite loop */
  for(;;)
  {
	  rssi = rand() % 100;  // 0 to 99
	  char msg[64];

	  if (rssi < 20) {
		  snprintf(msg, sizeof(msg), "LINK: LOST (RSSI: %d)\r\n", rssi);
		  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
	  }

	  osDelay(1000);	//Check link every 1 seconds
  }
  /* USER CODE END StartLinkMonitorTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the sensorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  /* Infinite loop */
  for(;;)
  {
	  // ---- Random number version
	  // Generate a random number, that is used to simulate the sensor value. Between 1 and 100. Use STM32F7's hardware RNG.
	  // This is used instead of ADC conversion to generate a value for the Sensor.
	  uint32_t value;
	  HAL_RNG_GenerateRandomNumber(&hrng, &value);
	  //int sensorValue = (value % 100) + 1;
	  adc_value = (value % 100) + 1;
    osDelay(1000);
  }
  /* USER CODE END StartSensorTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

#ifdef  USE_FULL_ASSERT
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
